/*
  icqaccount.cpp  -  ICQ Account Class

  Copyright (c) 2002 by Chris TenHarmsel            <tenharmsel@staticmethod.net>
  Copyright (c) 2004 by Richard Smith               <kde@metafoo.co.uk>
  Kopete    (c) 2002-2004 by the Kopete developers  <kopete-devel@kde.org>

  *************************************************************************
  *                                                                       *
  * This program is free software; you can redistribute it and/or modify  *
  * it under the terms of the GNU General Public License as published by  *
  * the Free Software Foundation; either version 2 of the License, or     *
  * (at your option) any later version.                                   *
  *                                                                       *
  *************************************************************************
*/

#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmenu.h>
#include <kmessagebox.h>

#include "kopeteawayaction.h"
#include "kopetemessage.h"
#include "kopetecontactlist.h"
#include "kopeteuiglobal.h"

#include "client.h"
#include "icquserinfo.h"
#include "oscarsettings.h"
#include "oscarutils.h"
#include "contactmanager.h"

#include "icqcontact.h"
#include "icqprotocol.h"
#include "icqaccount.h"

ICQMyselfContact::ICQMyselfContact( ICQAccount *acct ) : OscarMyselfContact( acct )
{
	QObject::connect( acct->engine(), SIGNAL( loggedIn() ), this, SLOT( fetchShortInfo() ) );
	QObject::connect( acct->engine(), SIGNAL( receivedIcqShortInfo( const QString& ) ),
	                  this, SLOT( receivedShortInfo( const QString& ) ) );
}

void ICQMyselfContact::userInfoUpdated()
{
	DWORD extendedStatus = details().extendedStatus();
	kDebug( OSCAR_ICQ_DEBUG ) << k_funcinfo << "extendedStatus is " << QString::number( extendedStatus, 16 ) << endl;
	ICQ::Presence presence = ICQ::Presence::fromOscarStatus( extendedStatus & 0xffff );
	setOnlineStatus( presence.toOnlineStatus() );
	setProperty( Kopete::Global::Properties::self()->statusMessage(), static_cast<ICQAccount*>( account() )->engine()->statusMessage() );
}

void ICQMyselfContact::receivedShortInfo( const QString& contact )
{
	if ( Oscar::normalize( contact ) != Oscar::normalize( contactId() ) )
		return;

	ICQShortInfo shortInfo = static_cast<ICQAccount*>( account() )->engine()->getShortInfo( contact );
	if ( !shortInfo.nickname.isEmpty() )
	{
		setProperty( Kopete::Global::Properties::self()->nickName(), static_cast<ICQAccount*>( account() )->defaultCodec()->toUnicode( shortInfo.nickname ) );
	}
}

void ICQMyselfContact::fetchShortInfo()
{
	static_cast<ICQAccount*>( account() )->engine()->requestShortInfo( contactId() );
}

ICQAccount::ICQAccount(Kopete::Protocol *parent, QString accountID)
	: OscarAccount(parent, accountID, true)
{
	kDebug(14152) << k_funcinfo << accountID << ": Called."<< endl;
	setMyself( new ICQMyselfContact( this ) );
	myself()->setOnlineStatus( ICQ::Presence( ICQ::Presence::Offline, ICQ::Presence::Visible ).toOnlineStatus() );

	QString nickName = configGroup()->readEntry("NickName", QString() );
	mWebAware = configGroup()->readEntry( "WebAware", false );
	mHideIP = configGroup()->readEntry( "HideIP", true );
	mInitialStatusMessage.clear();

	//setIgnoreUnknownContacts(pluginData(protocol(), "IgnoreUnknownContacts").toUInt() == 1);

	/* FIXME: need to do this when web aware or hide ip change
	if(isConnected() && (oldhideip != mHideIP || oldwebaware != mWebAware))
	{
		kDebug(14153) << k_funcinfo <<
			"sending status to reflect HideIP and WebAware settings" << endl;
		//setStatus(mStatus, QString::null);
	}*/
}

ICQAccount::~ICQAccount()
{
}

ICQProtocol* ICQAccount::protocol()
{
	return static_cast<ICQProtocol*>(OscarAccount::protocol());
}


ICQ::Presence ICQAccount::presence()
{
	return ICQ::Presence::fromOnlineStatus( myself()->onlineStatus() );
}


KActionMenu* ICQAccount::actionMenu()
{
	KActionMenu* actionMenu = Kopete::Account::actionMenu();

	actionMenu->addSeparator();

	/*	KToggleAction* actionInvisible =
	    new KToggleAction( i18n( "In&visible" ),
	                       ICQ::Presence( presence().type(), ICQ::Presence::Invisible ).toOnlineStatus().iconFor( this ),
	                       0, this, SLOT( slotToggleInvisible() ), this );
	actionInvisible->setChecked( presence().visibility() == ICQ::Presence::Invisible );
	actionMenu->insert( actionInvisible );
    
	actionMenu->popupMenu()->insertSeparator();
	//actionMenu->insert( new KToggleAction( i18n( "Send &SMS..." ), 0, 0, this, SLOT( slotSendSMS() ), this, "ICQAccount::mActionSendSMS") );
	*/
	return actionMenu;
}


void ICQAccount::connectWithPassword( const QString &password )
{
	if ( password.isNull() )
		return;

	kDebug(14153) << k_funcinfo << "accountId='" << accountId() << "'" << endl;

	Kopete::OnlineStatus status = initialStatus();
	if ( status == Kopete::OnlineStatus() &&
	     status.status() == Kopete::OnlineStatus::Unknown )
		//use default online in case of invalid online status for connecting
		status = Kopete::OnlineStatus( Kopete::OnlineStatus::Online );
	ICQ::Presence pres = ICQ::Presence::fromOnlineStatus( status );
	bool accountIsOffline = ( presence().type() == ICQ::Presence::Offline ||
	                          myself()->onlineStatus() == protocol()->statusManager()->connectingStatus() );

	if ( accountIsOffline )
	{
		myself()->setOnlineStatus( protocol()->statusManager()->connectingStatus() );
		QString icqNumber = accountId();
		kDebug(14153) << k_funcinfo << "Logging in as " << icqNumber << endl ;
		QString server = configGroup()->readEntry( "Server", QString::fromLatin1( "login.oscar.aol.com" ) );
		uint port = configGroup()->readEntry( "Port", 5190 );
		Connection* c = setupConnection( server, port );

		//set up the settings for the account
		Oscar::Settings* oscarSettings = engine()->clientSettings();
		oscarSettings->setWebAware( configGroup()->readEntry( "WebAware", false ) );
		oscarSettings->setHideIP( configGroup()->readEntry( "HideIP", true ) );
		oscarSettings->setRequireAuth( configGroup()->readEntry( "RequireAuth", false ) );
		oscarSettings->setRespectRequireAuth( configGroup()->readEntry( "RespectRequireAuth", true ) );
		oscarSettings->setFileProxy( configGroup()->readEntry( "FileProxy", false ) );
		oscarSettings->setFirstPort( configGroup()->readEntry( "FirstPort", 5190 ) );
		oscarSettings->setLastPort( configGroup()->readEntry( "LastPort", 5199 ) );
		oscarSettings->setTimeout( configGroup()->readEntry( "Timeout", 10 ) );
		//FIXME: also needed for the other call to setStatus (in setPresenceTarget)
		DWORD status = pres.toOscarStatus();

		if ( !mHideIP )
			status |= ICQ::StatusCode::SHOWIP;
		if ( mWebAware )
			status |= ICQ::StatusCode::WEBAWARE;

		engine()->setStatus( status, mInitialStatusMessage );
		updateVersionUpdaterStamp();
		engine()->start( server, port, accountId(), password.left(8) );
		engine()->connectToServer( c, server, true /* doAuth */ );

		mInitialStatusMessage.clear();
	}
}

void ICQAccount::disconnected( DisconnectReason reason )
{
	kDebug(14153) << k_funcinfo << "Attempting to set status offline" << endl;
	ICQ::Presence presOffline = ICQ::Presence( ICQ::Presence::Offline, presence().visibility() );
	myself()->setOnlineStatus( presOffline.toOnlineStatus() );

	QHash<QString, Kopete::Contact*> contactList = contacts();
	foreach( Kopete::Contact* c, contactList )
	{
		OscarContact* oc = dynamic_cast<OscarContact*>( c );
		if ( oc )
		{
			if ( oc->ssiItem().waitingAuth() )
				oc->setOnlineStatus( protocol()->statusManager()->waitingForAuth() );
			else
				oc->setOnlineStatus( ICQ::Presence( ICQ::Presence::Offline, ICQ::Presence::Visible ).toOnlineStatus() );
		}
	}

	OscarAccount::disconnected( reason );
}


void ICQAccount::slotToggleInvisible()
{
	using namespace ICQ;
	setInvisible( (presence().visibility() == Presence::Visible) ? Presence::Invisible : Presence::Visible );
}

void ICQAccount::setAway( bool away, const QString &awayReason )
{
	kDebug(14153) << k_funcinfo << "account='" << accountId() << "'" << endl;
	if ( away )
		setPresenceType( ICQ::Presence::Away, awayReason );
	else
		setPresenceType( ICQ::Presence::Online );
}


void ICQAccount::setInvisible( ICQ::Presence::Visibility vis )
{
	ICQ::Presence pres = presence();
	if ( vis == pres.visibility() )
		return;

	kDebug(14153) << k_funcinfo << "changing invisible setting to " << (int)vis << endl;
	setPresenceTarget( ICQ::Presence( pres.type(), vis ) );
}

void ICQAccount::setPresenceType( ICQ::Presence::Type type, const QString &message )
{
	ICQ::Presence pres = presence();
	kDebug(14153) << k_funcinfo << "new type=" << (int)type << ", old type=" << (int)pres.type() << ", new message=" << message << endl;
	//setAwayMessage(awayMessage);
	setPresenceTarget( ICQ::Presence( type, pres.visibility() ), message );
}

void ICQAccount::setPresenceTarget( const ICQ::Presence &newPres, const QString &message )
{
	bool targetIsOffline = (newPres.type() == ICQ::Presence::Offline);
	bool accountIsOffline = ( presence().type() == ICQ::Presence::Offline ||
	                          myself()->onlineStatus() == protocol()->statusManager()->connectingStatus() );

	if ( targetIsOffline )
	{
		OscarAccount::disconnect();
		// allow toggling invisibility when offline
		myself()->setOnlineStatus( newPres.toOnlineStatus() );
	}
	else if ( accountIsOffline )
	{
		mInitialStatusMessage = message;
		OscarAccount::connect( newPres.toOnlineStatus() );
	}
	else
	{
		engine()->setStatus( newPres.toOscarStatus(), message );
	}
}


void ICQAccount::setOnlineStatus( const Kopete::OnlineStatus& status, const Kopete::StatusMessage &reason )
{
	if ( status.status() == Kopete::OnlineStatus::Invisible )
	{
		// called from outside, i.e. not by our custom action menu entry...

		if ( presence().type() == ICQ::Presence::Offline )
		{
			// ...when we are offline go online invisible.
			setPresenceTarget( ICQ::Presence( ICQ::Presence::Online, ICQ::Presence::Invisible ) );
		}
		else
		{
			// ...when we are not offline set invisible.
			setInvisible( ICQ::Presence::Invisible );
		}
	}
	else
	{
		setPresenceType( ICQ::Presence::fromOnlineStatus( status ).type(), reason.message() );
	}
}

void ICQAccount::setStatusMessage( const Kopete::StatusMessage &statusMessage )
{
}

OscarContact *ICQAccount::createNewContact( const QString &contactId, Kopete::MetaContact *parentContact, const OContact& ssiItem )
{
	ICQContact* contact = new ICQContact( this, contactId, parentContact, QString::null, ssiItem );
	if ( !ssiItem.alias().isEmpty() )
		contact->setProperty( Kopete::Global::Properties::self()->nickName(), ssiItem.alias() );

	if ( isConnected() )
		contact->loggedIn();

	return contact;
}

QString ICQAccount::sanitizedMessage( const QString& message )
{
	return Kopete::Message::escape( message );
}

#include "icqaccount.moc"

//kate: tab-width 4; indent-mode csands;
