/*
    yahooprotocol.cpp - Yahoo Plugin for Kopete

    Copyright (c) 2002 by Duncan Mac-Vicar Prett <duncan@kde.org>

    Copyright (c) 2002 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

/* QT Includes */
#include <qcursor.h>
#include <qwidget.h>
#include <qobject.h>

/* KDE Includes */
#include <kdebug.h>
#include <kaction.h>
#include <kpopupmenu.h>
#include <kstatusbar.h>
#include <kgenericfactory.h>
#include <kiconloader.h>
#include <kstandarddirs.h>
#include <ksimpleconfig.h>
#include <klocale.h>

/* Local Includes */
#include "yahoodebug.h"
#include "yahoostatus.h"
#include "yahooprotocol.h"
#include "yahoocontact.h"

/* Kopete Includes */
#include "kopete.h"
#include "kopetecontact.h"
#include "kopetemetacontact.h"
#include "kopetecontactlist.h"
#include "statusbaricon.h"
#include "systemtray.h"

K_EXPORT_COMPONENT_FACTORY( kopete_yahoo, KGenericFactory<YahooProtocol> );

class KPopupMenu;

YahooProtocol::YahooProtocol( QObject *parent, const char *name, const QStringList & ) : KopeteProtocol( parent, name )
{
	kdDebug() << "YahooProtocol::YahooProtocol()" << endl;
	kdDebug() << "Loading Yahoo Plugin..." << endl;

	if ( !protocolStatic_ )
	{
		protocolStatic_ = this;
	}
	else
	{
		kdDebug() << "YahooProtocol already initialized" << endl;
	}
	
	/* Init actions and icons and create the status bar icon */
	initIcons();
	initActions();
	statusBarIcon = new StatusBarIcon();
	
	QObject::connect(statusBarIcon, SIGNAL(rightClicked(const QPoint&)), this, SLOT(slotIconRightClicked(const QPoint&)));
			
	statusBarIcon->setPixmap(offlineIcon);

	/* Create preferences menu */
	mPrefs = new YahooPreferences("yahoo_protocol_32", this);
	connect(mPrefs, SIGNAL(saved(void)), this, SLOT(slotSettingsChanged(void)));

	mIsConnected = false;

	/* Call slotSettingsChanged() to get it all registered. */
	slotSettingsChanged();

	if (KGlobal::config()->readBoolEntry( "AutoConnect", false ) )
	{
		Connect();
	}
}

YahooProtocol::~YahooProtocol()
{
	kdDebug() << "YahooProtocol::~YahooProtocol()" << endl;
	protocolStatic_ = 0L;
}

YahooProtocol* YahooProtocol::protocolStatic_ = 0L;

/***************************************************************************
 *                                                                         *
 *   Re-implementation of Plugin class methods                             *
 *                                                                         *
 ***************************************************************************/

bool YahooProtocol::serialize( KopeteMetaContact *metaContact, QStringList &strList ) const
{
	kdDebug() << "[YahooProtocol::serialize] Serializing metacontact" << endl;
	
	KopeteContact *c;
	bool done = false;
	
	for( c = metaContact->contacts().first(); c ; c = metaContact->contacts().next() )
	{
		if ( c->protocol() == this )
		{
			kdDebug() << "[YahooProtocol::serialize] Found Yahoo Contact in MetaContact" << endl;
			YahooContact *g = static_cast<YahooContact*>(c);

			if ( !g )
			{
				/* try the next one :) */
				continue;
			}
				
			strList << g->displayName() << g->group();
			metaContact->setAddressBookField( YahooProtocol::protocol(), "messaging/yahoo" , g->id() );
			done = true;
		}
	}
	
	return done;
}

YahooProtocol *YahooProtocol::protocol()
{
	return protocolStatic_;
}


void YahooProtocol::deserialize( KopeteMetaContact *metaContact, const QStringList &strList )
{
	kdDebug() << "[YahooProtocol::deserialize] Deserializing metacontact" << endl;
	
	QString protocolId = this->id();

	QString userid, alias, group;
	userid = metaContact->addressBookField( this, "messaging/yahoo" ) ;

	alias = strList[0];
	
	if ( strList[1] != QString::null )
		group = strList[1];
	else
		group = QString::null;

	/* Robustness (TM) */
	if ( userid == QString::null )
	{
		kdDebug() << "[YahooProtocol::deserialize] null userid! trying next!" << endl;
		return;
	}
	
	if ( m_contactsMap.contains(userid) )
	{
		kdDebug() << "[YahooProtocol::deserialize] User " << userid << " already in contacts map" << endl;
		return;
	}
	
	addContact(userid, alias, group,metaContact);
}

QStringList YahooProtocol::addressBookFields() const
{
	return QStringList("messaging/yahoo");
}

// Unload statusbar icon
bool YahooProtocol::unload()
{
	kdDebug() << "YahooProtocol::unload()" << endl;

	if (kopeteapp->statusBar()) 
	{
		kopeteapp->statusBar()->removeWidget(statusBarIcon);
		delete statusBarIcon;
	}

	return KopeteProtocol::unload();
}

void YahooProtocol::init()
{
}

/***************************************************************************
 *                                                                         *
 *   Re-implementation of KopeteContact Class Methods                      *
 *                                                                         *
 ***************************************************************************/

KopeteContact *YahooProtocol::myself() const
{
#warning "For future maintainers : reimplement this!"
	return 0L;
}

void YahooProtocol::Connect()
{
	YahooSession *session_;
	session_ = 0L;
	
	kdDebug() << "YahooProtocol::Connect()" << endl;

	if ( ! isConnected() )
	{
		kdDebug() << "Attempting to connect to Yahoo server <"
			<< mServer << ":" << mPort << "< with user <" << mUsername << ">" << endl;

		session_ = YahooSessionManager::manager()->createSession( mPrefs->username(), mPrefs->password(), YAHOO_STATUS_AVAILABLE);
		m_session = session_;
	}
	else if (isAway())
	{	// They're really away, and they want to un-away.
		// XXX slotGoOnline();
	}
	else
	{			
		// Nope, just your regular crack junky.
		kdDebug() << "Yahoo plugin: Ignoring connect request (already connected)." <<endl;
	}
	
	if (session_) 
	{
		statusBarIcon->setPixmap(onlineIcon);
		mIsConnected = true;
		
		/* We have a session, time to connect its signals to our plugin slots */
		QObject::connect( session_ , SIGNAL(loginResponse( int,  const QString &)), this , SLOT(slotLoginResponse( int, const QString &)) );
		QObject::connect( session_ , SIGNAL(gotBuddy(const QString &, const QString &, const QString &)), this, SLOT(slotGotBuddy(const QString &, const QString &, const QString &)));
		QObject::connect( session_ , SIGNAL(gotIgnore( YList *)), this , SLOT(void slotGotIgnore( YList * )) );
		QObject::connect( session_ , SIGNAL(gotIdentities( YList *)), this , SLOT(slotGotIdentities( YList *)) );
		QObject::connect( session_ , SIGNAL(statusChanged( const QString &, int, const QString &, int)), this , SLOT(slotStatusChanged( const QString &, int , const QString &, int )) );
		QObject::connect( session_ , SIGNAL(gotIm( const QString &, const QString &, long, int )), this , SLOT(slotGotIm( const QString &, const QString &, long, int)) );
		QObject::connect( session_ , SIGNAL(gotConfInvite( const QString &, const QString &, const QString &, YList *)), this , SLOT(slotGotConfInvite( const QString &, const QString &, const QString &, YList *)) );
		QObject::connect( session_ , SIGNAL(confUserDecline( const QString &, const QString &, const QString &)), this , SLOT(slotConfUserDecline( const QString &, const QString &, const QString &)) );
		QObject::connect( session_ , SIGNAL(confUserJoin( const QString &, const QString &)), this , SLOT(slotConfUserJoin( const QString &, const QString &)) );
		QObject::connect( session_ , SIGNAL(confUserLeave( const QString &, const QString &)), this , SLOT(slotConfUserLeave( const QString &, const QString &)) );
		QObject::connect( session_ , SIGNAL(confMessage( const QString &, const QString &, const QString &)), this , SLOT(slotConfMessage( const QString &, const QString &, const QString &)) );
		QObject::connect( session_ , SIGNAL(gotFile( const QString &, const QString &, long expires, const QString &, const QString &, unsigned long)), this , SLOT(slotGotFile( const QString &, const QString &, long , const QString &, const QString &, unsigned long )) );
		QObject::connect( session_ , SIGNAL(contactAdded( const QString &, const QString &, const QString &)), this , SLOT(slotContactAdded( const QString &, const QString &, const QString &)) );
		QObject::connect( session_ , SIGNAL(rejected( const QString &, const QString &)), this , SLOT(slotRejected( const QString &, const QString &)) );
		QObject::connect( session_ , SIGNAL(typingNotify( const QString &, int)), this , SLOT(slotTypingNotify( const QString &, int )) );
		QObject::connect( session_ , SIGNAL(gameNotify( const QString &, int)), this , SLOT(slotGameNotify( const QString &, int )) );
		QObject::connect( session_ , SIGNAL(mailNotify( const QString &, const QString &, int )), this , SLOT(slotMailNotify( const QString &, const QString &, int )) );
		QObject::connect( session_ , SIGNAL(systemMessage( const QString &)), this , SLOT(slotSystemMessage( const QString &)) );
		QObject::connect( session_ , SIGNAL(error( const QString &, int )), this , SLOT(slotError( const QString &, int )) );
	}
	
}

void YahooProtocol::Disconnect()
{
	kdDebug() << "YahooProtocol::Disconnect()" << endl;

	if (isConnected()) 
	{
		kdDebug() <<  "Attempting to disconnect from Yahoo server " << mServer << endl;
		
		
		m_session->logOff();
		statusBarIcon->setPixmap(offlineIcon);
		//m_engine->Disconnect();
		mIsConnected = false;
	}
	else
	{
		// Again, what's with the crack? Sheez.
		kdDebug() << "Ignoring disconnect request (not connected)." << endl;
	}
}

void YahooProtocol::setAvailable()
{
	kdDebug() << "YahooProtocol::setAvailable()" << endl;
}

void YahooProtocol::setAway()
{
	kdDebug() << "YahooProtocol::setAway()" << endl;
}

bool YahooProtocol::isConnected() const
{
	kdDebug() << "YahooProtocol::isConnected()" << endl;
	return mIsConnected; // XXX
}

bool YahooProtocol::isAway() const
{
	kdDebug() << "YahooProtocol::isAway()" << endl;

	return false; // XXX
}

QString YahooProtocol::protocolIcon() const
{
	kdDebug() <<  "YahooProtocol::protocolIcon" << endl;
	
	return ""; // XXX
}

AddContactPage *YahooProtocol::createAddContactWidget(QWidget * parent)
 {
	kdDebug() << "YahooProtocol::createAddContactWidget(<parent>)" << endl;
	//return (new YahooAddContactPage(this,parent));
	return 0L;
}

void YahooProtocol::slotIconRightClicked(const QPoint&)
{
	kdDebug() << "YahooProtocol::slotIconRightClicked(<qpoint>)" << endl;

	QString handle = mUsername + "@" + mServer;

	popup = new KPopupMenu(statusBarIcon);
	popup->insertTitle(handle);
	actionGoOnline->plug(popup);
	actionGoOffline->plug(popup);
	actionGoStatus001->plug(popup);
	actionGoStatus002->plug(popup);
	actionGoStatus003->plug(popup);
	actionGoStatus004->plug(popup);
	actionGoStatus005->plug(popup);
	actionGoStatus006->plug(popup);
	actionGoStatus007->plug(popup);
	actionGoStatus008->plug(popup);
	actionGoStatus009->plug(popup);
	actionGoStatus012->plug(popup);
	actionGoStatus099->plug(popup);
	actionGoStatus999->plug(popup);
	popup->popup(QCursor::pos());
}

void YahooProtocol::slotSettingsChanged()
{
	kdDebug() << "YahooProtocol::slotSettingsChanged()" <<endl;
	mUsername = KGlobal::config()->readEntry("UserID", "");
	mPassword = KGlobal::config()->readEntry("Password", "");
	mServer   = KGlobal::config()->readEntry("Server", "cs.yahoo.com");
	mPort     = KGlobal::config()->readNumEntry("Port", 5050);
}

void YahooProtocol::slotConnected()
{
	kdDebug() << "Yahoo: WE ARE CONNECTED YAY!!!!!!" << endl;
}

void YahooProtocol::slotGoOffline()
{
	Disconnect();
}

void YahooProtocol::initIcons()
{
	kdDebug() << "YahooProtocol::initIcons()" << endl;
	KIconLoader *loader = KGlobal::iconLoader();
	KStandardDirs dir;
	onlineIcon  = QPixmap(loader->loadIcon("yahoo_online",  KIcon::User));
	offlineIcon = QPixmap(loader->loadIcon("yahoo_offline", KIcon::User));
	busyIcon    = QPixmap(loader->loadIcon("yahoo_busy",    KIcon::User));
	idleIcon    = QPixmap(loader->loadIcon("yahoo_idle",    KIcon::User));
	mobileIcon  = QPixmap(loader->loadIcon("yahoo_mobile",  KIcon::User));
}

void YahooProtocol::initActions()
{
	kdDebug() << "YahooProtocol::initActions()" << endl;

	actionGoOnline = new KAction(YSTAvailable, "yahoo_online",
				0, this, SLOT(Connect()), this, "actionYahooConnect");
	actionGoOffline = new KAction(i18n("Offline"), "yahoo_offline",
				0, this, SLOT(Disconnect()), this, "actionYahooDisconnect");
	actionGoStatus001 = new KAction(YSTBeRightBack, "yahoo_busy",
				0, this, SLOT(Connect()), this, "actionYahooConnect");
	actionGoStatus002 = new KAction(YSTBusy, "yahoo_busy",
				0, this, SLOT(Connect()), this, "actionYahooConnect");
	actionGoStatus003 = new KAction(YSTNotAtHome, "yahoo_busy",
				0, this, SLOT(Connect()), this, "actionYahooConnect");
	actionGoStatus004 = new KAction(YSTNotAtMyDesk, "yahoo_busy",
				0, this, SLOT(Connect()), this, "actionYahooConnect");
	actionGoStatus005 = new KAction(YSTNotInTheOffice, "yahoo_busy",
				0, this, SLOT(Connect()), this, "actionYahooConnect");
	actionGoStatus006 = new KAction(YSTOnThePhone, "yahoo_busy",
				0, this, SLOT(Connect()), this, "actionYahooConnect");
	actionGoStatus007 = new KAction(YSTOnVacation, "yahoo_busy",
				0, this, SLOT(Connect()), this, "actionYahooConnect");
	actionGoStatus008 = new KAction(YSTOutToLunch, "yahoo_busy",
				0, this, SLOT(Connect()), this, "actionYahooConnect");
	actionGoStatus009 = new KAction(YSTSteppedOut, "yahoo_busy",
				0, this, SLOT(Connect()), this, "actionYahooConnect");
	actionGoStatus012 = new KAction(i18n("Invisible"), "yahoo_offline",
				0, this, SLOT(Connect()), this, "actionYahooConnect"); // XXX Connect with invisible on
	actionGoStatus099 = new KAction(i18n("Custom"), "yahoo_online",
				0, this, SLOT(Connect()), this, "actionYahooConnect"); // XXX Get some dialogbox
	actionGoStatus999 = new KAction(YSTIdle, "yahoo_idle",
				0, this, SLOT(Connect()), this, "actionYahooConnect");

	actionStatusMenu = new KActionMenu("Yahoo", this);
	actionStatusMenu->insert(actionGoOnline);
	actionStatusMenu->insert(actionGoOffline);
	actionStatusMenu->insert(actionGoStatus001);
	actionStatusMenu->insert(actionGoStatus002);
	actionStatusMenu->insert(actionGoStatus003);
	actionStatusMenu->insert(actionGoStatus004);
	actionStatusMenu->insert(actionGoStatus005);
	actionStatusMenu->insert(actionGoStatus006);
	actionStatusMenu->insert(actionGoStatus007);
	actionStatusMenu->insert(actionGoStatus008);
	actionStatusMenu->insert(actionGoStatus009);
	actionStatusMenu->insert(actionGoStatus012);
	actionStatusMenu->insert(actionGoStatus099);
	actionStatusMenu->insert(actionGoStatus999);
	actionStatusMenu->plug(kopeteapp->systemTray()->contextMenu(), 1);
}


/***************************************************************************
 *                                                                         *
 *   Slot for KYahoo signals                                               *
 *                                                                         *
 ***************************************************************************/

void YahooProtocol::slotLoginResponse( int succ, const QString &url)
{
	kdDebug() << "[YahooProtocol::slotLoginResponse]" << endl;
}

void YahooProtocol::slotGotBuddies(YList * buds)
{
	kdDebug() << "[YahooProtocol::slotGotBuddies]" << endl;
}

void YahooProtocol::slotGotBuddy(const QString &userid, const QString &alias, const QString &group)
{
	kdDebug() << "[YahooProtocol::slotGotBuddy]" << endl;
	addContact(userid, alias, group, 0L);
}

void YahooProtocol::addContact(const QString &userid, const QString &alias, const QString &group, KopeteMetaContact *metaContact)
{
	kdDebug() << "[YahooProtocol::addContact]" << endl;

	if ( ! m_contactsMap.contains(userid) )
	{
		kdDebug() << "[YahooProtocol::addContact] Got new buddy " << userid << endl;
		
		if ( metaContact )
		{
			/* Ok they are forcing us to use this metacontact */
		
		}
		else
		{
			/* They didn't provide us a metaContact, we will try to get one */
			metaContact = KopeteContactList::contactList()->findContact( id(), QString::null, userid );
			
			if ( metaContact )
			{
				/* Ok there was a metacontact containing us */
			}
			else
			{
				metaContact = new KopeteMetaContact();
				
				if ( alias != QString::null )
					metaContact->setDisplayName( alias );
				else
					metaContact->setDisplayName( userid );	
			}
		}
		
		/* Ok lets create the contact now */
		YahooContact *contact = new YahooContact( userid, alias, group, this, metaContact);
		m_contactsMap[userid] = contact;
		metaContact->addContact(contact);
		KopeteContactList::contactList()->addMetaContact(metaContact);	
	}
	else
	{
		kdDebug() << "[YahooProtocol::addContact] Buddy already exists" << endl;	
	}
}

void YahooProtocol::slotGotIgnore( YList * igns)
{
	kdDebug() << "[YahooProtocol::slotGotIgnore]" << endl;
}

void YahooProtocol::slotGotIdentities( const QStringList &ids)
{
	kdDebug() << "[YahooProtocol::slotGotIdentities]" << endl;
	
}

void YahooProtocol::slotStatusChanged( const QString &who, int stat, const QString &msg, int away)
{
	kdDebug() << "[YahooProtocol::slotStatusChanged]" << endl;
}

void YahooProtocol::slotGotIm( const QString &who, const QString &msg, long tm, int stat)
{
	kdDebug() << "[YahooProtocol::slotGotIm]" << endl;
}

void YahooProtocol::slotGotConfInvite( const QString &who, const QString &room, const QString &msg, const QStringList &members)
{
	kdDebug() << "[YahooProtocol::slotGotConfInvite]" << endl;
}

void YahooProtocol::slotConfUserDecline( const QString &who, const QString &room, const QString &msg)
{
	kdDebug() << "[YahooProtocol::slotConfUserDecline]" << endl;
}

void YahooProtocol::slotConfUserJoin( const QString &who, const QString &room)
{
	kdDebug() << "[YahooProtocol::slotConfUserJoin]" << endl;
}

void YahooProtocol::slotConfUserLeave( const QString &who, const QString &room)
{
	kdDebug() << "[YahooProtocol::slotConfUserLeave]" << endl;
}

void YahooProtocol::slotConfMessage( const QString &who, const QString &room, const QString &msg)
{
	kdDebug() << "[YahooProtocol::slotConfMessage]" << endl;
}

void YahooProtocol::slotGotFile( const QString &who, const QString &url, long expires, const QString &msg, const QString &fname, unsigned long fesize)
{
	kdDebug() << "[YahooProtocol::slotGotFile]" << endl;
}

void YahooProtocol::slotContactAdded( const QString &myid, const QString &who, const QString &msg)
{
	kdDebug() << "[YahooProtocol::slotContactAdded]" << endl;
}

void YahooProtocol::slotRejected( const QString &who, const QString &msg)
{
	kdDebug() << "[YahooProtocol::slotRejected]" << endl;
}

void YahooProtocol::slotTypingNotify( const QString &who, int stat)
{
	kdDebug() << "[YahooProtocol::slotTypingNotify]" << endl;
}

void YahooProtocol::slotGameNotify( const QString &who, int stat)
{
	kdDebug() << "[YahooProtocol::slotGameNotify]" << endl;
}

void YahooProtocol::slotMailNotify( const QString &from, const QString &subj, int cnt)
{
	kdDebug() << "[YahooProtocol::slotMailNotify]" << endl;
}

void YahooProtocol::slotSystemMessage( const QString &msg)
{
	kdDebug() << "[YahooProtocol::slotSystemMessage]" << msg << endl;
}

void YahooProtocol::slotError( const QString &err, int fatal)
{
	kdDebug() << "[YahooProtocol::slotError]" << endl;
}

void YahooProtocol::slotRemoveHandler( int fd)
{
	kdDebug() << "[YahooProtocol::slotRemoveHandler]" << endl;
}

#include "yahooprotocol.moc"

/*
 * Local variables:
 * c-indentation-style: k&r
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 */
// vim: set noet ts=4 sts=4 sw=4:

