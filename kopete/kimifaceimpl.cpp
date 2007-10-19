/*
    kimifaceimpl.cpp - Kopete DCOP Interface

    Copyright (c) 2004 by Will Stephenson     <wstephenson@kde.org>

    Kopete    (c) 2002-2004      by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include <qstringlist.h>
//Added by qt3to4:
#include <QPixmap>

#include <dcopclient.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kplugininfo.h>
#include <kabc/addressbook.h>
#include <kabc/stdaddressbook.h>

#include "kopeteaccount.h"
#include "kopeteaccountmanager.h"
#include "kopetecontactlist.h"
#include "kopetechatsession.h"
#include "kopetemetacontact.h"
#include "kopeteprotocol.h"
#include "kopetepluginmanager.h"
#include "kopeteuiglobal.h"
#include "kopetecontact.h"

#include <kdebug.h>

#include "kimifaceimpl.h"

KIMIfaceImpl::KIMIfaceImpl() : DCOPObject( "KIMIface" ), QObject()
{
	connect( Kopete::ContactList::self(),
		SIGNAL( metaContactAdded( Kopete::MetaContact * ) ),
		SLOT( slotMetaContactAdded( Kopete::MetaContact * ) ) );
}

KIMIfaceImpl::~KIMIfaceImpl()
{
}

QStringList KIMIfaceImpl::allContacts()
{
	QStringList result;
	QList<Kopete::MetaContact*> list = Kopete::ContactList::self()->metaContacts();
	QList<Kopete::MetaContact*>::iterator it;
	for( it = list.begin(); it != list.end(); ++it )
	{
		if ( !(*it)->metaContactId().contains(':') )
			result.append( (*it)->metaContactId() );
	}

	return result;
}

QStringList KIMIfaceImpl::reachableContacts()
{
	QStringList result;
	QList<Kopete::MetaContact*> list = Kopete::ContactList::self()->metaContacts();
	QList<Kopete::MetaContact*>::iterator it;
	for( it = list.begin(); it != list.end(); ++it )
	{
		if ( (*it)->isReachable() && !(*it)->metaContactId().contains(':') )
			result.append( (*it)->metaContactId() );
	}

	return result;
}

QStringList KIMIfaceImpl::onlineContacts()
{
	QStringList result;
	QList<Kopete::MetaContact*> list = Kopete::ContactList::self()->metaContacts();
	QList<Kopete::MetaContact*>::iterator it;
	for( it = list.begin(); it != list.end(); ++it )
	{
		if ( (*it)->isOnline() && !(*it)->metaContactId().contains(':') )
			result.append( (*it)->metaContactId() );
	}

	return result;
}

QStringList KIMIfaceImpl::fileTransferContacts()
{
	QStringList result;
	QList<Kopete::MetaContact*> list = Kopete::ContactList::self()->metaContacts();
	QList<Kopete::MetaContact*>::iterator it;
	for( it = list.begin(); it != list.end(); ++it )
	{
		if ( (*it)->canAcceptFiles() && !(*it)->metaContactId().contains(':') )
			result.append( (*it)->metaContactId() );
	}

	return result;
}

bool KIMIfaceImpl::isPresent( const QString & uid )
{
	Kopete::MetaContact *mc;
	mc = Kopete::ContactList::self()->metaContact( uid );

	return ( mc != 0 );
}


QString KIMIfaceImpl::displayName( const QString & uid )
{
	Kopete::MetaContact *mc;
	mc = Kopete::ContactList::self()->metaContact( uid );
	QString name;
	if ( mc )
		name = mc->displayName();
	
	return name;
}

int KIMIfaceImpl::presenceStatus( const QString & uid )
{
	//kDebug( 14000 ) ;
	int p = -1;
	Kopete::MetaContact *m = Kopete::ContactList::self()->metaContact( uid );
	if ( m )
	{
		Kopete::OnlineStatus status = m->status();
		switch ( status.status() )
		{
			case Kopete::OnlineStatus::Unknown:
				p = 0;
			break;
			case Kopete::OnlineStatus::Offline:
			case Kopete::OnlineStatus::Invisible:
				p = 1;
			break;
			case Kopete::OnlineStatus::Connecting:
				p = 2;
			break;
			case Kopete::OnlineStatus::Away:
				p = 3;
			break;
			case Kopete::OnlineStatus::Online:
				p = 4;
			break;
		}
	}
	return p;
}

QString KIMIfaceImpl::presenceString( const QString & uid )
{
	//kDebug( 14000 ) <<  "KIMIfaceImpl::presenceString";
	QString p;
	Kopete::MetaContact *m = Kopete::ContactList::self()->metaContact( uid );
	if ( m )
	{
		Kopete::OnlineStatus status = m->status();
			p = status.description();
		kDebug( 14000 ) << "Got presence for " <<  uid << " : " << p.toAscii();
	}
	else
	{
		kDebug( 14000 ) << "Couldn't find MC: " << uid;;
		p = QString();
	}
	return p;
}

bool KIMIfaceImpl::canReceiveFiles( const QString & uid )
{
	Kopete::MetaContact *mc;
	mc = Kopete::ContactList::self()->metaContact( uid );

	if ( mc )
		return mc->canAcceptFiles();
	else
		return false;
}

bool KIMIfaceImpl::canRespond( const QString & uid )
{
	Kopete::MetaContact *mc;
	mc = Kopete::ContactList::self()->metaContact( uid );

	if ( mc )
	{
		QList<Kopete::Contact*> list = mc->contacts();
		QList<Kopete::Contact*>::iterator it;
		for ( it = list.begin(); it != list.end(); ++it )
		{
			Kopete::Contact *contact = (*it);
			if ( contact->isOnline() && contact->protocol()->pluginId() != "SMSProtocol" )
				return true;
		}
	}
	return false;
}

QString KIMIfaceImpl::locate( const QString & contactId, const QString & protocolId )
{
	Kopete::MetaContact *mc = locateProtocolContact( contactId, protocolId );
	if ( mc )
		return mc->metaContactId();
	else
		return QString();
}

Kopete::MetaContact * KIMIfaceImpl::locateProtocolContact( const QString & contactId, const QString & protocolId )
{
	foreach( Kopete::Account *ac , Kopete::AccountManager::self()->accounts() )
	{
		if( ac->protocol()->pluginId() == protocolId )
		{
			if( ac->contacts().contains(contactId) )
			{
				Kopete::Contact *c=ac->contacts()[contactId];
				if(c)
					return c->metaContact();
			}
		}
	}
	return 0;
}

QPixmap KIMIfaceImpl::icon( const QString & uid )
{
	Kopete::MetaContact *m = Kopete::ContactList::self()->metaContact( uid );
	QPixmap p;
	if ( m )
		p = SmallIcon( m->statusIcon() );
	return p;
}

QString KIMIfaceImpl::context( const QString & uid )
{
	// TODO: support context
	// shush warning
	QString myUid = uid;

	return QString();
}

QStringList KIMIfaceImpl::protocols()
{
	QList<KPluginInfo *> protocols = Kopete::PluginManager::self()->availablePlugins( "Protocols" );
	QStringList protocolList;
	for ( QList<KPluginInfo *>::Iterator it = protocols.begin(); it != protocols.end(); ++it )
		protocolList.append( (*it)->name() );

	return protocolList;
}

void KIMIfaceImpl::messageContact( const QString &uid, const QString& messageText )
{
	Kopete::MetaContact *m = Kopete::ContactList::self()->metaContact( uid );
	if ( m )
	{
		Kopete::Contact * c = m->preferredContact();
		Kopete::ChatSession * manager = c->manager(Kopete::Contact::CanCreate);
		c->manager( Kopete::Contact::CanCreate )->view( true );
		Kopete::Message msg = Kopete::Message( manager->myself(), manager->members(), messageText,
				Kopete::Message::Outbound, Kopete::Message::PlainText);
		manager->sendMessage( msg );
	}
	else
		unknown( uid );
}

void KIMIfaceImpl::messageNewContact( const QString &contactId, const QString &protocol )
{
	Kopete::MetaContact *mc = locateProtocolContact( contactId, protocol );
	if ( mc )
		mc->sendMessage();
}

void KIMIfaceImpl::chatWithContact( const QString &uid )
{
	Kopete::MetaContact *m = Kopete::ContactList::self()->metaContact( uid );
	if ( m )
		m->execute();
	else
		unknown( uid );
}

void KIMIfaceImpl::sendFile(const QString &uid, const KUrl &sourceURL,
		const QString &altFileName, uint fileSize)
{
	Kopete::MetaContact *m = Kopete::ContactList::self()->metaContact( uid );
	if ( m )
		m->sendFile( sourceURL, altFileName, fileSize );
    // else, prompt to create a new MC associated with UID
}

bool KIMIfaceImpl::addContact( const QString &contactId, const QString &protocolId )
{
	// find its accounts
	foreach( Kopete::Account *ac , Kopete::AccountManager::self()->accounts() )
	{
		if( ac->protocol()->pluginId() == protocolId )
		{
			return ac->addContact( contactId );
		}
	}
	return false;
}

void KIMIfaceImpl::slotMetaContactAdded( Kopete::MetaContact *mc )
{
	connect( mc, SIGNAL( onlineStatusChanged( Kopete::MetaContact *, Kopete::OnlineStatus::StatusType ) ),
		SLOT( slotContactStatusChanged( Kopete::MetaContact * ) ) );
}

void KIMIfaceImpl::slotContactStatusChanged( Kopete::MetaContact *mc )
{
	if ( !mc->metaContactId().contains( ':' ) )
	{
		int p = -1;
		Kopete::OnlineStatus status = mc->status();
		switch ( status.status() )
		{
			case Kopete::OnlineStatus::Unknown:
				p = 0;
			break;
			case Kopete::OnlineStatus::Offline:
			case Kopete::OnlineStatus::Invisible:
				p = 1;
			break;
			case Kopete::OnlineStatus::Connecting:
				p = 2;
			break;
			case Kopete::OnlineStatus::Away:
				p = 3;
			break;
			case Kopete::OnlineStatus::Online:
				p = 4;
			break;
		}
		// tell anyone who's listening over DCOP
		contactPresenceChanged( mc->metaContactId(), kapp->name(), p );
/*		QByteArray params;
		QDataStream stream( &params,QIODevice::WriteOnly);
		stream.setVersion(QDataStream::Qt_3_1);
		stream << mc->metaContactId();
		stream << kapp->name();
		stream << p;
		kapp->dcopClient()->emitDCOPSignal( "contactPresenceChanged( QString, QCString, int )", params );*/
	}
}

void KIMIfaceImpl::unknown( const QString &uid )
{
	// warn the user that the KABC contact associated with this UID isn't known to kopete,
	// either associate an existing contact with KABC or add a new one using the ACW.
	KABC::AddressBook *bk = KABC::StdAddressBook::self( false );
	KABC::Addressee addr = bk->findByUid( uid );
	if ( addr.isEmpty() )
	{
		KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Sorry, i18n("Another KDE application tried to use Kopete for instant messaging, but Kopete could not find the specified contact in the KDE address book."), i18n( "Not Found in Address Book" ) );
	}
	else
	{
		QString apology = i18nc( "Translators: %1 is the name of a person taken from the KDE address book, who Kopete does not know about. Kopete must either be told that an existing contact in Kopete is this person, or add a new contact for them",
			"<qt><p>The KDE Address Book has no instant messaging information for</p><p><b>%1</b>.</p><p>If he/she is already present in the Kopete contact list, indicate the correct addressbook entry in their properties.</p><p>Otherwise, add a new contact using the Add Contact wizard.</p></qt>", addr.realName() );
		KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Information, apology, i18n( "No Instant Messaging Address" ) );
	}
}

#include "kimifaceimpl.moc"

