/*
    msnnotifysocket.cpp - Notify Socket for the MSN Protocol

    Copyright (c) 2002      by Duncan Mac-Vicar Prett <duncan@kde.org>
    Copyright (c) 2002-2003 by Martijn Klingens       <klingens@kde.org>
    Copyright (c) 2002-2005 by Olivier Goffart        <ogoffart at kde.org>
    Copyright (c) 2005      by Michaël Larouche       <larouche@kde.org>
    Copyright (c) 2005      by Gregg Edghill          <gregg.edghill@gmail.com>

    Kopete    (c) 2002-2005 by the Kopete developers  <kopete-devel@kde.org>

    Portions taken from
    KMerlin   (c) 2001      by Olaf Lueg              <olueg@olsd.de>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "msnnotifysocket.h"
#include "msncontact.h"
#include "msnaccount.h"
#include "msnsecureloginhandler.h"
#include "msnchallengehandler.h"

#include <QDateTime>
#include <QRegExp>
#include <qdom.h>
#include <QTextStream>

#include <kdebug.h>
#include <kdeversion.h>
#include <klocale.h>
#include <kcodecs.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <ktemporaryfile.h>
#include <krun.h>
#include <kio/job.h>
#include <qfile.h>
#include <kconfig.h>
#include <knotification.h>

#include "kopeteuiglobal.h"
#include "kopeteglobal.h"
#include "kopetestatusmessage.h"

#include <ctime>


MSNNotifySocket::MSNNotifySocket( MSNAccount *account, const QString& /*msnId*/, const QString &password )
: MSNSocket( account )
{
	m_newstatus = MSNProtocol::protocol()->NLN;
	m_secureLoginHandler=0L;
	m_challengeHandler = 0L;

	m_isHotmailAccount=false;
	m_ping=false;
	m_disconnectReason=Kopete::Account::Unknown;

	m_account = account;
	m_password=password;
	QObject::connect( this, SIGNAL( blockRead( const QByteArray & ) ),
		this, SLOT( slotReadMessage( const QByteArray & ) ) );
	m_keepaliveTimer = 0L;
	m_isLogged = false;
}

MSNNotifySocket::~MSNNotifySocket()
{
	delete m_secureLoginHandler;
	delete m_challengeHandler;

	kDebug(14140) ;
}

void MSNNotifySocket::doneConnect()
{
//	kDebug( 14140 ) << "Negotiating server protocol version";
	sendCommand( "VER", "MSNP11 MSNP10 CVR0" );
}


void MSNNotifySocket::disconnect()
{
	m_isLogged = false;
	if(	m_disconnectReason==Kopete::Account::Unknown )
		m_disconnectReason=Kopete::Account::Manual;
	if( onlineStatus() == Connected )
		sendCommand( "OUT", QString(), false );

	if( m_keepaliveTimer )
		m_keepaliveTimer->stop();

	// the socket is not connected yet, so I should force the signals
	if ( onlineStatus() == Disconnected || onlineStatus() == Connecting )
		emit socketClosed();
	else
		MSNSocket::disconnect();
}

void MSNNotifySocket::handleError( uint code, uint id )
{
	kDebug(14140) ;

	QString handle;
	if(m_tmpHandles.contains(id))
		handle=m_tmpHandles[id];

	// See http://www.hypothetic.org/docs/msn/basics.php for a
	// description of all possible error codes.
	// TODO: Add support for all of these!
	switch( code )
	{
	case 201:
	case 205:
	case 208:
	{
		QString msg = i18n( "<qt>The MSN user '%1' does not exist.<br />Please check the MSN ID.</qt>", handle );
		//KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Sorry, msg, i18n( "MSN Plugin" ) );
		emit errorMessage( MSNSocket::ErrorSorry, msg );
		break;
	}
	case 207:
	case 218:
	case 540:
	{
		QString msg =i18n( "<qt>An internal error occurred in the MSN plugin.<br />"
			      "MSN Error: %1<br />"
			      "please send us a detailed bug report "
			      "at kopete-devel@kde.org containing the raw debug output on the "
			      "console (in gzipped format, as it is probably a lot of output.)" , code);
		//KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Information, msg , i18n( "MSN Internal Error" ) );
		emit errorMessage( MSNSocket::ErrorInternal, msg );
		break;

	}
	case 209:
	{
		if(handle==m_account->accountId())
		{
			QString msg = i18n( "Unable to change your display name.\n"
				"Please ensure your display is not too long and does not contains censored words." );
			//KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Sorry, msg, i18n( "MSN Plugin" ) );
			emit errorMessage( MSNSocket::ErrorSorry, msg );
		}
		/*else
		 {
			QString msg = i18n( "You are trying to change the display name of a user who has not "
				"confirmed his or her email address;\n"
				"the contact was not renamed on the server." );
			KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Error, msg, i18n( "MSN Plugin" ) );
		}*/
		break;
	}
	case 210:
	{
		QString msg = i18n("Your contact list is full; you cannot add any new contacts.");
		//KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Sorry, msg, i18n( "MSN Contact List Full" ) );
		emit errorMessage( MSNSocket::ErrorSorry, msg );
		break;
	}
	case 215:
	{
		QString msg = i18n( "<qt>The user '%1' already exists in this group on the MSN server;<br />"
			"if Kopete does not show the user, please send us a detailed bug report "
			"at kopete-devel@kde.org containing the raw debug output on the "
			"console (in gzipped format, as it is probably a lot of output.)</qt>" , handle);
		//KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Information, msg, i18n( "MSN Plugin" ) );
		emit errorMessage( MSNSocket::ErrorInformation, msg );
		break;
	}
	case 216:
	{
		//This might happen is you rename an user if he is not in the contact list
		//currently, we just iniore;
		//TODO: try to don't rename user not in the list
		//actualy, the bug is in MSNChatSession::slotUserJoined()
		break;
	}
	case 219:
	{
		QString msg = i18n( "The user '%1' seems to already be blocked or allowed on the server." , handle);
		//KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Sorry, msg, i18n( "MSN Plugin" ) );
		emit errorMessage( MSNSocket::ErrorSorry, msg );
		break;
	}
	case 223:
	{
		QString msg = i18n( "You have reached the maximum number of groups:\n"
			"MSN does not support more than 30 groups." );
		//KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Sorry, msg, i18n( "MSN Plugin" ) );
		emit errorMessage( MSNSocket::ErrorSorry, msg );
		break;
	}
	case 224:
	case 225:
	case 230:
	{
		QString msg = i18n("Kopete is trying to perform an operation on a group or a contact that does not exists on the server.\n"
			"This might happen if the Kopete contact list and the MSN-server contact list are not correctly synchronized; if this is the case, you probably should send a bug report.");
		//KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Information,msg, i18n( "MSN Plugin" ) );
		emit errorMessage( MSNSocket::ErrorInformation, msg );
		break;
	}

	case 229:
	{
		QString msg = i18n("The group name is too long; it has not been changed on the MSN server.");
		//KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Sorry, msg, i18n( "Invalid Group Name - MSN Plugin" ) );
		emit errorMessage( MSNSocket::ErrorSorry, msg );
		break;
	}
	case 710:
	{
		QString msg = i18n( "You cannot open a Hotmail inbox because you do not have an MSN account with a valid "
			"Hotmail or MSN mailbox." );
		//KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Sorry, msg, i18n( "MSN Plugin" ) );
		emit errorMessage( MSNSocket::ErrorSorry, msg );
		break;
	}
	case 715:
	{
		/*
		//if(handlev==m_account->accountId())
		QString msg = i18n( "Your email address has not been verified with the MSN server.\n"
			"You should have received a mail with a link to confirm your email address.\n"
			"Some functions will be restricted if you do not confirm your email address." );
		KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Sorry, msg, i18n( "MSN Plugin" ) ); //TODO do not show again
		*/
		break;
	}
	case 800:
	{
		//This happen when too much commends are sent to the server.
		//the command will not be executed, too bad.
		// ignore it for now, as we don't really know what command it was.
/*		QString msg = i18#n( "You are trying to change your status, or your display name too rapidly.\n"
	 		"This might happen if you added yourself to your own contact list." );
		KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Sorry, msg, i18n( "MSN Plugin" ) );
		//FIXME: try to fix this problem*/
		break;
	}
	case 911:
		m_disconnectReason=Kopete::Account::BadPassword;
		disconnect();
		break;
	case 913:
	{
		QString msg = i18n( "You can not send messages when you are offline or when you are invisible." );
		//KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Sorry, msg, i18n( "MSN Plugin" ) );
		emit errorMessage( MSNSocket::ErrorSorry, msg );
		break;
	}
	case 923:
	{
		QString msg = i18n( "You are trying to perform an action you are not allowed to perform in 'kid mode'." );
		//KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Sorry, msg, i18n( "MSN Plugin" ) );
		emit errorMessage( MSNSocket::ErrorSorry, msg );
		break;
	}

	default:
		MSNSocket::handleError( code, id );
		break;
	}
}

void MSNNotifySocket::parseCommand( const QString &cmd, uint id, const QString &data )
{
	//kDebug(14140) << "MSNNotifySocket::parseCommand: Command: " << cmd;

	if ( cmd == "VER" )
	{
		sendCommand( "CVR", "0x0409 winnt 5.1 i386 MSNMSGR 7.0.0816 MSMSGS " + m_account->accountId() );
/*
		struct utsname utsBuf;
		uname ( &utsBuf );

		sendCommand( "CVR", i18n( "MS Local code, see http://www.microsoft.com/globaldev/reference/oslocversion.mspx", "0x0409" ) +
			' ' + escape( utsBuf.sysname ) + ' ' + escape( utsBuf.release ) + ' ' + escape( utsBuf.machine ) + " Kopete " +
			escape( KGlobal::mainComponent().aboutData()->version() ) + " Kopete " + m_msnId );
*/
	}
	else if ( cmd == "CVR" ) //else if ( cmd == "INF" )
	{
		sendCommand( "USR", "TWN I " + m_account->accountId() );
	}
	else if( cmd == "USR" ) //// here follow the auth processus
	{
		if( data.section( ' ', 1, 1 ) == "S" )
		{
			m_secureLoginHandler = new MSNSecureLoginHandler(m_account->accountId(), m_password, data.section( ' ' , 2 , 2 ));

			QObject::connect(m_secureLoginHandler, SIGNAL(loginFailed()), this, SLOT(sslLoginFailed()));
			QObject::connect(m_secureLoginHandler, SIGNAL(loginBadPassword()), this, SLOT(sslLoginIncorrect()));
			QObject::connect(m_secureLoginHandler, SIGNAL(loginSuccesful(QString )), this, SLOT(sslLoginSucceeded(QString )));

			m_secureLoginHandler->login();
		}
		else
		{
			// Successful authentication.
			m_disconnectReason=Kopete::Account::Unknown;

			// Synchronize with the server.
			QString lastSyncTime, lastChange;

			if(m_account->contacts().count() > 1)
			{
				// Retrieve the last synchronization timestamp, and last change timestamp.
				lastSyncTime = m_account->configGroup()->readEntry("lastsynctime", "0");
				lastChange = m_account->configGroup()->readEntry("lastchange", "0");
			}
			else
			{
				//the contactliust has maybe being removed, force to sync
				//(the only contact is myself)
				lastSyncTime='0';
				lastChange='0';
			}

			sendCommand( "SYN", lastChange + ' ' + lastSyncTime);
			// Get client features.
			if(!useHttpMethod()) {
				sendCommand( "GCF", "Shields.xml");
				// We are connected start to ping
				slotSendKeepAlive();
			}
		}
	}
	else if( cmd == "LST" )
	{
		// MSNP11 changed command. Now it's:
		// LST N=passport@hotmail.com F=Display%20Name C=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx 13 xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
		// But can be
		// LST N=passport@hotmail.com 10
		QString publicName, contactGuid, groups;
		uint lists;

		QRegExp regex("N=([^ ]+)(?: F=([^ ]+))?(?: C=([0-9a-fA-F]{8}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{12}))? (\\d+)\\s?((?:[0-9a-fA-F]{8}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{12},?)*)$");
		regex.indexIn(data);

		// Capture passport email.
		m_tmpLastHandle = regex.cap(1);
		// Capture public name.
		publicName = unescape( regex.cap(2) );
		// Capture contact guid.
		contactGuid = regex.cap(3);
		// Capture list enum type.
		lists = regex.cap(4).toUInt();
		// Capture contact group(s) guid(s)
		groups = regex.cap(5);

// 		kDebug(14140) << " msnId: " << m_tmpLastHandle << " publicName: " << publicName << " contactGuid: " << contactGuid << " list: " << lists << " groupGuid: " << groups;

		// handle, publicName, Contact GUID, lists, Group GUID
		emit contactList(  m_tmpLastHandle , publicName, contactGuid, lists, groups );
	}
	else if( cmd == "GCF" )
	{
		m_configFile = data.section(' ', 0, 0);
		read( data.section( ' ', 1, 1 ).toUInt() );
	}
	else if( cmd == "MSG" )
	{
		read( data.section( ' ', 2, 2 ).toUInt() );
	}
	else if( cmd == "ILN" ||  cmd == "NLN" )
	{
		// status handle publicName strangeNumber MSNOBJECT
		MSNContact *c = static_cast<MSNContact*>( m_account->contacts()[ data.section( ' ', 1, 1 ) ] );
		if( c && c->contactId() != m_account->accountId() )
		{
			QString publicName=unescape( data.section( ' ', 2, 2 ) );
			if ( (publicName!=c->contactId() ||  c->hasProperty(Kopete::Global::Properties::self()->nickName().key())  ) &&
						 publicName!=c->property( Kopete::Global::Properties::self()->nickName()).value().toString() )

				changePublicName(publicName,c->contactId());
			QString obj=unescape(data.section( ' ', 4, 4 ));
			c->setObject( obj );
			c->setOnlineStatus( convertOnlineStatus( data.section( ' ', 0, 0 ) ) );
			c->setClientFlags(data.section( ' ', 3, 3 ).toUInt());
		}
	}
	else if( cmd == "UBX" )
	{
		m_tmpLastHandle = data.section(' ', 0, 0);
		uint length = data.section( ' ', 1, 1 ).toUInt();
		if(length > 0) {
			read( length );
		}
	}
	else if( cmd == "UUX" )
	{
		// UUX is sended to acknowledge that the server has received and processed the personal Message.
		// if the result is 0, set the myself() contact personalMessage.
		if( data.section(' ', 0, 0) == QString::fromUtf8("0") )
			m_account->myself()->setProperty(MSNProtocol::protocol()->propPersonalMessage, m_propertyPersonalMessage);
	}
	else if( cmd == "FLN" )
	{
		MSNContact *c = static_cast<MSNContact*>( m_account->contacts()[ data.section( ' ', 0, 0 ) ] );
		if( c && c->contactId() != m_account->accountId() )
		{
			c->setOnlineStatus( MSNProtocol::protocol()->FLN );
			c->removeProperty(  MSNProtocol::protocol()->propClient );
		}
	}
	else if( cmd == "XFR" )
	{
		QString stype=data.section( ' ', 0, 0 );
		if( stype=="SB" ) //switchboard connection (chat)
		{
			// Address, AuthInfo
			emit startChat( data.section( ' ', 1, 1 ), data.section( ' ', 3, 3 ) );
		}
		else if( stype=="NS" ) //notifysocket  ; Got our notification server
		{ //we are connecting and we receive the initial NS, or the msn server encounter a problem, and we are switching to another switchboard
			QString host = data.section( ' ', 1, 1 );
			QString server = host.section( ':', 0, 0 );
			uint port = host.section( ':', 1, 1 ).toUInt();
			setOnlineStatus( Connected );
			emit receivedNotificationServer( server, port );
			disconnect();
		}

	}
	else if( cmd == "RNG" )
	{
		// SessionID, Address, AuthInfo, handle, publicName
		emit invitedToChat( QString::number( id ), data.section( ' ', 0, 0 ), data.section( ' ', 2, 2 ),
			data.section( ' ', 3, 3 ), unescape( data.section( ' ', 4, 4 ) ) );
	}
	else if( cmd == "ADC" )
	{
		QString msnId, list, publicName, contactGuid, groupGuid;

		// Retrieve the list parameter (FL/AL/BL/RL)
		list = data.section( ' ', 0, 0 );

		// Examples of received data
		// ADC TrID xL N=example@passport.com
		// ADC TrID FL C=contactGuid groupdGuid
		// ADC TrID RL N=example@passport.com F=friednly%20name
		// ADC TrID FL N=ex@pas.com F=My%20Name C=contactGuid
		// Thanks Gregg for that complex RegExp.
		QRegExp regex("(?:N=([^ ]+))?(?: F=([^ ]+))?(?: C=([0-9a-fA-F]{8}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{12}))?\\s?((?:[0-9a-fA-F]{8}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{12},?)*)$");
		regex.indexIn( data.section( ' ', 1 ) );

		// Capture passport email.
		msnId = regex.cap(1);
		// Capture public name.
		publicName = unescape( regex.cap(2) );
		// Capture contact guid.
		contactGuid = regex.cap(3);
		// Capture contact group(s) guid(s)
		groupGuid = regex.cap(4);

// 		kDebug(14140) << list << " msnId: " << msnId << " publicName: " << publicName << " contactGuid: " << contactGuid << " groupGuid: " << groupGuid;

		// handle, list, publicName, contactGuid, groupGuid
		emit contactAdded( msnId, list, publicName, contactGuid, groupGuid );
	}
	else if( cmd == "REM" ) // someone is removed from a list
	{
		QString handle, list, contactGuid, groupGuid;
		list = data.section( ' ', 0, 0 );
		if( list  == "FL" )
		{
			// Removing a contact
		 	if( data.count( ' ' ) < 2 )
			{
				contactGuid = data.section( ' ', 1, 1 );
			}
			// Removing a contact from a group
			else if( data.count( ' ' ) < 3 )
			{
				contactGuid = data.section( ' ', 1, 1 );
				groupGuid = data.section( ' ', 2, 2 );
			}
		}
		else
		{
			handle = data.section( ' ', 1, 1);
		}

		// handle, list, contactGuid, groupGuid
		emit contactRemoved( handle, list, contactGuid, groupGuid );

	}
	else if( cmd == "OUT" )
	{
		if( data.section( ' ', 0, 0 ) == "OTH" )
		{
			m_disconnectReason=Kopete::Account::OtherClient;
		}
		disconnect();
	}
	else if( cmd == "CHG" )
	{
		QString status = data.section( ' ', 0, 0 );
		setOnlineStatus( Connected );
		emit statusChanged( convertOnlineStatus( status ) );
	}
	else if( cmd == "SBP" )
	{
		QString contactGuid, type, publicName;
		contactGuid = data.section( ' ', 0, 0 );
		type = data.section( ' ', 1, 1 );
		if(type == "MFN" )
		{
			publicName = unescape( data.section( ' ', 2, 2 ) );
			MSNContact *c = m_account->findContactByGuid( contactGuid );
			if(c != 0L)
			{
				c->setNickName( publicName );
			}
		}
	}
	else if( cmd == "LSG" )
	{
		// New Format: LSG Friends xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
		// groupDisplayName, groupGuid
		emit groupListed( unescape( data.section( ' ', 0, 0 ) ), data.section( ' ', 1, 1) );
	}
	else if( cmd == "ADG" )
	{
		// groupName, groupGuid
		emit groupAdded( unescape( data.section( ' ', 0, 0 ) ),
			data.section( ' ', 1, 1 ) );
	}
	else if( cmd == "REG" )
	{
		// groupGuid, groupName
		emit groupRenamed( data.section( ' ', 0, 0 ), unescape( data.section( ' ', 1, 1 ) ) );
	}
	else if( cmd == "RMG" )
	{
		// groupGuid
		emit groupRemoved( data.section( ' ', 1, 1 ) );
	}
	else if( cmd  == "CHL" )
	{
		m_challengeHandler = new MSNChallengeHandler("CFHUR$52U_{VIX5T", "PROD0101{0RM?UBW");
		// Compute the challenge response hash, and send the response.
		QString chlResponse = m_challengeHandler->computeHash(data.section(' ', 0, 0));
		sendCommand("QRY", m_challengeHandler->productId(), true, chlResponse.toUtf8());
		// Dispose of the challenge handler.
		m_challengeHandler->deleteLater();
		m_challengeHandler = 0L;
	}
	else if( cmd == "SYN" )
	{
		// Retrieve the last synchronization timestamp known to the server.
		QString lastSyncTime = data.section( ' ', 1, 1 );
		QString lastChange   = data.section( ' ', 0, 0 );
		if( lastSyncTime != m_account->configGroup()->readEntry("lastsynctime") ||
			lastChange != m_account->configGroup()->readEntry("lastchange") )
		{
			// If the server timestamp and the local timestamp are different,
			// prepare to receive the contact list.
			emit newContactList();  // remove all contacts data, msn sends a new contact list
			m_account->configGroup()->writeEntry( "lastsynctime" , lastSyncTime);
			m_account->configGroup()->writeEntry( "lastchange", lastChange);
		}else
			kDebug(14140) << "Contact list up-to-date.";

		// set the status
		setStatus( m_newstatus );
	}
	else if( cmd == "BPR" )
	{
		MSNContact *c = static_cast<MSNContact*>( m_account->contacts()[ m_tmpLastHandle ] );
		if( c )
			c->setInfo(data.section( ' ', 0, 0 ),unescape(data.section( ' ', 1, 1 )));
	}
	else if( cmd == "PRP" )
	{
		MSNContact *c = static_cast<MSNContact*>( m_account->myself() );
		if( c )
		{
			QString type = data.section( ' ', 0, 0 );
			QString prpData = unescape( data.section( ' ', 1, 1 ) ); //SECURITY????????
			c->setInfo( type, prpData );
			m_account->configGroup()->writeEntry( type, prpData );
		}
	}
	else if( cmd == "BLP" )
	{
		if( id > 0 ) //FROM BLP
		{
			m_account->configGroup()->writeEntry( "serial" , data.section( ' ', 0, 0 ) );
			m_account->configGroup()->writeEntry( "BLP" , data.section( ' ', 1, 1 ) );
		}
		else //FROM SYN
			m_account->configGroup()->writeEntry( "BLP" , data.section( ' ', 0, 0 ) );
	}
	else if( cmd == "QRY" )
	{
		// Do nothing
	}
	else if( cmd == "QNG" )
	{
		//this is a reply from a ping
		m_ping=false;

		// id is the timeout in fact, and we remove 5% of it
		if( m_keepaliveTimer )
		{
			m_keepaliveTimer->setSingleShot( true );
			m_keepaliveTimer->start( id * 950 );
		}
		kDebug( 14140 ) << "timerTimeout=" << id << "sec";
	}
	else if( cmd == "URL" )
	{
		// URL 6 /cgi-bin/HoTMaiL https://loginnet.passport.com/ppsecure/md5auth.srf?lc=1033 2
		//example of reply: URL 10 /cgi-bin/HoTMaiL https://msnialogin.passport.com/ppsecure/md5auth.srf?lc=1036 3
		QString from_action_url = data.section( ' ', 1, 1 );
		QString rru = data.section( ' ', 0, 0 );
		QString id = data.section( ' ', 2, 2 );

		//write the tmp file
		QString UserID=m_account->accountId();

		time_t actualTime;
		time(&actualTime);
		QString sl = QString::number( ( unsigned long ) actualTime - m_loginTime.toULong() );

		QString md5this( m_MSPAuth + sl + m_password );
		KMD5 md5( md5this.toUtf8() );

		QString hotmailRequest = "<html>\n"
			"<head>\n"
				"<noscript>\n"
					"<meta http-equiv=Refresh content=\"0; url=http://www.hotmail.com\">\n"
				"</noscript>\n"
			"</head>\n"
			"<body onload=\"document.pform.submit(); \">\n"
				"<form name=\"pform\" action=\"" + from_action_url  + "\" method=\"POST\">\n"
					"<input type=\"hidden\" name=\"mode\" value=\"ttl\">\n"
					"<input type=\"hidden\" name=\"login\" value=\"" + UserID.left( UserID.indexOf('@') ) + "\">\n"
					"<input type=\"hidden\" name=\"username\" value=\"" + UserID + "\">\n"
					"<input type=\"hidden\" name=\"sid\" value=\"" + m_sid + "\">\n"
					"<input type=\"hidden\" name=\"kv\" value=\"" + m_kv + "\">\n"
					"<input type=\"hidden\" name=\"id\" value=\""+ id +"\">\n"
					"<input type=\"hidden\" name=\"sl\" value=\"" + sl +"\">\n"
					"<input type=\"hidden\" name=\"rru\" value=\"" + rru + "\">\n"
					"<input type=\"hidden\" name=\"auth\" value=\"" + m_MSPAuth + "\">\n"
					"<input type=\"hidden\" name=\"creds\" value=\"" + QString::fromLatin1( md5.hexDigest() ) + "\">\n"
					"<input type=\"hidden\" name=\"svc\" value=\"mail\">\n"
					"<input type=\"hidden\" name=\"js\" value=\"yes\">\n"
				"</form></body>\n</html>\n";

		KTemporaryFile tmpMailFile;
		tmpMailFile.setPrefix("kopetehotmail-");
		tmpMailFile.setSuffix(".html");
		tmpMailFile.setAutoRemove(false);
		tmpMailFile.open();
		QTextStream textStream ( &tmpMailFile );
		textStream << hotmailRequest;
		textStream.flush();

		KRun::runUrl( KUrl( tmpMailFile.fileName() ), "text/html" , 0l, true );

	}
	else if ( cmd == "NOT" )
	{
		kDebug( 14140 ) << "Received NOT command, issuing read block for '" << id << " more bytes";
		read( id );		
	}	
	else
	{
		// Let the base class handle the rest
		//MSNSocket::parseCommand( cmd, id, data );
		kDebug( 14140 ) << "Unimplemented command '" << cmd << ' ' << id << ' ' << data << "' from server!";
	}
}


void MSNNotifySocket::sslLoginFailed()
{
	m_disconnectReason=Kopete::Account::InvalidHost;
	disconnect();
}

void MSNNotifySocket::sslLoginIncorrect()
{
	m_disconnectReason=Kopete::Account::BadPassword;
	disconnect();
}

void MSNNotifySocket::sslLoginSucceeded(QString ticket)
{
	sendCommand("USR" , "TWN S " + ticket);

	m_secureLoginHandler->deleteLater();
	m_secureLoginHandler = 0L;
}

void MSNNotifySocket::slotMSNAlertUnwanted()
{
	// user not interested .. clean up the list of actions
	m_msnAlertURLs.clear();
}

void MSNNotifySocket::slotMSNAlertLink(unsigned int action)
{
	// index into our action list and pull out the URL that was clicked ..
	KUrl tempURLForLaunch(m_msnAlertURLs[action-1]);
	
	KRun::runUrl(tempURLForLaunch, QString::fromUtf8("text/html") , 0l,  true);
}

void MSNNotifySocket::slotOpenInbox()
{
	sendCommand("URL", "INBOX" );
}

void MSNNotifySocket::sendMail(const QString &email)
{
	sendCommand("URL", QString("COMPOSE " + email).toUtf8() );
}

bool MSNNotifySocket::setUseHttpMethod(bool useHttp)
{
	bool ret = MSNSocket::setUseHttpMethod( useHttp );

	if( useHttpMethod() ) {
		if( m_keepaliveTimer ) {
			delete m_keepaliveTimer;
			m_keepaliveTimer = 0L;
		}
	}
	else {
		if( !m_keepaliveTimer ) {
			m_keepaliveTimer = new QTimer( this );
			m_keepaliveTimer->setObjectName( QLatin1String("m_keepaliveTimer") );
			QObject::connect( m_keepaliveTimer, SIGNAL( timeout() ), SLOT( slotSendKeepAlive() ) );
		}
	}

	return ret;
}

void MSNNotifySocket::slotReadMessage( const QByteArray &bytes )
{
	QString msg = QString::fromUtf8(bytes, bytes.size());

	if(msg.contains("text/x-msmsgsinitialmdatanotification"))
	{
		//Mail-Data: <MD><E><I>301</I><IU>1</IU><O>4</O><OU>2</OU></E><Q><QTM>409600</QTM><QNM>204800</QNM></Q></MD>
		// MD - Mail Data
		// E  - email
		// I  - initial mail
		// IU - initial unread
		// O  - other mail
		// OU - other unread.
		QRegExp regex("<MD><E><I>(\\d+)?</I>(?:<IU>(\\d+)?</IU>)<O>(\\d+)?</O><OU>(\\d+)?</OU></E><Q>.*</Q></MD>");
		regex.indexIn(msg);

		bool unread;
		// Retrieve the number of unread email messages.
		mailCount = regex.cap(2).toUInt(&unread);
		if(unread && mailCount > 0)
		{
			// If there are new email message available, raise the unread email event.
#ifdef __GNUC__
#warning Fix KNotification here
#endif
#if 0
			QObject::connect(KNotification::event( QString::fromUtf8("msn_mail"), i18np( "You have one unread message in your MSN inbox.",
							 "You have %1 unread messages in your MSN inbox.", mailCount ), QPixmap() , 0 ),
				SIGNAL(activated(unsigned int ) ) , this, SLOT( slotOpenInbox() ) );
#endif
		}
	}
	else if(msg.contains("text/x-msmsgsactivemailnotification"))
	{
		 //this sends the server if mails are deleted
		 QString m = msg.right(msg.length() - msg.indexOf("Message-Delta:") );
		 m = m.left(msg.indexOf("\r\n"));
		 mailCount = mailCount - m.right(m.length() -m.indexOf(' ')-1).toUInt();
	}
	else if(msg.contains("text/x-msmsgsemailnotification"))
	{
		 //this sends the server if a new mail has arrived
		QRegExp rx("From-Addr: ([A-Za-z0-9@._\\-]*)");
		rx.indexIn(msg);
		QString m=rx.cap(1);

		mailCount++;
#ifdef __GNUC__
#warning Fix KNotification here
#endif
#if 0
		//TODO:  it is also possible to get the subject  (but warning about the encoding)
		QObject::connect(KNotification::event( QString::fromUtf8("msn_mail"),i18n( "You have one new email from %1 in your MSN inbox." , m),
										0 , 0 ),
				SIGNAL(activated(unsigned int ) ) , this, SLOT( slotOpenInbox() ) );
#endif
	}
	else if(msg.contains("text/x-msmsgsprofile"))
	{
		//Hotmail profile
		if(msg.contains("MSPAuth:"))
		{
			QRegExp rx("MSPAuth: ([A-Za-z0-9$!*]*)");
			rx.indexIn(msg);
			m_MSPAuth=rx.cap(1);
		}
		if(msg.contains("sid:"))
		{
			QRegExp rx("sid: ([0-9]*)");
			rx.indexIn(msg);
			m_sid=rx.cap(1);
		}
		if(msg.contains("kv:"))
		{
			QRegExp rx("kv: ([0-9]*)");
			rx.indexIn(msg);
			m_kv=rx.cap(1);
		}
		if(msg.contains("LoginTime:"))
		{
			QRegExp rx("LoginTime: ([0-9]*)");
			rx.indexIn(msg);
			m_loginTime=rx.cap(1);
		}
			else //IN MSNP9  there are no logintime it seems, so set it manualy
			{
				time_t actualTime;
				time(&actualTime);
				m_loginTime=QString::number((unsigned long)actualTime);
			}
		if(msg.contains("EmailEnabled:"))
		{
			QRegExp rx("EmailEnabled: ([0-9]*)");
			rx.indexIn(msg);
			m_isHotmailAccount = (rx.cap(1).toUInt() == 1);
			emit hotmailSeted(m_isHotmailAccount);
		}
		if(msg.contains("ClientIP:"))
		{
			QRegExp rx("ClientIP: ([0-9.]*)");
			rx.indexIn(msg);
			m_localIP = rx.cap(1);
		}

		// We are logged when we receive the initial profile from Hotmail.
		m_isLogged = true;
	}
	else if (msg.contains("NOTIFICATION"))
	{
		// MSN alert (i.e. NOTIFICATION) [for docs see http://www.hypothetic.org/docs/msn/client/notification.php]
		// format of msg is as follows:
		//
		// <NOTIFICATION ver="2" id="1342902633" siteid="199999999" siteurl="http://alerts.msn.com">
		// <TO pid="0x0006BFFD:0x8582C0FB" name="example@passport.com"/>
		// <MSG pri="1" id="1342902633">
		// 	<SUBSCR url="http://g.msn.com/3ALMSNTRACKING/199999999ToastChange?http://alerts.msn.com/Alerts/MyAlerts.aspx?strela=1"/>
		// 	<ACTION url="http://g.msn.com/3ALMSNTRACKING/199999999ToastAction?http://alerts.msn.com/Alerts/MyAlerts.aspx?strela=1"/>
		// 	<BODY lang="3076" icon="">
		// 		<TEXT>utf8-encoded text</TEXT>
		//	</BODY>
		// </MSG>
		// </NOTIFICATION>

		// MSN sends out badly formed XML .. fix it for them (thanks MS!)
		QString notificationDOMAsString(msg);

		QRegExp rx( "&(?!amp;)" );      // match ampersands but not &amp;
		notificationDOMAsString.replace(rx, "&amp;");
		QDomDocument alertDOM;
		alertDOM.setContent(notificationDOMAsString);

		QDomNodeList msgElements = alertDOM.elementsByTagName("MSG");
		for (int i = 0 ; i < msgElements.count() ; i++)
		{
			QString subscString;
			QString actionString;
			QString textString;

			QDomNode msgDOM = msgElements.item(i);

			QDomNodeList msgChildren = msgDOM.childNodes();
			for (uint i = 0 ; i < msgChildren.length() ; i++) {
				QDomNode child = msgChildren.item(i);
				QDomElement element = child.toElement();
				if (element.tagName() == "SUBSCR")
				{
					QDomAttr subscElementURLAttribute;
					if (element.hasAttribute("url"))
					{
						subscElementURLAttribute = element.attributeNode("url");
						subscString = subscElementURLAttribute.value();
					}
				}
				else if (element.tagName() == "ACTION")
				{
					// process ACTION node to pull out URL the alert is tied to
					QDomAttr actionElementURLAttribute;
					if (element.hasAttribute("url"))
					{
						actionElementURLAttribute = element.attributeNode("url");
						actionString = actionElementURLAttribute.value();
					}
				}
				else if (element.tagName() == "BODY")
				{
					// process BODY node to get the text of the alert
					QDomNodeList textElements = element.elementsByTagName("TEXT");
					if (textElements.count() >= 1)
					{
						QDomElement textElement = textElements.item(0).toElement();
						textString = textElement.text();
					}
				}


			}

//			kDebug( 14140 ) << "subscString " << subscString << " actionString " << actionString << " textString " << textString;
			// build an internal list of actions ... we'll need to index into this list when we receive an event
			QStringList actions;
			actions.append(i18n("More Information"));
			m_msnAlertURLs.append(actionString);

			actions.append(i18n("Manage Subscription"));
			m_msnAlertURLs.append(subscString);

			// Don't do any MSN alerts notification for new blog updates
			if( subscString != QString::fromLatin1("s.htm") && actionString != QString::fromLatin1("a.htm") )
			{
				KNotification* notification = KNotification::event("msn_alert", textString, 0L, 0L);
				QObject::connect(notification, SIGNAL(activated(unsigned int)), this, SLOT(slotMSNAlertLink(unsigned int)));
				QObject::connect(notification, SIGNAL(closed()), this, SLOT(slotMSNAlertUnwanted()));
			}
		} // end for each MSG tag
	}

	if(!m_configFile.isNull())
	{
		// TODO Get client features.
	}

	if(!m_tmpLastHandle.isNull())
	{
		QString personalMessage, currentMedia;
		QDomDocument psm;
		if( psm.setContent(msg) )
		{
			// Get the first child of the xml "document";
			QDomElement psmElement = psm.documentElement().firstChild().toElement();

			while( !psmElement.isNull() )
			{
				if(psmElement.tagName() == QString::fromUtf8("PSM"))
				{
					personalMessage = psmElement.text();
					kDebug(14140) << "Personnal Message received: " << personalMessage;
				}
				else if(psmElement.tagName() == QString::fromUtf8("CurrentMedia"))
				{
					if( !psmElement.text().isEmpty() )
					{
						kDebug(14140) << "XML CurrentMedia: " << psmElement.text();
						currentMedia = processCurrentMedia( psmElement.text() );
					}
				}
				psmElement = psmElement.nextSibling().toElement();
			}

			MSNContact *contact = static_cast<MSNContact*>(m_account->contacts()[ m_tmpLastHandle ]);
			if(contact)
			{
				// TODO: Maybe use a display status message for current media ?
				contact->setStatusMessage( Kopete::StatusMessage(currentMedia.isEmpty() ? personalMessage : currentMedia) );
			}
		}
		m_tmpLastHandle.clear();
	}
}

QString MSNNotifySocket::processCurrentMedia( const QString &mediaXmlElement )
{
	/*
		The value of the CurrentMedia tag you can think of like an array
		separated by "\0" characters (literal backslash followed by zero, not NULL).

		The elements of this "array" are as follows:

		* Application - This is the app you are using. Usually empty
		* Type - This is the type of PSM, either “Music”, “Games” or “Office”
		* Enabled - This is a boolean value (0/1) to enable/disable
		* Format - A formatter string ala .Net; For example, “{0} - {1}”
		* First line - The first line (Matches {0} in the Format)
		* Second line - The second line (Matches {1} in the Format)
		* Third line - The third line (Matches {2} in the Format)

		There is probably no limit to the number of lines unless you go over the maximum length of the tag.

		Example of currentMedia xml tag:
		<CurrentMedia>\0Music\01\0{0} - {1}\0 Song Title\0Song Artist\0Song Album\0\0</CurrentMedia>
		<CurrentMedia>\0Games\01\0Playing {0}\0Game Name\0</CurrentMedia>
		<CurrentMedia>\0Office\01\0Office Message\0Office App Name\0</CurrentMedia>

		From http://msnpiki.msnfanatic.com/index.php/MSNP11:Changes
	*/
	QString application, type, format, currentMedia;
	bool enabled=false, test;
	// \0 is textual, it's the "array" separator.
	QStringList argumentLists = mediaXmlElement.split(QString::fromUtf8("\\0"), QString::KeepEmptyParts);

	// Retrive the "stable" array elements.
	application = argumentLists[0];
	type = argumentLists[1];
	enabled = argumentLists[2].toInt(&test);
	format = argumentLists[3];

	// Get the formatter strings
	QStringList formatterStrings;
	QStringList::ConstIterator it;
	int stringListIndex = 0;
	//for( it = argumentLists.indexOf(4); it != argumentLists.end(); ++it )
	for(stringListIndex = 4; stringListIndex < argumentLists.count() - 1; stringListIndex++)
	{
		formatterStrings.append( argumentLists[stringListIndex] );
	}

	// Replace the formatter in the format string.
	currentMedia = format;
	for(int i=0; i<formatterStrings.size(); i++)
	{
		currentMedia = currentMedia.replace(QString("{%1}").arg(i), formatterStrings[i]);
	}

	if( type == QString::fromUtf8("Music") )
	{
		// the  "♫" is encoded in utf8 (and should be in utf8)
		currentMedia = i18n("Now Listening: ♫ %1 ♫", currentMedia);
	}

	kDebug(1414) << "Current Media received: " << currentMedia;

	return currentMedia;
}

void MSNNotifySocket::addGroup(const QString& groupName)
{
	// escape spaces
	sendCommand( "ADG", escape( groupName ) );
}

void MSNNotifySocket::renameGroup( const QString& groupName, const QString& groupGuid )
{
	// escape spaces
	sendCommand( "REG", groupGuid + ' ' + escape( groupName ) );
}

void MSNNotifySocket::removeGroup( const QString& groupGuid )
{
	sendCommand( "RMG",  groupGuid );
}

void MSNNotifySocket::addContact( const QString &handle, int list, const QString& publicName, const QString& contactGuid, const QString& groupGuid )
{
	QString args;
	switch( list )
	{
		case MSNProtocol::FL:
		{
			// Adding the contact to a group
			if( !contactGuid.isEmpty() )
			{
				args = QString("FL C=%1 %2").arg( contactGuid ).arg( groupGuid );
				kDebug(14140) << "In adding contact to a group";
			}
			// Adding a new contact
			else
			{
				args = QString("FL N=%1 F=%2").arg( handle ).arg( escape( publicName ) );
				kDebug(14140) << "In adding contact to a new contact";
			}
			break;
		}
		case MSNProtocol::AL:
			args = QString("AL N=%1").arg( handle );
			break;
		case MSNProtocol::BL:
			args = QString("BL N=%1").arg( handle );
			break;
		case MSNProtocol::RL:
			args = QString("RL N=%1").arg( handle );
			break;
		default:
			kDebug(14140) <<"WARNING! Unknown list " << list << '!';
			return;
	}
	unsigned int id=sendCommand( "ADC", args );
	m_tmpHandles[id]=handle;
}

void MSNNotifySocket::removeContact( const QString &handle, int list, const QString& contactGuid, const QString& groupGuid )
{
	QString args;
	switch( list )
	{
	case MSNProtocol::FL:
		args = "FL " + contactGuid;
		// Removing a contact from a group
		if( !groupGuid.isEmpty() )
			args += ' ' + groupGuid;
		break;
	case MSNProtocol::AL:
		args = "AL " + handle;
		break;
	case MSNProtocol::BL:
		args = "BL " + handle;
		break;
	case MSNProtocol::PL:
		args = "PL " + handle;
		break;
	default:
		kDebug(14140) << "WARNING! Unknown list " << list << '!';
		return;
	}
	unsigned int id=sendCommand( "REM", args );
	m_tmpHandles[id]=handle;
}

void MSNNotifySocket::setStatus( const Kopete::OnlineStatus &status )
{
//	kDebug( 14140 ) << statusToString( status );

	if( onlineStatus() == Disconnected )
		m_newstatus = status;
	else
		sendCommand( "CHG", statusToString( status ) + ' ' + m_account->myselfClientId() + ' ' + escape(m_account->pictureObject()) );
}

void MSNNotifySocket::changePublicName( const QString &publicName, const QString &handle )
{
	QString tempPublicName = publicName;

	//The maximum length is 387.  but with utf8 or encodage, each character may be triple
	//  387/3 = 129   so we make sure the length is not logner than 129 char,  even if
	// it's possible to have longer nicks.
	if( escape(publicName).length() > 129 )
	{
		tempPublicName = publicName.left(129);
	}

	if( handle.isNull() )
	{
		unsigned int id = sendCommand( "PRP", "MFN " + escape( tempPublicName ) );
		m_tmpHandles[id] = m_account->accountId();
	}
	else
	{
		MSNContact *currentContact = static_cast<MSNContact *>(m_account->contacts()[handle]);
		if(currentContact && !currentContact->guid().isEmpty() )
		{
			// FIXME if there is not guid server disconnects.
			unsigned int id = sendCommand( "SBP", currentContact->guid() + " MFN " + escape( tempPublicName ) );
			m_tmpHandles[id] = handle;
		}
	}
}

void MSNNotifySocket::changePersonalMessage( const Kopete::StatusMessage &personalMessage )
{
	QString tempPersonalMessage;
	QString xmlCurrentMedia;

	// Only espace and cut the personalMessage is the type is normal.
	tempPersonalMessage = personalMessage.message();
	//Magic number : 129 characters
	if( escape(tempPersonalMessage).length() > 129 )
	{
		// We cut. for now.
		tempPersonalMessage = tempPersonalMessage.left(129);
	}

	QDomDocument xmlMessage;
	xmlMessage.appendChild( xmlMessage.createElement( "Data" ) );

	QDomElement psm = xmlMessage.createElement("PSM");
	psm.appendChild( xmlMessage.createTextNode( tempPersonalMessage ) );
	xmlMessage.documentElement().appendChild( psm );

	QDomElement currentMedia = xmlMessage.createElement("CurrentMedia");

	/* Example of currentMedia xml tag:
		<CurrentMedia>\0Music\01\0{0} - {1}\0 Song Title\0Song Artist\0Song Album\0\0</CurrentMedia>
		<CurrentMedia>\0Games\01\0Playing {0}\0Game Name\0</CurrentMedia>
		<CurrentMedia>\0Office\01\0Office Message\0Office App Name\0</CurrentMedia>
	*/
	if( personalMessage.hasMetaData("title") || personalMessage.hasMetaData("artist") || personalMessage.hasMetaData("album") )
	{
		xmlCurrentMedia = "\\0Music\\01\\0";
		QString formatterArguments;
		if( personalMessage.hasMetaData("title") ) // Current Track
		{
			xmlCurrentMedia += "{0}";
			formatterArguments += QString("%1\\0").arg( personalMessage.metaData("title").toString() );
		}
		if( personalMessage.hasMetaData("artist") ) // Current Artist
		{
			xmlCurrentMedia += " - {1}";
			formatterArguments += QString("%1\\0").arg( personalMessage.metaData("artist").toString() );
		}
		if( personalMessage.hasMetaData("album")  ) // Current Album
		{
			xmlCurrentMedia += " ({2})";
			formatterArguments += QString("%1\\0").arg( personalMessage.metaData("album").toString() );
		}
		xmlCurrentMedia += "\\0" + formatterArguments + "\\0";
	}

	currentMedia.appendChild( xmlMessage.createTextNode( xmlCurrentMedia ) );

	// Set the status message for myself, check if currentMedia is empty, for either using the normal or Music personal
	m_propertyPersonalMessage = xmlCurrentMedia.isEmpty() ? tempPersonalMessage : processCurrentMedia( currentMedia.text() );

	xmlMessage.documentElement().appendChild( currentMedia );

	unsigned int id = sendCommand("UUX","",true, xmlMessage.toString().toUtf8(), false);
	m_tmpHandles[id] = m_account->accountId();

}

void MSNNotifySocket::changePhoneNumber( const QString &key, const QString &data )
{
	sendCommand( "PRP", key + ' ' + escape ( data ) );
}


void MSNNotifySocket::createChatSession()
{
	sendCommand( "XFR", "SB" );
}

QString MSNNotifySocket::statusToString( const Kopete::OnlineStatus &status ) const
{
	if( status == MSNProtocol::protocol()->NLN )
		return "NLN";
	else if( status == MSNProtocol::protocol()->BSY )
		return "BSY";
	else if( status == MSNProtocol::protocol()->BRB )
		return "BRB";
	else if( status == MSNProtocol::protocol()->AWY )
		return "AWY";
	else if( status == MSNProtocol::protocol()->PHN )
		return "PHN";
	else if( status == MSNProtocol::protocol()->LUN )
		return "LUN";
	else if( status == MSNProtocol::protocol()->FLN )
		return "FLN";
	else if( status == MSNProtocol::protocol()->HDN )
		return "HDN";
	else if( status == MSNProtocol::protocol()->IDL )
		return "IDL";
	else
	{
		kWarning( 14140 ) << "Unknown status " << status.internalStatus() << '!';
		return "UNK";
	}
}

void MSNNotifySocket::slotSendKeepAlive()
{
	//we did not received the previous QNG
	if(m_ping)
	{
		m_disconnectReason=Kopete::Account::ConnectionReset;
		disconnect();
		/*KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Information,
		i18n( "The connection with the MSN network has been lost." ) , i18n ("MSN Plugin") );*/
		return;
	}
	else
	{
		// Send a dummy command to fake activity. This makes sure MSN doesn't
		// disconnect you when the notify socket is idle.
		sendCommand( "PNG" , QString(), false );
		m_ping=true;
	}

	//at least 90 second has been ellapsed since the last messages
	// we shouldn't receive error from theses command anymore
	m_tmpHandles.clear();
}

Kopete::OnlineStatus MSNNotifySocket::convertOnlineStatus( const QString &status )
{
	if( status == "NLN" )
		return MSNProtocol::protocol()->NLN;
	else if( status == "FLN" )
		return MSNProtocol::protocol()->FLN;
	else if( status == "HDN" )
		return MSNProtocol::protocol()->HDN;
	else if( status == "PHN" )
		return MSNProtocol::protocol()->PHN;
	else if( status == "LUN" )
		return MSNProtocol::protocol()->LUN;
	else if( status == "BRB" )
		return MSNProtocol::protocol()->BRB;
	else if( status == "AWY" )
		return MSNProtocol::protocol()->AWY;
	else if( status == "BSY" )
		return MSNProtocol::protocol()->BSY;
	else if( status == "IDL" )
		return MSNProtocol::protocol()->IDL;
	else
		return MSNProtocol::protocol()->UNK;
}


#include "msnnotifysocket.moc"

// vim: set noet ts=4 sts=4 sw=4:

