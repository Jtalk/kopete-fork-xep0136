/*
   webpresenceplugin.cpp

   Kopete Web Presence plugin

   Copyright (c) 2002,2003 by Will Stephenson <will@stevello.free-online.co.uk>

   Kopete    (c) 2002,2003 by the Kopete developers  <kopete-devel@kde.org>

 *************************************************************************
 *                                                                    	*
 * This program is free software; you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation; either version 2 of the License, or     *
 * (at your option) any later version.                                   *
 *                                                                       *
 *************************************************************************
 */

#include "config.h"

#include <qdom.h>
#include <qtimer.h>
#include <qfile.h>

#include <kdebug.h>
#include <kconfig.h>
#include <kgenericfactory.h>
#include <ktempfile.h>
#include <kstandarddirs.h>

#ifdef HAVE_XSLT
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <libxslt/xsltconfig.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#endif

#include "kopetepluginmanager.h"
#include "kopeteprotocol.h"
#include "kopeteaccountmanager.h"
#include "kopeteaccount.h"

#include "webpresenceplugin.h"

typedef KGenericFactory<WebPresencePlugin> WebPresencePluginFactory;
K_EXPORT_COMPONENT_FACTORY( kopete_webpresence, WebPresencePluginFactory( "kopete_webpresence" )  )

WebPresencePlugin::WebPresencePlugin( QObject *parent, const char *name, const QStringList& /*args*/ )
: KopetePlugin( WebPresencePluginFactory::instance(), parent, name )
{
	m_writeScheduler = new QTimer( this );
	connect ( m_writeScheduler, SIGNAL( timeout() ), this, SLOT( slotWriteFile() ) );
	connect( KopeteAccountManager::manager(), SIGNAL(accountReady(KopeteAccount*)),
				this, SLOT( listenToAllAccounts() ) );
	connect( KopeteAccountManager::manager(), SIGNAL(accountUnregistered(KopeteAccount*)),
				this, SLOT( listenToAllAccounts() ) );
	

	connect(this, SIGNAL(settingsChanged()), this, SLOT( loadSettings() ) );
	loadSettings();
	listenToAllAccounts();
}

WebPresencePlugin::~WebPresencePlugin()
{
}

void WebPresencePlugin::loadSettings()
{
	KConfig *kconfig = KGlobal::config();
	kconfig->setGroup( "Web Presence Plugin" );
	
	frequency = kconfig->readNumEntry("UploadFrequency" , 15);
	
	url = kconfig->readEntry("uploadURL");
	useDefaultStyleSheet = kconfig->readBoolEntry("formatDefault", true);
	justXml = kconfig->readBoolEntry("formatXML", false);
	userStyleSheet = kconfig->readEntry("formatStylesheetURL");

	useImName = kconfig->readBoolEntry("showName", true);
	userName = kconfig->readEntry("showThisName");
	showAddresses = kconfig->readBoolEntry("includeIMAddress", false);
}


void WebPresencePlugin::listenToAllAccounts()
{
	// connect to signals notifying of all accounts' status changes
	QPtrList<KopeteProtocol> protocols = allProtocols();
	for ( KopeteProtocol *p = protocols.first();
			p; p = protocols.next() )
	{
		QDict<KopeteAccount> dict=KopeteAccountManager::manager()->accounts( p );
		QDictIterator<KopeteAccount> it( dict );
		for( ; KopeteAccount *account=it.current(); ++it )
		{
			 listenToAccount( account );
		}
	}
	slotWaitMoreStatusChanges();
}

void WebPresencePlugin::listenToAccount( KopeteAccount* account )
{
	if(account->myself())
	{
		// Connect to the account's status changed signal
		// because we can't know if the account has already connected
		QObject::disconnect( account->myself(),
						SIGNAL(onlineStatusChanged( KopeteContact *,
								const KopeteOnlineStatus &,
								const KopeteOnlineStatus & ) ),
						this,
						SLOT( slotWaitMoreStatusChanges() ) ) ;
		QObject::connect( account->myself(),
						SIGNAL(onlineStatusChanged( KopeteContact *,
								const KopeteOnlineStatus &,
								const KopeteOnlineStatus & ) ),
						this,
						SLOT( slotWaitMoreStatusChanges() ) );
	}
}

void WebPresencePlugin::slotWaitMoreStatusChanges()
{
	if ( !m_writeScheduler->isActive() )
		 m_writeScheduler->start( frequency * 1000);

}

void WebPresencePlugin::slotWriteFile()
{
	bool error = false;
	// generate the (temporary) XML file representing the current contactlist
	KTempFile* xml = generateFile();
	xml->setAutoDelete( true );

	if ( url.isEmpty() )
	{
		kdDebug(14309) << "url is empty. NOT UPDATING!" << endl;
		error = true;
	}

	kdDebug(14309) << k_funcinfo << " " << xml->name() << endl;

	if ( justXml )
	{
		m_output = xml;
		xml = 0L;
	}
	else
	{
		// transform XML to the final format
		m_output = new KTempFile();
		m_output->setAutoDelete( true );
		if ( !transform( xml, m_output ) )
		{
			error = true;
			delete m_output;
		}
		delete xml; // might make debugging harder!
	}
	if ( !error )
	{
		// upload it to the specified URL
		KURL src( m_output->name() );
		KURL dest( url );
		KIO::FileCopyJob *job = KIO::file_copy( src, dest, -1, true, false, false );
		connect( job, SIGNAL( result( KIO::Job * ) ),
				SLOT(  slotUploadJobResult( KIO::Job * ) ) );
	}
	m_writeScheduler->stop();
}

void WebPresencePlugin::slotUploadJobResult( KIO::Job *job )
{
	if (  job->error() ) {
		kdDebug(14309) << "Error uploading presence info." << endl;
		job->showErrorDialog( 0 );
	}
	delete m_output;
	return;
}

KTempFile* WebPresencePlugin::generateFile()
{
	// generate the (temporary) XML file representing the current contactlist
	kdDebug( 14309 ) << k_funcinfo << endl;
	QString notKnown = i18n( "Not yet known" );

	QDomDocument doc( "webpresence" );
	QDomElement root = doc.createElement( "webpresence" );
	doc.appendChild( root );

	// insert the current date/time
	QDomElement date = doc.createElement( "listdate" );
	QDomText t = doc.createTextNode( 
			KGlobal::locale()->formatDateTime( QDateTime::currentDateTime() ) );
	date.appendChild( t );
	root.appendChild( date );

	// insert the user's name
	QDomElement name = doc.createElement( "name" );
	QDomText nameText;
	if ( !useImName && !userName.isEmpty() )
		nameText = doc.createTextNode( userName );
	else
		nameText = doc.createTextNode( notKnown );
	name.appendChild( nameText );
	root.appendChild( name );

	// insert the list of the user's accounts
	QDomElement accounts = doc.createElement( "accounts" );
	root.appendChild( accounts );

	QPtrList<KopeteAccount> list = KopeteAccountManager::manager()->accounts();
	// If no accounts, stop here
	if ( !list.isEmpty() )
	{
		for( QPtrListIterator<KopeteAccount> it( list );
			 KopeteAccount *account=it.current();
			 ++it )
		{
			QDomElement acc = doc.createElement( "account" );
			//output += h.openTag( "account" );

		QDomElement protoName = doc.createElement( "protocol" );
		QDomText protoNameText = doc.createTextNode(
				account->protocol()->pluginId() );
		protoName.appendChild( protoNameText );
		acc.appendChild( protoName );

			KopeteContact* me = account->myself();
			QDomElement accName = doc.createElement( "accountname" );
			QDomText accNameText = doc.createTextNode( ( me )
					? me->displayName().latin1()
					: notKnown.latin1() );
			accName.appendChild( accNameText );
			acc.appendChild( accName );

			QDomElement accStatus = doc.createElement( "accountstatus" );
			QDomText statusText = doc.createTextNode( ( me )
					? statusAsString( me->onlineStatus() ).latin1()
					: notKnown.latin1() ) ;
			accStatus.appendChild( statusText );
			acc.appendChild( accStatus );

			if ( showAddresses )
			{
				QDomElement accAddress = doc.createElement( "accountaddress" );
				QDomText addressText = doc.createTextNode( ( me )
						? me->contactId().latin1()
						: notKnown.latin1() );
				accAddress.appendChild( addressText );
				acc.appendChild( accAddress );
			}

			accounts.appendChild( acc );
		}
	}

	// write the XML to a temporary file
	KTempFile* file = new KTempFile();
	QTextStream *stream = file->textStream();
	stream->setEncoding( QTextStream::UnicodeUTF8 );
	doc.save( *stream, 0 );
	file->close();
	return file;
}

bool WebPresencePlugin::transform( KTempFile * src, KTempFile * dest )
{
#ifdef HAVE_XSLT
	QString error = "";
	xmlSubstituteEntitiesDefault( 1 );
	xmlLoadExtDtdDefaultValue = 1;
	// test if the stylesheet exists
	QFile sheet;
	if ( useDefaultStyleSheet )
		sheet.setName( locate( "appdata", "webpresence/webpresencedefault.xsl" ) );
	else
		sheet.setName( userStyleSheet );
	
	if ( sheet.exists() )
	{
		// and if it is a valid stylesheet
		xsltStylesheetPtr cur = NULL;
		if ( ( cur = xsltParseStylesheetFile(
					( const xmlChar *) sheet.name().latin1() ) ) )
		{
			// and if we can parse the input XML
			xmlDocPtr doc = NULL;
			if ( ( doc = xmlParseFile( QFile::encodeName( src->name() ) ) ) != 0L )
			{
				// and if we can apply the stylesheet
				xmlDocPtr res = NULL;
				if ( ( res = xsltApplyStylesheet( cur, doc, 0 ) ) )
				{
					// and if we can save the result
					if ( xsltSaveResultToFile(dest->fstream() , res, cur)
							!= -1 )
					{
						// then it all worked!
						dest->close();
					}
					else
						error = "write result!";
				}
				else
				{
					error = "apply stylesheet!";
					error += " Check the stylesheet works using xsltproc";
				}
				xmlFreeDoc(res);
			}
			else
				error = "parse input XML!";
			xmlFreeDoc(doc);
		}
		else
			error = "parse stylesheet!";
		xsltFreeStylesheet(cur);
	}
	else
		error = "find stylesheet" + sheet.name() + "!";

	xsltCleanupGlobals();
	xmlCleanupParser();
	
	if ( error.isEmpty() )
		return true;
	else
	{
		kdDebug(14309) << k_funcinfo << " - couldn't "
			<< error << endl;
		return false;
	}
#else
	Q_UNUSED( src );
	Q_UNUSED( dest );

	return false;
#endif
}

QPtrList<KopeteProtocol> WebPresencePlugin::allProtocols()
{
	kdDebug( 14309 ) << k_funcinfo << endl;

	QMap<KPluginInfo *, KopetePlugin *> plugins = KopetePluginManager::self()->loadedPlugins( "Protocols" );
	QMap<KPluginInfo *, KopetePlugin *>::ConstIterator it;
	QPtrList<KopeteProtocol> result;
	for ( it = plugins.begin(); it != plugins.end(); ++it )
		result.append( static_cast<KopeteProtocol *>( it.data() ) );

	return result;
}

QString WebPresencePlugin::statusAsString( const KopeteOnlineStatus &newStatus )
{
	QString status;
	switch ( newStatus.status() )
	{
	case KopeteOnlineStatus::Online:
		status = "ONLINE";
		break;
	case KopeteOnlineStatus::Away:
		status = "AWAY";
		break;
	case KopeteOnlineStatus::Offline:
		status = "OFFLINE";
		break;
	default:
		status = "UNKNOWN";
	}

	return status;
}

// vim: set noet ts=4 sts=4 sw=4:
#include "webpresenceplugin.moc"
