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
#include <klineeditdlg.h>
#include <kapplication.h>
#include <kaboutdata.h>

#include "kopeteview.h"
#include "kopetestdaction.h"

#include "irccontactmanager.h"
#include "ircchannelcontact.h"
#include "ircusercontact.h"
#include "ircaccount.h"
#include "ircprotocol.h"
#include "ksparser.h"

IRCChannelContact::IRCChannelContact(IRCContactManager *contactManager, const QString &channel, KopeteMetaContact *metac)
	: IRCContact( contactManager, channel, metac, QString::fromLatin1("irc_contact_channel_")+channel )
{
	// KIRC Engine stuff
	QObject::connect(m_engine, SIGNAL(userJoinedChannel(const QString &, const QString &)),
		this, SLOT(slotUserJoinedChannel(const QString &, const QString &)));
	QObject::connect(m_engine, SIGNAL(incomingPartedChannel(const QString &, const QString &, const QString &)),
		this, SLOT(slotUserPartedChannel(const QString &, const QString &, const QString &)));
	QObject::connect(m_engine, SIGNAL(incomingNamesList(const QString &, const QStringList &)),
		this, SLOT(slotNamesList(const QString &, const QStringList &)));
	QObject::connect(m_engine, SIGNAL(incomingExistingTopic(const QString &, const QString &)),
		this, SLOT(slotChannelTopic(const QString&, const QString &)));
	QObject::connect(m_engine, SIGNAL(incomingTopicChange(const QString &, const QString &, const QString &)),
		this, SLOT(slotTopicChanged(const QString&,const QString&,const QString&)));
	QObject::connect(m_engine, SIGNAL(incomingModeChange(const QString&, const QString&, const QString&)),
		this, SLOT(slotIncomingModeChange(const QString&,const QString&, const QString&)));
	QObject::connect(m_engine, SIGNAL(incomingChannelMode(const QString&, const QString&, const QString&)),
		this, SLOT(slotIncomingChannelMode(const QString&,const QString&, const QString&)));
	QObject::connect(m_engine, SIGNAL(connectedToServer()),
		this, SLOT(slotConnectedToServer()));

	updateStatus();
}

IRCChannelContact::~IRCChannelContact()
{
}

void IRCChannelContact::updateStatus()
{
	kdDebug(14120) << k_funcinfo << "for:" << m_nickName << endl;

	KIRC::EngineStatus status = m_engine->status();
	KopeteOnlineStatus kopeteStatus;
	switch( status )
	{
	case KIRC::Disconnected:
	case KIRC::Connecting:
	case KIRC::Authentifying:
		kopeteStatus = IRCProtocol::IRCChannelOffline();
		break;
	case KIRC::Connected:
	case KIRC::Closing:
		kopeteStatus = IRCProtocol::IRCChannelOnline();
		break;
	default:
		kopeteStatus = IRCProtocol::IRCUnknown();
	}
	setOnlineStatus( kopeteStatus );
}

void IRCChannelContact::messageManagerDestroyed()
{
	kdDebug(14120) << k_funcinfo << "for:" << m_nickName << endl;
	if(manager(false))
	{
		slotPart();
		KopeteContactPtrList contacts = manager()->members();
		for( KopeteContact *c = contacts.first(); c; c = contacts.next() )
			m_account->contactManager()->unregisterUser(c);
	}

	IRCContact::messageManagerDestroyed();
}

void IRCChannelContact::slotJoinChannel( KopeteView *view )
{
	if( view->msgManager() == manager(false) )
	{
		m_engine->joinChannel(m_nickName);
	}
}

void IRCChannelContact::slotConnectedToServer()
{
	kdDebug(14120) << k_funcinfo << endl;
	setOnlineStatus( IRCProtocol::IRCChannelOnline() );
}

void IRCChannelContact::slotNamesList(const QString &channel, const QStringList &nicknames)
{
	if ( m_isConnected && channel.lower() == m_nickName.lower() )
	{
		kdDebug(14120) << k_funcinfo << "Names List:" << channel << endl;

		mJoinedNicks += nicknames;
		if( mJoinedNicks.count() == nicknames.count() )
			slotAddNicknames();
	}
}

void IRCChannelContact::slotAddNicknames()
{
	if( !m_isConnected || mJoinedNicks.isEmpty() )
		return;

	QString nickToAdd = mJoinedNicks.front();
	mJoinedNicks.pop_front();

	if (nickToAdd.lower() != m_nickName.lower())
	{
		QChar firstChar = nickToAdd[0];
		if( firstChar == '@' || firstChar == '+' )
			nickToAdd = nickToAdd.remove(0, 1);

		IRCContact *user = m_account->findUser( nickToAdd );
		user->setOnlineStatus( IRCProtocol::IRCUserOnline() );

		if ( firstChar == '@' )
			manager()->setContactOnlineStatus( static_cast<KopeteContact*>(user), IRCProtocol::IRCUserOp() );
		else if( firstChar == '+')
			manager()->setContactOnlineStatus( static_cast<KopeteContact*>(user), IRCProtocol::IRCUserVoice() );

		manager()->addContact( static_cast<KopeteContact*>(user) , true);
	}
	QTimer::singleShot(0, this, SLOT( slotAddNicknames() ) );
}

void IRCChannelContact::slotChannelTopic(const QString &channel, const QString &topic)
{
	if( m_isConnected && m_nickName.lower() == channel.lower() )
	{
		mTopic = topic;
		manager()->setDisplayName( caption() );
		KopeteMessage msg((KopeteContact*)this, mMyself, i18n("Topic for %1 is %2").arg(m_nickName).arg(mTopic), KopeteMessage::Internal, KopeteMessage::PlainText, KopeteMessage::Chat);
		msg.setImportance( KopeteMessage::Low); //set the importance manualy to low
		manager()->appendMessage(msg);
	}
}

void IRCChannelContact::slotJoin()
{
	if ( !m_isConnected && onlineStatus().status() == KopeteOnlineStatus::Online )
		execute();
}

void IRCChannelContact::slotPart()
{
	kdDebug(14120) << k_funcinfo << "Part channel:" << m_nickName << endl;
	if( m_isConnected )
		m_engine->partChannel(m_nickName, QString("Kopete %1 : http://kopete.kde.org").arg(kapp->aboutData()->version()));
}

void IRCChannelContact::slotUserJoinedChannel(const QString &user, const QString &channel)
{
	if( m_isConnected && (channel.lower() == m_nickName.lower()) )
	{
		QString nickname = user.section('!', 0, 0);
		if ( nickname.lower() == m_engine->nickName().lower() )
		{
			KopeteMessage msg((KopeteContact *)this, mMyself, i18n("You have joined channel %1").arg(m_nickName),
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
			IRCUserContact *contact = m_account->findUser( nickname );
			contact->setOnlineStatus( IRCProtocol::IRCUserOnline() );
			manager()->addContact((KopeteContact *)contact, true);
			KopeteMessage msg((KopeteContact *)this, mMyself, i18n("User <b>%1</b> [%2] joined channel %3").arg(nickname).arg(user.section('!', 1)).arg(m_nickName), KopeteMessage::Internal, KopeteMessage::RichText, KopeteMessage::Chat);
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
			m_account->unregisterUser( nickname );
		}
	}
}

void IRCChannelContact::setTopic(const QString &topic)
{
	if ( m_isConnected )
	{
		bool okPressed = true;
		QString newTopic = topic;
		if( newTopic.isNull() )
			newTopic = KLineEditDlg::getText( i18n("New Topic"), i18n("Enter the new topic:"), mTopic, &okPressed, 0L );

		if( okPressed )
		{
			mTopic = newTopic;
			m_engine->setTopic( m_nickName, newTopic );
		}
	}
}

void IRCChannelContact::slotTopicChanged(const QString &channel, const QString &nick, const QString &newtopic)
{
	if( m_isConnected && m_nickName.lower() == channel.lower() )
	{
		mTopic = newtopic;
		manager()->setDisplayName( caption() );
		KopeteMessage msg((KopeteContact *)this, mMyself, i18n("%1 has changed the topic to %2").arg(nick).arg(newtopic), KopeteMessage::Internal, KopeteMessage::PlainText, KopeteMessage::Chat);
		msg.setImportance(KopeteMessage::Low); //set the importance manualy to low
		appendMessage(msg);
	}
}

void IRCChannelContact::slotIncomingModeChange( const QString &nick, const QString &channel, const QString &mode )
{
	if( m_isConnected && m_nickName.lower() == channel.lower() )
	{
		KopeteMessage msg((KopeteContact *)this, mMyself, i18n("%1 sets mode %2 %3").arg(nick).arg(mode).arg(m_nickName), KopeteMessage::Internal, KopeteMessage::PlainText, KopeteMessage::Chat);
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
	if( m_isConnected && channel.lower() == m_nickName.lower() )
	{
		kdDebug(14120) << k_funcinfo << channel << " : " << mode << " : " << params << endl;
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
	/*toggleMode( 't', actionModeT->isChecked(), true );
	toggleMode( 'n', actionModeN->isChecked(), true );
	toggleMode( 's', actionModeS->isChecked(), true );
	toggleMode( 'm', actionModeM->isChecked(), true );
	toggleMode( 'i', actionModeI->isChecked(), true );*/
}

void IRCChannelContact::toggleMode( QChar mode, bool enabled, bool update )
{
	if( m_isConnected )
	{
		/*switch( mode )
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
		}*/
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

KActionCollection *IRCChannelContact::customContextMenuActions()
{
	// KAction stuff
	mCustomActions = new KActionCollection(this);
	actionJoin = new KAction(i18n("&Join"), 0, this, SLOT(slotJoin()), mCustomActions, "actionJoin");
	actionPart = new KAction(i18n("&Part"), 0, this, SLOT(slotPart()), mCustomActions, "actionPart");
	actionTopic = new KAction(i18n("Change &Topic..."), 0, this, SLOT(setTopic()), mCustomActions, "actionTopic");
	actionModeMenu = new KActionMenu(i18n("Channel Modes"), 0, mCustomActions, "actionModeMenu");

	actionModeT = new KToggleAction(i18n("Only Operators Can Change &Topic"), 0, this, SLOT(slotModeChanged()), actionModeMenu );
	actionModeN = new KToggleAction(i18n("&No Outside Messages"), 0, this, SLOT(slotModeChanged()), actionModeMenu );
	actionModeS = new KToggleAction(i18n("&Secret"), 0, this, SLOT(slotModeChanged()), actionModeMenu );
	actionModeM = new KToggleAction(i18n("&Moderated"), 0, this, SLOT(slotModeChanged()), actionModeMenu );
	actionModeI = new KToggleAction(i18n("&Invite Only"), 0, this, SLOT(slotModeChanged()), actionModeMenu );

	actionModeMenu->insert( actionModeT );
	actionModeMenu->insert( actionModeN );
	actionModeMenu->insert( actionModeS );
	actionModeMenu->insert( actionModeM );
	actionModeMenu->insert( actionModeI );
	actionModeMenu->setEnabled( true );

	bool isOperator = m_isConnected && ( manager()->contactOnlineStatus( m_account->myself() ) == IRCProtocol::IRCUserOp() );

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
	QString cap;
	if ( m_isConnected )
	{
		cap = QString::fromLatin1("%1 @ %2").arg(m_nickName).arg(m_engine->host());
		if( !mTopic.isNull() && !mTopic.isEmpty() )
			cap.append( QString::fromLatin1(" - %1").arg(mTopic) );
	}
	else
		cap = QString::null;

	return cap;
}

void IRCChannelContact::privateMessage(IRCContact *from, IRCContact *to, const QString &message)
{
	if(to == this)
	{
		KopeteMessage msg(from, manager()->members(), message, KopeteMessage::Inbound, KopeteMessage::PlainText, KopeteMessage::Chat);
		msg.setBody( KSParser::parse( msg.escapedBody() ), KopeteMessage::RichText );
		/*to->*/appendMessage(msg);
	}
}

void IRCChannelContact::action(IRCContact *from, IRCContact *to, const QString &action)
{
	if(to == this)
	{
		KopeteMessage msg(from, manager()->members(), action, KopeteMessage::Action, KopeteMessage::PlainText, KopeteMessage::Chat);
		msg.setBody( KSParser::parse( msg.escapedBody() ), KopeteMessage::RichText );
		/*to->*/appendMessage(msg);
	}
}

#include "ircchannelcontact.moc"
