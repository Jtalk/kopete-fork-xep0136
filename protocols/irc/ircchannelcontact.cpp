/*
    ircchannelcontact.cpp - IRC Channel Contact

    Copyright (c) 2002      by Nick Betcher <nbetcher@kde.org>

    Kopete    (c) 2002      by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include <qtimer.h>

#include <kdebug.h>
#include <kinputdialog.h>
#include <kapplication.h>
#include <kaboutdata.h>
#include <kglobal.h>
#include <kmessagebox.h>

#include "kopeteview.h"
#include "kcodecaction.h"
#include "kopetestdaction.h"
#include "kopetemessagemanagerfactory.h"

#include "irccontactmanager.h"
#include "ircchannelcontact.h"
#include "ircusercontact.h"
#include "ircservercontact.h"
#include "ircaccount.h"
#include "ircprotocol.h"

IRCChannelContact::IRCChannelContact(IRCContactManager *contactManager, const QString &channel, KopeteMetaContact *metac)
	: IRCContact(contactManager, channel, metac, "irc_channel")
{
	mInfoTimer = new QTimer( this );
	QObject::connect(mInfoTimer, SIGNAL(timeout()), this, SLOT( slotUpdateInfo() ) );

	QObject::connect(KopeteMessageManagerFactory::factory(), SIGNAL(viewCreated(KopeteView*)),
			this, SLOT(slotJoinChannel(KopeteView*)) );

	// KIRC Engine stuff
	QObject::connect(m_engine, SIGNAL(userJoinedChannel(const QString &, const QString &)),
		this, SLOT(slotUserJoinedChannel(const QString &, const QString &)));

	QObject::connect(m_engine, SIGNAL(incomingPartedChannel(const QString &, const QString &, const QString &)),
		this, SLOT(slotUserPartedChannel(const QString &, const QString &, const QString &)));

	QObject::connect(m_engine, SIGNAL(incomingKick(const QString &, const QString &,const QString &, const QString &)),
		this, SLOT(slotUserKicked(const QString &, const QString &, const QString &, const QString &)));

	QObject::connect(m_engine, SIGNAL(incomingNamesList(const QString &, const QStringList &)),
		this, SLOT(slotNamesList(const QString &, const QStringList &)));

	QObject::connect(m_engine, SIGNAL(incomingExistingTopic(const QString &, const QString &)),
		this, SLOT(slotChannelTopic(const QString&, const QString &)));

	QObject::connect(m_engine, SIGNAL(incomingTopicChange(const QString &, const QString &, const QString &)),
		this, SLOT(slotTopicChanged(const QString&,const QString&,const QString&)));

	QObject::connect(m_engine, SIGNAL(incomingTopicUser(const QString &, const QString &, const QDateTime &)),
		this, SLOT(slotTopicUser(const QString&,const QString&,const QDateTime&)));

	QObject::connect(m_engine, SIGNAL(incomingModeChange(const QString&, const QString&, const QString&)),
		this, SLOT(slotIncomingModeChange(const QString&,const QString&, const QString&)));

	QObject::connect(m_engine, SIGNAL(incomingChannelMode(const QString&, const QString&, const QString&)),
		this, SLOT(slotIncomingChannelMode(const QString&,const QString&, const QString&)));

	QObject::connect(m_engine, SIGNAL(connectedToServer()),
		this, SLOT(slotConnectedToServer()));

	QObject::connect(m_engine, SIGNAL(incomingFailedChankey(const QString &)),
		this, SLOT(slotFailedChankey(const QString&)));

	QObject::connect(m_engine, SIGNAL(incomingFailedChanFull(const QString &)),
		this, SLOT(slotFailedChanFull(const QString&)));

	QObject::connect(m_engine, SIGNAL(incomingFailedChanInvite(const QString &)),
		this, SLOT(slotFailedChanInvite(const QString&)));

	QObject::connect(m_engine, SIGNAL(incomingFailedChanBanned(const QString &)),
		this, SLOT(slotFailedChanBanned(const QString&)));

	QObject::connect(m_engine, SIGNAL(incomingUserIsAway( const QString &, const QString & )),
		this, SLOT(slotIncomingUserIsAway(const QString &, const QString &)));

	actionJoin = 0L;
	actionModeT = new KToggleAction(i18n("Only Operators Can Change &Topic"), 0, this, SLOT(slotModeChanged()), this );
	actionModeN = new KToggleAction(i18n("&No Outside Messages"), 0, this, SLOT(slotModeChanged()), this );
	actionModeS = new KToggleAction(i18n("&Secret"), 0, this, SLOT(slotModeChanged()), this );
	actionModeM = new KToggleAction(i18n("&Moderated"), 0, this, SLOT(slotModeChanged()), this );
	actionModeI = new KToggleAction(i18n("&Invite Only"), 0, this, SLOT(slotModeChanged()), this );

	updateStatus();
}

IRCChannelContact::~IRCChannelContact()
{
}

void IRCChannelContact::updateStatus()
{
	KIRC::EngineStatus status = m_engine->status();
	switch( status )
	{
		case KIRC::Disconnected:
		case KIRC::Connecting:
		case KIRC::Authentifying:
			setOnlineStatus(m_protocol->m_ChannelStatusOffline);
			break;
		case KIRC::Connected:
		case KIRC::Closing:
			setOnlineStatus(m_protocol->m_ChannelStatusOnline);
			break;
		default:
			setOnlineStatus(m_protocol->m_StatusUnknown);
	}
}

void IRCChannelContact::messageManagerDestroyed()
{
	if(manager(false))
	{
		part();
		KopeteContactPtrList contacts = manager()->members();
		// remove all the users on the channel
		for( KopeteContact *c = contacts.first(); c; c = contacts.next() )
			m_account->contactManager()->unregisterUser(c);
	}

	IRCContact::messageManagerDestroyed();
}

void IRCChannelContact::slotJoinChannel( KopeteView *view )
{
	if( view->msgManager() == manager(false) )
		m_engine->joinChannel(m_nickName, password());
}

void IRCChannelContact::slotConnectedToServer()
{
	setOnlineStatus( m_protocol->m_ChannelStatusOnline );
	if( m_isConnected )
		m_engine->joinChannel(m_nickName, password());
}

void IRCChannelContact::slotNamesList(const QString &channel, const QStringList &nicknames)
{
	if ( m_isConnected && channel.lower() == m_nickName.lower() )
	{
		mJoinedNicks += nicknames;
		if( mJoinedNicks.count() == nicknames.count() )
			slotAddNicknames();
	}
}

void IRCChannelContact::slotAddNicknames()
{
	if( !m_isConnected || mJoinedNicks.isEmpty() )
	{
		slotUpdateInfo();
		return;
	}

	QString nickToAdd = mJoinedNicks.front();
	QChar firstChar = nickToAdd[0];
	if( firstChar == '@' || firstChar == '+' )
		nickToAdd = nickToAdd.remove(0, 1);

	mJoinedNicks.pop_front();
	IRCContact *user;

	if ( nickToAdd.lower() != m_account->mySelf()->nickName().lower() )
	{
		//kdDebug(14120) << k_funcinfo << m_nickName << " NICK: " << nickToAdd << endl;
		user = m_account->contactManager()->findUser( nickToAdd );
		user->setOnlineStatus( m_protocol->m_UserStatusOnline );
		manager()->addContact( static_cast<KopeteContact*>(user) , true);
	}
	else
	{
	        user = m_account->mySelf();
	}

	if ( firstChar == '@' || firstChar == '%' )
		manager()->setContactOnlineStatus( static_cast<KopeteContact*>(user), m_protocol->m_UserStatusOp );
	else if( firstChar == '+')
		manager()->setContactOnlineStatus( static_cast<KopeteContact*>(user), m_protocol->m_UserStatusVoice );

	QTimer::singleShot(0, this, SLOT( slotAddNicknames() ) );
}

void IRCChannelContact::slotUpdateInfo()
{
	if( m_isConnected )
	{
		mInfoTimer->start( 60000, true );
		if( manager()->members().count() < 100 )
			m_engine->writeMessage( QString::fromLatin1("WHO %1").arg( m_nickName ), true );
	}
}

void IRCChannelContact::slotChannelTopic(const QString &channel, const QString &topic)
{
	if( m_isConnected && m_nickName.lower() == channel.lower() )
	{
		mTopic = topic;
		manager()->setDisplayName( caption() );
		KopeteMessage msg((KopeteContact*)this, mMyself, i18n("Topic for %1 is %2").arg(m_nickName).arg(mTopic), KopeteMessage::Internal, KopeteMessage::PlainText, KopeteMessage::Chat);
		/*manager()->*/appendMessage(msg);
	}
}

void IRCChannelContact::slotJoin()
{
	if ( !m_isConnected && onlineStatus().status() == KopeteOnlineStatus::Online )
		execute();
}

void IRCChannelContact::part()
{
	if( m_isConnected )
		m_engine->partChannel( m_nickName, m_account->defaultPart() );
}

void IRCChannelContact::slotIncomingUserIsAway( const QString &nick, const QString & )
{
	if( nick.lower() == m_account->mySelf()->nickName().lower() )
	{
		IRCUserContact *c = m_account->mySelf();
		if( m_isConnected && manager()->members().contains( c ) )
		{
			KopeteOnlineStatus status = manager()->contactOnlineStatus( c );
			if( status == m_protocol->m_UserStatusOp )
				manager()->setContactOnlineStatus( c, m_protocol->m_UserStatusOpAway );
			else if( status == m_protocol->m_UserStatusOpAway )
				manager()->setContactOnlineStatus( c, m_protocol->m_UserStatusOp );
			else if( status == m_protocol->m_UserStatusVoice )
				manager()->setContactOnlineStatus( c, m_protocol->m_UserStatusVoiceAway );
			else if( status == m_protocol->m_UserStatusVoiceAway )
				manager()->setContactOnlineStatus( c, m_protocol->m_UserStatusVoice );
			else if( status == m_protocol->m_UserStatusAway )
				manager()->setContactOnlineStatus( c, m_protocol->m_UserStatusOnline );
			else
				manager()->setContactOnlineStatus( c, m_protocol->m_UserStatusAway );
		}
	}
}

void IRCChannelContact::slotUserJoinedChannel(const QString &user, const QString &channel)
{
	if( m_isConnected && (channel.lower() == m_nickName.lower()) )
	{
		QString nickname = user.section('!', 0, 0);
		if ( nickname.lower() == m_account->mySelf()->nickName().lower() )
		{
			KopeteMessage msg((KopeteContact *)this, mMyself,
				i18n("You have joined channel %1").arg(m_nickName),
				KopeteMessage::Internal, KopeteMessage::PlainText, KopeteMessage::Chat);
			msg.setImportance( KopeteMessage::Low); //set the importance manualy to low
			appendMessage(msg);
			while( !messageQueue.isEmpty() )
			{
				slotSendMsg( messageQueue.front(), manager() );
				messageQueue.pop_front();
			}
			setMode( QString::null );
		}
		else
		{
			IRCUserContact *contact = m_account->contactManager()->findUser( nickname );
			contact->setOnlineStatus( m_protocol->m_UserStatusOnline );
			manager()->addContact((KopeteContact *)contact, true);
			KopeteMessage msg((KopeteContact *)this, mMyself,
				i18n("User <b>%1</b> [%2] joined channel %3").arg(nickname).arg(user.section('!', 1)).arg(m_nickName),
				KopeteMessage::Internal, KopeteMessage::RichText, KopeteMessage::Chat);
			msg.setImportance( KopeteMessage::Low); //set the importance manualy to low
			manager()->appendMessage(msg);
		}
	}
}

void IRCChannelContact::slotUserPartedChannel(const QString &user, const QString &channel, const QString &reason)
{
	QString nickname = user.section('!', 0, 0);
	if ( m_isConnected && channel.lower() == m_nickName.lower() && nickname.lower() != m_engine->nickName().lower() )
	{
		KopeteContact *c = locateUser( nickname );
		if ( c )
		{
			manager()->removeContact( c, reason );
			m_account->contactManager()->unregisterUser(c);
		}
	}
}

void IRCChannelContact::slotUserKicked(const QString &nick, const QString &channel,
		const QString &nickKicked, const QString &reason)
{
	if ( m_isConnected && channel.lower() == m_nickName.lower() )
	{
		QString r = i18n("Kicked by %1.").arg( nick );
		if( reason != nick )
			r.append( i18n(" Reason: %2").arg( reason ) );

		if( nickKicked.lower() != m_engine->nickName().lower() )
		{
			KopeteContact *c = locateUser( nickKicked );
			if ( c )
			{
				manager()->removeContact( c, r );
				m_account->contactManager()->unregisterUser(c);
				KopeteMessage msg( (KopeteContact *)this, mMyself,
					r, KopeteMessage::Internal, KopeteMessage::PlainText, KopeteMessage::Chat);
				msg.setImportance(KopeteMessage::Low);
				appendMessage(msg);
			}
		}
		else
		{
			KMessageBox::error(0l, r, i18n("IRC Plugin"));
			manager()->view()->closeView();
		}
	}
}

void IRCChannelContact::setTopic(const QString &topic)
{
	if ( m_isConnected )
	{
		if( manager()->contactOnlineStatus( manager()->user() ) ==
			m_protocol->m_UserStatusOp || !modeEnabled('t') )
		{
			bool okPressed = true;
			QString newTopic = topic;
			if( newTopic.isNull() )
				newTopic = KInputDialog::getText( i18n("New Topic"), i18n("Enter the new topic:"), mTopic, &okPressed, 0L );

			if( okPressed )
			{
				mTopic = newTopic;
				m_engine->setTopic( m_nickName, newTopic );
			}
		}
		else
		{
			KopeteMessage msg(m_account->myServer(), manager()->members(),
				i18n("You must be a channel operator on %1 to do that.").arg(m_nickName),
				KopeteMessage::Internal, KopeteMessage::PlainText, KopeteMessage::Chat);
			manager()->appendMessage(msg);
		}
	}
}

void IRCChannelContact::slotTopicChanged(const QString &nick, const QString &channel, const QString &newtopic)
{
	if( m_isConnected && m_nickName.lower() == channel.lower() )
	{
		mTopic = newtopic;
		manager()->setDisplayName( caption() );
		KopeteMessage msg(m_account->myServer(), mMyself,
			i18n("%1 has changed the topic to: %2").arg(nick).arg(newtopic), KopeteMessage::Internal, KopeteMessage::PlainText, KopeteMessage::Chat);
		msg.setImportance(KopeteMessage::Low); //set the importance manualy to low
		appendMessage(msg);
	}
}

void IRCChannelContact::slotTopicUser(const QString &channel, const QString &nick, const QDateTime &time)
{
	if( m_isConnected && m_nickName.lower() == channel.lower() )
	{
		KopeteMessage msg( m_account->myServer(), mMyself,
			i18n("Topic set by %1 at %2").arg(nick).arg(
				KGlobal::locale()->formatDateTime(time, true)
			), KopeteMessage::Internal, KopeteMessage::PlainText, KopeteMessage::Chat);
		msg.setImportance(KopeteMessage::Low); //set the importance manualy to low
		appendMessage(msg);
	}
}

void IRCChannelContact::slotIncomingModeChange( const QString &nick, const QString &channel, const QString &mode )
{
	if( m_isConnected && m_nickName.lower() == channel.lower() )
	{
		KopeteMessage msg((KopeteContact *)this, mMyself, i18n("%1 sets mode %2 on  %3").arg(nick).arg(mode).arg(m_nickName), KopeteMessage::Internal, KopeteMessage::PlainText, KopeteMessage::Chat);
		msg.setImportance( KopeteMessage::Low); //set the importance manualy to low
		appendMessage(msg);

		bool inParams = false;
		bool modeEnabled = false;
		QString params = QString::null;
		for( uint i=0; i < mode.length(); i++ )
		{
			switch( mode[i] )
			{
				case '+':
					modeEnabled = true;
					break;

				case '-':
					modeEnabled = false;
					break;

				case ' ':
					inParams = true;
					break;
				default:
					if( inParams )
						params.append( mode[i] );
					else
						toggleMode( mode[i], modeEnabled, false );
					break;
			}
		}
	}
}

void IRCChannelContact::slotIncomingChannelMode( const QString &channel, const QString &mode, const QString &params )
{
	if( m_isConnected && (channel.lower() == m_nickName.lower()) )
	{
		for( uint i=1; i < mode.length(); i++ )
		{
			if( mode[i] != 'l' && mode[i] != 'k' )
				toggleMode( mode[i], true, false );
		}
	}
}

void IRCChannelContact::setMode( const QString &mode )
{
	if( m_isConnected )
		m_engine->changeMode( m_nickName, mode );
}

void IRCChannelContact::slotModeChanged()
{
	toggleMode( 't', actionModeT->isChecked(), true );
	toggleMode( 'n', actionModeN->isChecked(), true );
	toggleMode( 's', actionModeS->isChecked(), true );
	toggleMode( 'm', actionModeM->isChecked(), true );
	toggleMode( 'i', actionModeI->isChecked(), true );
}

void IRCChannelContact::slotFailedChanBanned(const QString &channel)
{
	if ( m_isConnected && channel.lower() == m_nickName.lower() )
	{
		manager()->deleteLater();
		KMessageBox::error( 0l,
			i18n("<qt>You can not join %1 because you have been banned.</qt>").arg(channel), i18n("IRC Plugin") );
	}
}

void IRCChannelContact::slotFailedChanInvite(const QString &channel)
{
	if ( m_isConnected && channel.lower() == m_nickName.lower() )
	{
		manager()->deleteLater();
		KMessageBox::error( 0l,
			i18n("<qt>You can not join %1 because it is set to invite only, and no one has invited you.</qt>").arg(channel), i18n("IRC Plugin") );
	}
}

void IRCChannelContact::slotFailedChanFull(const QString &channel)
{
	if ( m_isConnected && channel.lower() == m_nickName.lower() )
	{
		manager()->deleteLater();
		KMessageBox::error( 0l,
			i18n("<qt>You can not join %1 because it has reached its user limit.</qt>").arg(channel),
			i18n("IRC Plugin") );
	}
}

void IRCChannelContact::slotFailedChankey(const QString &channel)
{
	if ( m_isConnected && channel.lower() == m_nickName.lower() )
	{
		bool ok;
		QString diaPassword = KInputDialog::getText( i18n( "IRC Plugin" ),
			i18n( "Please enter key for channel %1: ").arg(channel),
			QString::null,
			&ok );

		if ( !ok )
			manager()->deleteLater();
		else
		{
			setPassword(diaPassword);
			m_engine->joinChannel(channel, password());
		}
	}
}

void IRCChannelContact::toggleMode( QChar mode, bool enabled, bool update )
{
	if( m_isConnected )
	{
		switch( mode )
		{
			case 't':
   				actionModeT->setChecked( enabled );
				break;
			case 'n':
				actionModeN->setChecked( enabled );
				break;
			case 's':
				actionModeS->setChecked( enabled );
				break;
			case 'm':
				actionModeM->setChecked( enabled );
				break;
			case 'i':
				actionModeI->setChecked( enabled );
				break;
		}
	}

	if( update )
	{
		if( modeMap[mode] != enabled )
		{
			if( enabled )
				setMode( QString::fromLatin1("+") + mode );
			else
				setMode( QString::fromLatin1("-") + mode );
		}
	}

	modeMap[mode] = enabled;
}

bool IRCChannelContact::modeEnabled( QChar mode, QString *value )
{
	if( !value )
		return modeMap[mode];
	
	return false;
}

QPtrList<KAction> *IRCChannelContact::customContextMenuActions()
{
	// KAction stuff
	QPtrList<KAction> *mCustomActions = new QPtrList<KAction>();
	if( !actionJoin )
	{
		actionJoin = new KAction(i18n("&Join"), 0, this, SLOT(slotJoin()), this, "actionJoin");
		actionPart = new KAction(i18n("&Part"), 0, this, SLOT(part()), this, "actionPart");
		actionTopic = new KAction(i18n("Change &Topic..."), 0, this, SLOT(setTopic()), this, "actionTopic");
		actionModeMenu = new KActionMenu(i18n("Channel Modes"), 0, this, "actionModeMenu");

		actionModeMenu->insert( actionModeT );
		actionModeMenu->insert( actionModeN );
		actionModeMenu->insert( actionModeS );
		actionModeMenu->insert( actionModeM );
		actionModeMenu->insert( actionModeI );
		actionModeMenu->setEnabled( true );

		codecAction = new KCodecAction( i18n("&Encoding"), 0, this, "selectcharset" );
		connect( codecAction, SIGNAL( activated( const QTextCodec * ) ),
			this, SLOT( setCodec( const QTextCodec *) ) );
		codecAction->setCodec( codec() );
	}

	mCustomActions->append( actionJoin );
	mCustomActions->append( actionPart );
	mCustomActions->append( actionTopic );
	mCustomActions->append( actionModeMenu );
	mCustomActions->append( codecAction );

	bool isOperator = m_isConnected && ( manager()->contactOnlineStatus( m_account->myself() ) == m_protocol->m_UserStatusOp );

	actionJoin->setEnabled( !m_isConnected );
	actionPart->setEnabled( m_isConnected );
	actionTopic->setEnabled( m_isConnected && ( !modeEnabled('t') || isOperator ) );

	actionModeT->setEnabled(isOperator);
	actionModeN->setEnabled(isOperator);
	actionModeS->setEnabled(isOperator);
	actionModeM->setEnabled(isOperator);
	actionModeI->setEnabled(isOperator);

	return mCustomActions;
}

const QString IRCChannelContact::caption() const
{
	QString cap = QString::fromLatin1("%1 @ %2").arg(m_nickName).arg(m_engine->currentHost());
		if( !mTopic.isNull() && !mTopic.isEmpty() )
			cap.append( QString::fromLatin1(" - %1").arg(mTopic) );

	return cap;
}

void IRCChannelContact::privateMessage(IRCContact *from, IRCContact *to, const QString &message)
{
	if(to == this)
	{
		KopeteMessage msg(from, manager()->members(), message, KopeteMessage::Inbound, KopeteMessage::PlainText, KopeteMessage::Chat);
		/*to->*/appendMessage(msg);
	}
}

void IRCChannelContact::action(IRCContact *from, IRCContact *to, const QString &action)
{
	if(to == this)
	{
		KopeteMessage msg(from, manager()->members(), action, KopeteMessage::Action, KopeteMessage::PlainText, KopeteMessage::Chat);
		/*to->*/appendMessage(msg);
	}
}

#include "ircchannelcontact.moc"
