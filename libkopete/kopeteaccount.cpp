/*
    kopeteaccount.cpp - Kopete Account

    Copyright (c) 2003-2004 by Olivier Goffart       <ogoffart@tiscalinet.be>
    Copyright (c) 2003-2004 by Martijn Klingens      <klingens@kde.org>
    Copyright (c) 2004      by Richard Smith         <kde@metafoo.co.uk>
    Kopete    (c) 2002-2004 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include <qapplication.h>
#include <qtimer.h>

#include <kconfig.h>
#include <kdebug.h>
#include <kdeversion.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kiconeffect.h>

#include "kopetecontactlist.h"
#include "kopeteaccount.h"
#include "kopeteaccountmanager.h"
#include "kopetemetacontact.h"
#include "kopeteprotocol.h"
#include "kopetepluginmanager.h"
#include "kopetegroup.h"
#include "kopetepassword.h"
#include "kopeteprefs.h"
#include "kopeteblacklister.h"

static QString configGroup( Kopete::Protocol *protocol, const QString &accountId )
{
	return QString::fromLatin1( "Account_%2_%1" ).arg( accountId, protocol->pluginId() );
}

class KopeteAccountPrivate
{
public:
	KopeteAccountPrivate( Kopete::Protocol *protocol, const QString &accountId )
	 : protocol( protocol ), id( accountId )
	 , password( configGroup( protocol, accountId ) )
	 , autologin( false ), priority( 0 ), myself( 0 )
	 , suppressStatusTimer( 0 ), suppressStatusNotification( false )
	 , blackList( new Kopete::BlackLister( protocol->pluginId(), accountId ) )
	{
	}
	
	~KopeteAccountPrivate() { delete blackList; }

	Kopete::Protocol *protocol;
	QString id;
	Kopete::Password password;
	bool autologin;
	uint priority;
	QDict<Kopete::Contact> contacts;
	QColor color;
	Kopete::Contact *myself;
	QTimer *suppressStatusTimer;
	bool suppressStatusNotification;
	Kopete::BlackLister *blackList;
};

Kopete::Account::Account( Kopete::Protocol *parent, const QString &accountId, const char *name )
 : Kopete::PluginDataObject( parent, name ), d( new KopeteAccountPrivate( parent, accountId ) )
{
	d->suppressStatusTimer = new QTimer( this, "suppressStatusTimer" );
	QObject::connect( d->suppressStatusTimer, SIGNAL( timeout() ),
		this, SLOT( slotStopSuppression() ) );

	if ( Kopete::AccountManager::manager()->registerAccount( this ) )
		QTimer::singleShot( 0, this, SLOT( slotAccountReady() ) );
	else
		deleteLater();
}

Kopete::Account::~Account()
{
	// Delete all registered child contacts first
	while ( !d->contacts.isEmpty() )
		delete *QDictIterator<Kopete::Contact>( d->contacts );
	Kopete::AccountManager::manager()->unregisterAccount( this );

	delete d;
}

void Kopete::Account::slotAccountReady()
{
	Kopete::AccountManager::manager()->notifyAccountReady( this );
}

void Kopete::Account::connect( const Kopete::OnlineStatus& )
{
	//do nothing
}

void Kopete::Account::disconnect( DisconnectReason reason )
{
	//reconnect if needed
	if ( KopetePrefs::prefs()->reconnectOnDisconnect() == true && reason > Manual )
		//use a timer to allow the plugins to clean up after return
		QTimer::singleShot(0, this, SLOT(connect()));
}

Kopete::Protocol *Kopete::Account::protocol() const
{
	return d->protocol;
}

QString Kopete::Account::accountId() const
{
	return d->id;
}

const QColor Kopete::Account::color() const
{
	return d->color;
}

void Kopete::Account::setColor( const QColor &color )
{
	d->color = color;
	emit colorChanged( color );
}

void Kopete::Account::setPriority( uint priority )
{
 	d->priority = priority;
}

const uint Kopete::Account::priority() const
{
	return d->priority;
}

void Kopete::Account::setAccountId( const QString &accountId )
{
	if ( d->id != accountId )
	{
		d->id = accountId;
		emit ( accountIdChanged() );
	}
}

QPixmap Kopete::Account::accountIcon(const int size) const
{
	// FIXME: this code is duplicated with Kopete::OnlineStatus, can we merge it somehow?
	QPixmap base = KGlobal::instance()->iconLoader()->loadIcon(
		d->protocol->pluginIcon(), KIcon::Small, size );

	if ( d->color.isValid() )
	{
		KIconEffect effect;
		base = effect.apply( base, KIconEffect::Colorize, 1, d->color, 0);
	}

	if ( size > 0 && base.width() != size )
	{
		base = QPixmap( base.convertToImage().smoothScale( size, size ) );
	}

	return base;
}

QString Kopete::Account::configGroup() const
{
	return ::configGroup( protocol(), accountId() );
}

void Kopete::Account::writeConfig( const QString &configGroupName )
{
	KConfig *config = KGlobal::config();
	config->setGroup( configGroupName );

	config->writeEntry( "Protocol", d->protocol->pluginId() );
	config->writeEntry( "AccountId", d->id );
	config->writeEntry( "Priority", d->priority );

	config->writeEntry( "AutoConnect", d->autologin );

	if ( d->color.isValid() )
		config->writeEntry( "Color", d->color );
	else
		config->deleteEntry( "Color" );

	// Store other plugin data
	Kopete::PluginDataObject::writeConfig( configGroupName );
}

void Kopete::Account::readConfig( const QString &configGroupName )
{
	KConfig *config = KGlobal::config();
	config->setGroup( configGroupName );

	d->autologin = config->readBoolEntry( "AutoConnect", false );
	d->color = config->readColorEntry( "Color", &d->color );
	d->priority = config->readNumEntry( "Priority", 0 );

	// Handle the plugin data, if any
	QMap<QString, QString> entries = config->entryMap( configGroupName );
	QMap<QString, QString>::Iterator entryIt;
	QMap<QString, QMap<QString, QString> > pluginData;
	for ( entryIt = entries.begin(); entryIt != entries.end(); ++entryIt )
	{
		if ( entryIt.key().startsWith( QString::fromLatin1( "PluginData_" ) ) )
		{
			QStringList data = QStringList::split( '_', entryIt.key(), true );
			data.pop_front(); // Strip "PluginData_" first
			QString pluginName = data.first();
			data.pop_front();

			// Join remainder and store it
			pluginData[ pluginName ][ data.join( QString::fromLatin1( "_" ) ) ] = entryIt.data();
		}
	}

	// Lastly, pass on the plugin data to the account
	QMap<QString, QMap<QString, QString> >::Iterator pluginDataIt;
	for ( pluginDataIt = pluginData.begin(); pluginDataIt != pluginData.end(); ++pluginDataIt )
	{
		Kopete::Plugin *plugin = Kopete::PluginManager::self()->plugin( pluginDataIt.key() );
		if ( plugin )
			setPluginData( plugin, pluginDataIt.data() );
		else
		{
			kdDebug( 14010 ) << k_funcinfo <<
				"No plugin object for id '" << pluginDataIt.key() << "'" << endl;
		}
	}
	loaded();
}

void Kopete::Account::loaded()
{
	/* do nothing in default implementation */
}

QString Kopete::Account::password( bool error, bool *ok, unsigned int maxLength ) const
{
	d->password.setMaximumLength( maxLength );
	QString prompt;
	if ( error )
	{
		prompt = i18n( "<b>The password was wrong;</b> please re-enter your"\
			" password for %1 account <b>%2</b>" ).arg( protocol()->displayName(),
				accountId() );
	}
	else
	{
		prompt = i18n( "Please enter your password for %1 account <b>%2</b>" )
			.arg( protocol()->displayName(), accountId() );
	}

	QString pass = d->password.retrieve(
		accountIcon( Kopete::Password::preferredImageSize() ), prompt,
		error ? Kopete::Password::FromUser : Kopete::Password::FromConfigOrUser );

	if ( ok )
		*ok = !pass.isNull();
	return pass;
}

void Kopete::Account::setPassword( const QString &pass )
{
	d->password.set( pass );
}

void Kopete::Account::setAutoLogin( bool b )
{
	d->autologin = b;
}

bool Kopete::Account::autoLogin() const
{
	return d->autologin;
}

bool Kopete::Account::rememberPassword() const
{
	return d->password.remembered();
}

void Kopete::Account::registerContact( Kopete::Contact *c )
{
	d->contacts.insert( c->contactId(), c );
	QObject::connect( c, SIGNAL( contactDestroyed( Kopete::Contact * ) ),
		SLOT( slotKopeteContactDestroyed( Kopete::Contact * ) ) );
}

void Kopete::Account::slotKopeteContactDestroyed( Kopete::Contact *c )
{
	//kdDebug( 14010 ) << "Kopete::Protocol::slotKopeteContactDestroyed: " << c->contactId() << endl;
	d->contacts.remove( c->contactId() );
}

const QDict<Kopete::Contact>& Kopete::Account::contacts()
{
	return d->contacts;
}


bool Kopete::Account::addContact( const QString &contactId, const QString &displayName,
	Kopete::MetaContact *parentContact, const AddMode mode, const QString &groupName, bool isTemporary )
{
	if ( contactId == accountId() )
	{
		kdDebug( 14010 ) << k_funcinfo <<
			"WARNING: the user try to add myself to his contactlist - abort" << endl;
		return false;
	}

	Kopete::Group *parentGroup = 0L;
	//If this is a temporary contact, use the temporary group
	if ( !groupName.isNull() )
		parentGroup = isTemporary ? Kopete::Group::temporary() : Kopete::ContactList::contactList()->getGroup( groupName );

	if(!parentGroup && parentContact)
		parentGroup=parentContact->groups().first();


	Kopete::Contact *c = d->contacts[ contactId ];
	if ( c && c->metaContact() )
	{
		if ( c->metaContact()->isTemporary() && !isTemporary )
		{
			kdDebug( 14010 ) <<
				"Kopete::Account::addContact: You are trying to add an existing temporary contact. Just add it on the list" << endl;
			/* //FIXME: calling this can produce a message to delete the old contazct which should be deleted in many case.
			if(c->metaContact() != parentContact)
				c->setMetaContact(parentContact);*/

			c->metaContact()->setTemporary(false, parentGroup );
			if(!Kopete::ContactList::contactList()->metaContacts().contains(c->metaContact()))
				Kopete::ContactList::contactList()->addMetaContact(c->metaContact());
		}
		else
		{
			// should we here add the contact to the parentContact if any?
			kdDebug( 14010 ) << "Kopete::Account::addContact: Contact already exists" << endl;
		}
		return false; //(the contact is not in the correct metacontact, so false)
	}

	if ( parentContact )
	{
		//If we are given a MetaContact to add to that is marked as temporary. but
		//this contact is not temporary, then change the metacontact to non-temporary
		if ( parentContact->isTemporary() && !isTemporary )
			parentContact->setTemporary( false, parentGroup );
		else
			parentContact->addToGroup( parentGroup );
	}
	else
	{
		//Create a new MetaContact
		parentContact = new Kopete::MetaContact();
		//parentContact->setDisplayName( displayName );

		//Set it as a temporary contact if requested
		if ( isTemporary )
			parentContact->setTemporary( true );
		else
			parentContact->addToGroup( parentGroup );

		Kopete::ContactList::contactList()->addMetaContact( parentContact );
	}

	//Fix for 77835 (forward the metacontact name is displayName is empty)
	QString realDisplayName;
	if ( displayName.isEmpty() )
	{
		realDisplayName = parentContact->displayName();
	}
	else
	{
		realDisplayName = displayName;
	}

	if ( c )
	{
		c->setMetaContact( parentContact );
	}
	else
	{
		if ( !addContactToMetaContact( contactId, realDisplayName, parentContact ) )
			return false;
	}

	if ( mode == ChangeKABC )
	{
		kdDebug( 14010 ) << k_funcinfo << " changing KABC" << endl;
		parentContact->updateKABC();
	}
	/*else
		kdDebug( 14010 ) << k_funcinfo << " leaving KABC" << endl;*/
	return true;
}

KActionMenu * Kopete::Account::actionMenu()
{
	//default implementation
	return 0L;
}

bool Kopete::Account::isConnected() const
{
	return d->myself && ( d->myself->onlineStatus().status() != Kopete::OnlineStatus::Offline ) ;
}

bool Kopete::Account::isAway() const
{
	return d->myself && ( d->myself->onlineStatus().status() == Kopete::OnlineStatus::Away );
}

Kopete::Contact * Kopete::Account::myself() const
{
	return d->myself;
}

void Kopete::Account::setMyself( Kopete::Contact *myself )
{
	d->myself = myself;

	QObject::connect( myself, SIGNAL( onlineStatusChanged( Kopete::Contact *, const Kopete::OnlineStatus &, const Kopete::OnlineStatus & ) ),
		this, SLOT( slotOnlineStatusChanged( Kopete::Contact *, const Kopete::OnlineStatus &, const Kopete::OnlineStatus & ) ) );
}

void Kopete::Account::slotOnlineStatusChanged( Kopete::Contact * /* contact */,
	const Kopete::OnlineStatus &newStatus, const Kopete::OnlineStatus &oldStatus )
{
	if ( oldStatus.status() == Kopete::OnlineStatus::Offline ||
		oldStatus.status() == Kopete::OnlineStatus::Connecting ||
		newStatus.status() == Kopete::OnlineStatus::Offline )
	{
		// Wait for five seconds until we treat status notifications for contacts
		// as unrelated to our own status change.
		// Five seconds may seem like a long time, but just after your own
		// connection it's basically neglectible, and depending on your own
		// contact list's size, the protocol you are using, your internet
		// connection's speed and your computer's speed you *will* need it.
		d->suppressStatusNotification = true;
		d->suppressStatusTimer->start( 5000, true );
	}
}

void Kopete::Account::slotStopSuppression()
{
	d->suppressStatusNotification = false;
}

bool Kopete::Account::suppressStatusNotification() const
{
	return d->suppressStatusNotification;
}

Kopete::BlackLister* Kopete::Account::blackLister()
{
	return d->blackList;
}

void Kopete::Account::block( QString &contactId )
{
	d->blackList->slotAddContact( contactId );
}

void Kopete::Account::unblock( QString &contactId )
{
	d->blackList->slotRemoveContact( contactId );
}

bool Kopete::Account::isBlocked( QString &contactId )
{
	return d->blackList->isBlocked( contactId );
}

#include "kopeteaccount.moc"
// vim: set noet ts=4 sts=4 sw=4:
