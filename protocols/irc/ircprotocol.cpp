/*
    ircprotocol - IRC Protocol

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

#include <qregexp.h>
#include <dom/html_element.h>
#undef KDE_NO_COMPAT
#include <kaction.h>
#include <kdebug.h>
#include <kgenericfactory.h>
#include <ksimpleconfig.h>
#include <kiconloader.h>
#include <kstandarddirs.h>
#include <kglobal.h>
#include <kcharsets.h>
#include <kmessagebox.h>

#include <qlineedit.h>
#include <qdom.h>
#include <qfile.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qvalidator.h>

#include "kopeteaccountmanager.h"
#include "kopetecommandhandler.h"
#include "kopeteview.h"
#include "ksparser.h"

#include "networkconfig.h"
#include "channellist.h"
#include "ircaccount.h"
#include "ircaddcontactpage.h"
#include "ircchannelcontact.h"
#include "irccontactmanager.h"
#include "ircguiclient.h"
#include "ircusercontact.h"
#include "irceditaccountwidget.h"

#include "ircprotocol.h"

typedef KGenericFactory<IRCProtocol> IRCProtocolFactory;
K_EXPORT_COMPONENT_FACTORY( kopete_irc, IRCProtocolFactory( "kopete_irc" )  );

static QRegExp isChannelRegex( QString::fromLatin1("^[#!+&][^\\s,:]+$") );

IRCProtocol *IRCProtocol::s_protocol = 0L;

IRCProtocol::IRCProtocol( QObject *parent, const char *name, const QStringList & /* args */ )
: KopeteProtocol( IRCProtocolFactory::instance(), parent, name ),

	m_ServerStatusOnline(KopeteOnlineStatus::Online,	90, this, 21, QString::null,	i18n("Go O&nline"),	i18n("Online")),
	m_ServerStatusOffline(KopeteOnlineStatus::Offline,	80, this, 20, QString::null,	i18n("Go O&ffline"),	i18n("Offline")),

	m_ChannelStatusOnline(KopeteOnlineStatus::Online,	70, this, 11, QString::null,	i18n("Go O&nline"),	i18n("Online")),
	m_ChannelStatusOffline(KopeteOnlineStatus::Offline,	60, this, 10, QString::null,	i18n("Go O&ffline"),	i18n("Offline")),

	m_UserStatusOp(KopeteOnlineStatus::Online,		50, this, 5, "irc_op",		i18n("Go &Op"),		i18n("Op")),
	m_UserStatusOpAway(KopeteOnlineStatus::Online,		50, this, 5, "irc_away",	i18n("Go &Op"),		i18n("Away")),
	m_UserStatusVoice(KopeteOnlineStatus::Online,		40, this, 4, "irc_voice",	i18n("Go &Voice"),	i18n("Voice")),
	m_UserStatusVoiceAway(KopeteOnlineStatus::Online,	40, this, 4, "irc_away",	i18n("Go &Voice"), i18n("Away")),
	m_UserStatusOnline(KopeteOnlineStatus::Online,		30, this, 3, QString::null,	i18n("Go O&nline"),	i18n("Online")),
	m_UserStatusAway(KopeteOnlineStatus::Away,		20, this, 2, "irc_away",	i18n("Set &Away"),	i18n("Away")),
	m_UserStatusConnecting(KopeteOnlineStatus::Connecting,	10, this, 1, "irc_connecting",	i18n("Connecting"),	i18n("Connecting")),
	m_UserStatusOffline(KopeteOnlineStatus::Offline,	 0, this, 0, QString::null,	i18n("Go O&ffline"),	i18n("Offline")),

	m_StatusUnknown(KopeteOnlineStatus::Unknown, 999, this, 999, "status_unknown", "FIXME: Make this unselectable", i18n("Status not available"))
{
//	kdDebug(14120) << k_funcinfo << endl;

	s_protocol = this;

	//m_status = m_unknownStatus = m_Unknown;

	addAddressBookField("messaging/irc", KopetePlugin::MakeIndexField);

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("raw"),
		SLOT( slotRawCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /raw <text> - Sends the text in raw form to the server."), 1 );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("quote"),
		SLOT( slotQuoteCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /quote <text> - Sends the text in quoted form to the server."), 1 );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("ctcp"),
		SLOT( slotCtcpCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /ctcp <nick> <message> - Send the CTCP message to nick<action>."), 2 );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("ping"),
		SLOT( slotPingCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /ping <nickname> - Alias for /CTCP <nickname> PING."), 1, 1 );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("motd"),
		SLOT( slotMotdCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /motd [<server>] - Shows the message of the day for the current or the given server.") );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("list"),
		SLOT( slotListCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /list - List the public channels on the server.") );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("join"),
		SLOT( slotJoinCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /join <#channel 1> <#channel 2...> - Joins the specified channels."), 1 );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("topic"),
		SLOT( slotTopicCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /topic [<topic>] - Sets and/or displays the topic for the active channel.") );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("whois"),
		SLOT( slotWhoisCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /whois <nickname> - Display whois info on this user."), 1, 1 );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("who"),
		SLOT( slotWhoCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /who <nickname|channel> - Display who info on this user/channel."), 1, 1 );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("query"),
		SLOT( slotQueryCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /query <nickname> [<message>] - Open a private chat with this user."), 1 );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("mode"),
		SLOT( slotModeCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /mode <channel> <modes> - Set modes on the given channel."), 1 );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("nick"),
		SLOT( slotNickCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /nick <nickname> - Change your nickname to the given one."), 1, 1 );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("me"),
		SLOT( slotMeCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /me <action> - Do something."), 1 );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("kick"),
		SLOT( slotKickCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /kick <nickname> [<reason>] - Kick someone from the channel (Requires operator status).")
		, 1 );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("ban"),
		SLOT( slotBanCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /ban <nickname> - Add someone to this channel's ban list. Uses the hostmask nickname!*@* (Requires operator status)."),
		1 );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("op"),
		SLOT( slotOpCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /op <nickname> - Give channel operator status to someone (Requires operator status)."),
		1 );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("deop"),
		SLOT( slotDeopCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /deop <nickname> - Remove channel operator status from someone (Requires operator status)."), 1 );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("voice"),
		SLOT( slotVoiceCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /voice <nickname> - Give channel voice status to someone (Requires operator status)."),
		1);

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("devoice"),
		SLOT( slotDevoiceCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /devoice <nickanme> - Remove channel voice status from someone (Requires operator status)."),1 );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("quit"),
		SLOT( slotQuitCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /quit [<reason>] - Disconnect from IRC, optionally leaving a message.") );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("part"),
		SLOT( slotPartCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /part [<reason>] - Part from a channel, optionally leaving a message.") );

	KopeteCommandHandler::commandHandler()->registerCommand( this, QString::fromLatin1("invite"),
		SLOT( slotInviteCommand( const QString &, KopeteMessageManager*) ),
		i18n("USAGE: /invite <nickname> [<channel>] - Invite a user to join a channel."), 1 );

	KopeteCommandHandler::commandHandler()->registerAlias( this, QString::fromLatin1("j"),
		QString::fromLatin1("join %1"),
		i18n("USAGE: /j <#channel 1>, <#channel 2...> - Alias for JOIN."), KopeteCommandHandler::SystemAlias,
		1 );

	KopeteCommandHandler::commandHandler()->registerAlias( this, QString::fromLatin1("msg"),
		QString::fromLatin1("query %s"),
		i18n("USAGE: /msg <nickname> [<message>] - Alias for QUERY <nickname> <message>."), KopeteCommandHandler::SystemAlias, 1 );

	QObject::connect( KopeteMessageManagerFactory::factory(), SIGNAL(aboutToDisplay(KopeteMessage &)),
		this, SLOT(slotMessageFilter(KopeteMessage &)) );

	QObject::connect( KopeteMessageManagerFactory::factory(), SIGNAL( viewCreated( KopeteView* ) ),
		this, SLOT( slotViewCreated( KopeteView* ) ) );

	m_commandInProgress = false;

	m_networks.setAutoDelete(true);
	m_hosts.setAutoDelete(true);
	netConf = 0L;

	slotReadNetworks();
}

IRCProtocol * IRCProtocol::protocol()
{
	return s_protocol;
}

IRCProtocol::~IRCProtocol()
{
}

void IRCProtocol::slotViewCreated( KopeteView *view )
{
	if( view->msgManager()->protocol() == this )
		new IRCGUIClient( view->msgManager() );
}

bool IRCProtocol::supportsRichText() const
{
	//We support rich text
	return true;
}

void IRCProtocol::slotMessageFilter( KopeteMessage &msg )
{
	if( msg.from()->protocol() == this )
	{
		QString messageText = msg.escapedBody();

		//Add right click for channels, only replace text not in HTML tags
		messageText.replace( QRegExp( QString::fromLatin1("(?![^<]+>)(#[^#\\s]+)(?![^<]+>)") ), QString::fromLatin1("<span class=\"KopeteLink\" type=\"IRCChannel\">\\1</span>") );

		msg.setBody( messageText, KopeteMessage::RichText );
	}
}

QPtrList<KAction> *IRCProtocol::customChatWindowPopupActions( const KopeteMessage &m, DOM::Node &n )
{
	DOM::HTMLElement e = n;

	//isNull checks that the cast was successful
	if( !e.isNull() && !m.to().isEmpty() )
	{
		activeNode = n;
		activeAccount = static_cast<IRCAccount*>( m.from()->account() );
		if( e.getAttribute( QString::fromLatin1("type") ) == QString::fromLatin1("IRCChannel") )
			return activeAccount->contactManager()->findChannel(
				e.innerText().string() )->customContextMenuActions();
	}

	return 0L;
}

AddContactPage *IRCProtocol::createAddContactWidget(QWidget *parent, KopeteAccount *account)
{
	return new IRCAddContactPage(parent,static_cast<IRCAccount*>(account));
}

KopeteEditAccountWidget *IRCProtocol::createEditAccountWidget(KopeteAccount *account, QWidget *parent)
{
	return new IRCEditAccountWidget(this, static_cast<IRCAccount*>(account),parent);
}

KopeteAccount *IRCProtocol::createNewAccount(const QString &accountId)
{
	return new IRCAccount( this, accountId );
}

void IRCProtocol::deserializeContact( KopeteMetaContact *metaContact, const QMap<QString, QString> &serializedData,
	const QMap<QString, QString> & /* addressBookData */ )
{
	kdDebug(14120) << k_funcinfo << endl;

	QString contactId = serializedData[ "contactId" ];
	QString displayName = serializedData[ "displayName" ];

	if( displayName.isEmpty() )
		displayName = contactId;

	QDict<KopeteAccount> accounts = KopeteAccountManager::manager()->accounts( this );
	if( !accounts.isEmpty() )
	{
		KopeteAccount *a = accounts[ serializedData[ "accountId" ] ];
		if( a )
			a->addContact( contactId, displayName, metaContact );
		else
			kdDebug(14120) << k_funcinfo << serializedData[ "accountId" ] << " was a contact's account,"
				" but we don't have it in the accounts list" << endl;
	}
	else
		kdDebug(14120) << k_funcinfo << "No accounts loaded!" << endl;
}

void IRCProtocol::slotRawCommand( const QString &args, KopeteMessageManager *manager )
{
	if( !args.isEmpty() )
	{
		static_cast<IRCAccount*>( manager->account() )->engine()->writeRawMessage( args, false );
	}
	else
	{
		KopeteMessage msg(manager->user(), manager->members(),
			i18n("You must enter some text to send to the server."),
			KopeteMessage::Internal, KopeteMessage::PlainText, KopeteMessage::Chat);
		manager->appendMessage(msg);
	}
}

void IRCProtocol::slotQuoteCommand( const QString &args, KopeteMessageManager *manager )
{
	if( !args.isEmpty() )
	{
		static_cast<IRCAccount*>( manager->account() )->engine()->writeMessage( args, false );
	}
	else
	{
		KopeteMessage msg(manager->user(), manager->members(),
			i18n("You must enter some text to send to the server."),
			KopeteMessage::Internal, KopeteMessage::PlainText, KopeteMessage::Chat);
		manager->appendMessage(msg);
	}
}

void IRCProtocol::slotCtcpCommand( const QString &args, KopeteMessageManager *manager )
{
	if( !args.isEmpty() )
	{
		QString user = args.section( ' ', 0, 0 );
		QString message = args.section( ' ', 1 );
		static_cast<IRCAccount*>( manager->account() )->engine()->writeCtcpQueryMessage( user, QString::null, message );
	}
}

void IRCProtocol::slotMotdCommand( const QString &args, KopeteMessageManager *manager )
{
	QStringList argsList = KopeteCommandHandler::parseArguments( args );
	static_cast<IRCAccount*>( manager->account() )->engine()->motd(argsList.front());
}

void IRCProtocol::slotPingCommand( const QString &args, KopeteMessageManager *manager )
{
	QStringList argsList = KopeteCommandHandler::parseArguments( args );
	static_cast<IRCAccount*>( manager->account() )->engine()->sendCtcpPing(argsList.front());
}

void IRCProtocol::slotListCommand( const QString &/*args*/, KopeteMessageManager *manager )
{
	static_cast<IRCAccount*>( manager->account() )->listChannels();
}

void IRCProtocol::slotTopicCommand( const QString &args, KopeteMessageManager *manager )
{
	KopeteContactPtrList members = manager->members();
	IRCChannelContact *chan = dynamic_cast<IRCChannelContact*>( members.first() );
	if( chan )
	{
		if( !args.isEmpty() )
			chan->setTopic( args );
		else
		{
			static_cast<IRCAccount*>( manager->account() )->engine()->
				writeRawMessage( QString::fromLatin1("TOPIC %1").arg( chan->nickName() ), false );
		}
	}
	else
	{
		KopeteMessage msg(manager->user(), manager->members(),
			i18n("You must be in a channel to use the TOPIC command.").arg( chan->nickName() ),
			KopeteMessage::Internal, KopeteMessage::PlainText, KopeteMessage::Chat);
		manager->appendMessage(msg);
	}
}

void IRCProtocol::slotJoinCommand( const QString &args, KopeteMessageManager *manager )
{
	if( !args.isEmpty() )
	{
		QString chan = KopeteCommandHandler::parseArguments( args ).front();
		if( isChannelRegex.search( chan ) != -1 )
		{
			static_cast<IRCAccount*>( manager->account() )->
				contactManager()->findChannel( chan )->startChat();
		}
		else
		{
			KopeteMessage msg(manager->user(), manager->members(),
				i18n("\"%1\" is an invalid channel. Channels must start with '#', '!', '+', or '&'.").arg(chan),
				KopeteMessage::Internal, KopeteMessage::PlainText, KopeteMessage::Chat);
			manager->appendMessage(msg);
		}
	}
}

void IRCProtocol::slotInviteCommand( const QString &args, KopeteMessageManager *manager )
{
	if( !args.isEmpty() )
	{
		IRCChannelContact *c = 0L;
		QStringList argsList = KopeteCommandHandler::parseArguments( args );

		if( argsList.count() > 1 )
		{
			if( isChannelRegex.search(argsList[1]) != -1 )
			{
				c = static_cast<IRCAccount*>( manager->account() )->contactManager()->
					findChannel( argsList[1] );
			}
			else
			{
				KopeteMessage msg(manager->user(), manager->members(),
					i18n("\"%1\" is an invalid channel. Channels must start with '#', '!', '+', or '&'.").arg(argsList[1]),
					KopeteMessage::Internal, KopeteMessage::PlainText, KopeteMessage::Chat);
				manager->appendMessage(msg);
			}
		}
		else
		{
			KopeteContactPtrList members = manager->members();
			c = dynamic_cast<IRCChannelContact*>( members.first() );
		}

		if( c && c->manager()->contactOnlineStatus( manager->user() ) == m_UserStatusOp )
		{
			static_cast<IRCAccount*>( manager->account() )->engine()->writeMessage(
				QString::fromLatin1("INVITE %1 %2").arg( argsList[0] ).
				arg( c->nickName() )
			);
		}
		else
		{
			KopeteMessage msg(manager->user(), manager->members(), i18n("You must be a channel operator on %1 to do that.").arg(c->nickName()),
				KopeteMessage::Internal, KopeteMessage::PlainText, KopeteMessage::Chat);
			manager->appendMessage(msg);
		}
	}
	else
	{
		KopeteMessage msg(manager->user(), manager->members(), i18n("Please specify a user  to invite."),
			KopeteMessage::Internal, KopeteMessage::PlainText, KopeteMessage::Chat);
		manager->appendMessage(msg);
	}
}

void IRCProtocol::slotQueryCommand( const QString &args, KopeteMessageManager *manager )
{
	if( !args.isEmpty() )
	{
		QString user = args.section( ' ', 0, 0 );
		QString rest = args.section( ' ', 1 );

		if( isChannelRegex.search( user ) == -1 )
		{
			IRCUserContact *c = static_cast<IRCAccount*>( manager->account() )->
				contactManager()->findUser( user );
			c->startChat();
			if( !rest.isEmpty() )
			{
				KopeteMessage msg( c->manager()->user(), c->manager()->members(), rest,
					KopeteMessage::Outbound, KopeteMessage::PlainText, KopeteMessage::Chat);
				c->manager()->sendMessage(msg);
			}
		}
		else
		{
			KopeteMessage msg(manager->user(), manager->members(),
				i18n("\"%1\" is an invalid nickname. Nicknames must not start with '#','!','+', or '&'.").arg(user),
				KopeteMessage::Internal, KopeteMessage::PlainText, KopeteMessage::Chat);
			manager->appendMessage(msg);
		}
	}
}

void IRCProtocol::slotWhoisCommand( const QString &args, KopeteMessageManager *manager )
{
	if( !args.isEmpty() )
	{
		QStringList argsList = KopeteCommandHandler::parseArguments( args );
		static_cast<IRCAccount*>( manager->account() )->engine()->whoisUser( argsList.first() );
		m_commandInProgress = true;
	}
}

void IRCProtocol::slotWhoCommand( const QString &args, KopeteMessageManager *manager )
{
	if( !args.isEmpty() )
	{
		QStringList argsList = KopeteCommandHandler::parseArguments( args );
		static_cast<IRCAccount*>( manager->account() )->engine()->writeMessage(
			QString::fromLatin1("WHO %1").arg( argsList.first() ) );
		m_commandInProgress = true;
	}
}

void IRCProtocol::slotQuitCommand( const QString &args, KopeteMessageManager *manager )
{
	static_cast<IRCAccount*>( manager->account() )->quit( args );
}

void IRCProtocol::slotNickCommand( const QString &args, KopeteMessageManager *manager )
{
	if( !args.isEmpty() )
	{
		QStringList argsList = KopeteCommandHandler::parseArguments( args );
		static_cast<IRCAccount*>( manager->account() )->engine()->changeNickname( argsList.front() );
	}
}

void IRCProtocol::slotModeCommand( const QString &args, KopeteMessageManager *manager )
{
	QStringList argsList = KopeteCommandHandler::parseArguments( args );
	if( argsList.count() > 1 )
	{
		static_cast<IRCAccount*>( manager->account() )->engine()->changeMode( argsList.front(),
			args.section( QRegExp(QString::fromLatin1("\\s+")), 1 ) );
	}
}

void IRCProtocol::slotMeCommand( const QString &args, KopeteMessageManager *manager )
{
	if( !args.isEmpty() )
	{
		KopeteContactPtrList members = manager->members();
		QStringList argsList = KopeteCommandHandler::parseArguments( args );
		static_cast<IRCAccount*>( manager->account() )->engine()->actionContact(
			static_cast<const IRCContact*>(members.first())->nickName(), args );
	}
}

void IRCProtocol::slotKickCommand( const QString &args, KopeteMessageManager *manager )
{
	if( !args.isEmpty() && manager->contactOnlineStatus( manager->user() ) == m_UserStatusOp )
	{
		QRegExp spaces(QString::fromLatin1("\\s+"));
		QString nick = args.section( spaces, 0, 0);
		QString reason = args.section( spaces, 1);
		KopeteContactPtrList members = manager->members();
		QString channel = static_cast<IRCContact*>( members.first() )->nickName();
		if( isChannelRegex.search( channel ) != -1 )
			static_cast<IRCAccount*>( manager->account() )->engine()->kickUser( nick, channel, reason );
	}
}

void IRCProtocol::slotBanCommand( const QString &args, KopeteMessageManager *manager )
{
	if( !args.isEmpty() && manager->contactOnlineStatus( manager->user() ) == m_UserStatusOp )
	{
		QStringList argsList = KopeteCommandHandler::parseArguments( args );
		KopeteContactPtrList members = manager->members();
		IRCChannelContact *chan = static_cast<IRCChannelContact*>( members.first() );
		if( chan && chan->locateUser( argsList.front() ) )
			chan->setMode( QString::fromLatin1("+b %1!*@*").arg( argsList.front() ) );
	}
}

void IRCProtocol::slotPartCommand( const QString &args, KopeteMessageManager *manager )
{
	QStringList argsList = KopeteCommandHandler::parseArguments( args );
	KopeteContactPtrList members = manager->members();
	IRCChannelContact *chan = static_cast<IRCChannelContact*>( members.first() );
	if( chan )
	{
		//TODO: Allow to store a default value for this in the account config
		//and use it here if it exists
		if( !args.isEmpty() )
			static_cast<IRCAccount*>( manager->account() )->engine()->partChannel(chan->nickName(), args);
		else
			chan->part();
		manager->view()->closeView();
	}
}

void IRCProtocol::slotOpCommand( const QString &args, KopeteMessageManager *manager )
{
	simpleModeChange( args, manager, QString::fromLatin1("+o") );
}

void IRCProtocol::slotDeopCommand( const QString &args, KopeteMessageManager *manager )
{
	simpleModeChange( args, manager, QString::fromLatin1("-o") );
}

void IRCProtocol::slotVoiceCommand( const QString &args, KopeteMessageManager *manager )
{
	simpleModeChange( args, manager, QString::fromLatin1("+v") );
}

void IRCProtocol::slotDevoiceCommand( const QString &args, KopeteMessageManager *manager )
{
	simpleModeChange( args, manager, QString::fromLatin1("-v") );
}

void IRCProtocol::simpleModeChange( const QString &args, KopeteMessageManager *manager, const QString &mode )
{
	if( !args.isEmpty() && manager->contactOnlineStatus( manager->user() ) == m_UserStatusOp )
	{
		QStringList argsList = KopeteCommandHandler::parseArguments( args );
		KopeteContactPtrList members = manager->members();
		IRCChannelContact *chan = static_cast<IRCChannelContact*>( members.first() );
		if( chan && chan->locateUser( argsList.front() ) )
			chan->setMode( QString::fromLatin1("%1 %2").arg( mode ).arg( argsList.front() ) );
	}
}


void IRCProtocol::editNetworks()
{
	if( !netConf )
	{
		netConf = new NetworkConfig( 0L, "network_config", true );
		netConf->host->setValidator( new QRegExpValidator( QString::fromLatin1("^[\\w-\\.]*$"), netConf ) );
		netConf->upButton->setPixmap( SmallIcon( "up" )  );
		netConf->downButton->setPixmap( SmallIcon( "down" ) );

		connect( netConf->networkList, SIGNAL( selectionChanged() ), this, SLOT( slotUpdateNetworkConfig() ) );
		connect( netConf->hostList, SIGNAL( selectionChanged() ), this, SLOT( slotUpdateNetworkHostConfig() ) );
		connect( netConf, SIGNAL( accepted() ), this, SLOT( slotSaveNetworkConfig() ) );
		connect( netConf, SIGNAL( rejected() ), this, SLOT( slotReadNetworks() ) );
		connect( netConf->upButton, SIGNAL( clicked() ), this, SLOT( slotMoveServerUp() ) );
		connect( netConf->downButton, SIGNAL( clicked() ), this, SLOT( slotMoveServerDown() ) );
		connect( netConf->removeNetwork, SIGNAL( clicked() ), this, SLOT( slotDeleteNetwork() ) );
		connect( netConf->removeButton, SIGNAL( clicked() ), this, SLOT( slotDeleteHost() ) );
	}

	netConf->networkList->clear();

	for( QDictIterator<IRCNetwork> it( m_networks ); it.current(); ++it )
	{
		IRCNetwork *net = it.current();
		netConf->networkList->insertItem( net->name );
	}

	netConf->networkList->sort();

	slotUpdateNetworkConfig();

	netConf->show();
}

void IRCProtocol::slotUpdateNetworkConfig()
{
	IRCNetwork *net = m_networks[ netConf->networkList->currentText() ];
	if( net )
	{
		netConf->description->setText( net->description );
		netConf->hostList->clear();

		for( QValueList<IRCHost*>::iterator it = net->hosts.begin(); it != net->hosts.end(); ++it )
			netConf->hostList->insertItem( (*it)->host );

		netConf->hostList->setSelected( 0, true );
		slotUpdateNetworkHostConfig();
	}
}

void IRCProtocol::slotUpdateNetworkHostConfig()
{
	IRCHost *host = m_hosts[ netConf->hostList->currentText() ];
	if( host )
	{
		netConf->host->setText( host->host );
		netConf->password->setText( host->password );
		netConf->port->setValue( host->port );
		netConf->useSSL->setChecked( host->ssl );
	}
}

void IRCProtocol::slotDeleteNetwork()
{
	QString network = netConf->networkList->currentText();
	if( KMessageBox::warningContinueCancel(
		0L, i18n("<qt>Are you sure you want to delete the network <b>%1</b>?<br>"
		"Any accounts which use this network will have to be modified.</qt>")
		.arg(network), i18n("Deleting Network"),
		i18n("&Delete Network"), QString::fromLatin1("AskIRCDeleteNetwork") ) == KMessageBox::Continue )
	{
		IRCNetwork *net = m_networks[ network ];
		for( QValueList<IRCHost*>::iterator it = net->hosts.begin(); it != net->hosts.end(); ++it )
			m_hosts.remove( (*it)->host );

		m_networks.remove( network );
		netConf->networkList->removeItem( netConf->networkList->currentItem() );
		slotUpdateNetworkHostConfig();
	}
}

void IRCProtocol::slotDeleteHost()
{
	QString hostName = netConf->host->text();
	if( KMessageBox::warningContinueCancel(
		0L, i18n("<qt>Are you sure you want to delete the host <b>%1</b>?</qt>")
		.arg(hostName), i18n("Deleting Host"),
		i18n("&Delete Host"), QString::fromLatin1("AskIRCDeleteHost")) == KMessageBox::Continue )
	{

	}
}

void IRCProtocol::slotSaveNetworkConfig()
{
	QDomDocument doc("irc-networks");
	QDomNode root = doc.appendChild( doc.createElement("networks") );

	for( QDictIterator<IRCNetwork> it( m_networks ); it.current(); ++it )
	{
		IRCNetwork *net = it.current();

		QDomNode networkNode = root.appendChild( doc.createElement("network") );
		QDomNode nameNode = networkNode.appendChild( doc.createElement("name") );
		nameNode.appendChild( doc.createTextNode( net->name ) );

		QDomNode descNode = networkNode.appendChild( doc.createElement("description") );
		descNode.appendChild( doc.createTextNode( net->description ) );

		QDomNode serversNode = networkNode.appendChild( doc.createElement("servers") );

		for( QValueList<IRCHost*>::iterator it2 = net->hosts.begin(); it2 != net->hosts.end(); it2++ )
		{
			QDomNode serverNode = serversNode.appendChild( doc.createElement("server") );

			QDomNode hostNode = serverNode.appendChild( doc.createElement("host") );
			hostNode.appendChild( doc.createTextNode( (*it2)->host ) );

			QDomNode portNode = serverNode.appendChild( doc.createElement("port" ) );
			portNode.appendChild( doc.createTextNode( QString::number( (*it2)->port ) ) );

			QDomNode sslNode = serverNode.appendChild( doc.createElement("useSSL") );
			sslNode.appendChild( doc.createTextNode( (*it2)->ssl ? "true" : "false" ) );
		}
	}

	kdDebug() << k_funcinfo << doc.toString(4) << endl;
	QFile xmlFile( locateLocal( "appdata", "ircnetworks.xml" ) );
	QTextStream stream( &xmlFile );

	xmlFile.open( IO_WriteOnly );
	stream << doc.toString(4);
	xmlFile.close();

	emit networkConfigUpdated();
}

void IRCProtocol::slotReadNetworks()
{
	m_networks.clear();
	m_hosts.clear();

	QFile xmlFile( locate( "appdata", "ircnetworks.xml" ) );
	xmlFile.open( IO_ReadOnly );

	QDomDocument doc;
	doc.setContent( &xmlFile );
	QDomElement networkNode = doc.documentElement().firstChild().toElement();
	while( !networkNode.isNull () )
	{
		IRCNetwork *net = new IRCNetwork;

		QDomElement networkChild = networkNode.firstChild().toElement();
		while( !networkChild.isNull() )
		{
			if( networkChild.tagName() == "name" )
				net->name = networkChild.text();
			else if( networkChild.tagName() == "description" )
				net->description = networkChild.text();
			else if( networkChild.tagName() == "servers" )
			{
				QDomElement server = networkChild.firstChild().toElement();
				while( !server.isNull() )
				{
					IRCHost *host = new IRCHost;

					QDomElement serverChild = server.firstChild().toElement();
					while( !serverChild.isNull() )
					{
						if( serverChild.tagName() == "host" )
							host->host = serverChild.text();
						else if( serverChild.tagName() == "port" )
							host->port = serverChild.text().toInt();
						else if( serverChild.tagName() == "useSSL" )
						{
							host->ssl = ( serverChild.text() == "true" );
							kdDebug() << "SSL Enabled for " << host->host << ":" << host->ssl << endl;
						}

						serverChild = serverChild.nextSibling().toElement();
					}

					net->hosts.append( host );
					m_hosts.insert( host->host, host );
					server = server.nextSibling().toElement();
				}
			}
			networkChild = networkChild.nextSibling().toElement();
		}

		m_networks.insert( net->name, net );
		networkNode = networkNode.nextSibling().toElement();
	}

	xmlFile.close();
}

void IRCProtocol::slotMoveServerUp()
{
	IRCHost *selectedHost = m_hosts[ netConf->hostList->currentText() ];
	IRCNetwork *selectedNetwork = m_networks[ netConf->networkList->currentText() ];

	if( selectedNetwork && selectedHost )
	{
		QValueList<IRCHost*>::iterator pos = selectedNetwork->hosts.find( selectedHost );
		if( pos != selectedNetwork->hosts.begin() )
		{
			QValueList<IRCHost*>::iterator lastPos = pos;
			lastPos--;
			selectedNetwork->hosts.insert( lastPos, selectedHost );
			selectedNetwork->hosts.remove( pos );
		}
	}

	int currentPos = netConf->hostList->currentItem();
	if( currentPos > 0 )
	{
		netConf->hostList->removeItem( currentPos );
		netConf->hostList->insertItem( selectedHost->host, --currentPos );
		netConf->hostList->setSelected( currentPos, true );
	}
}

void IRCProtocol::slotMoveServerDown()
{
	IRCHost *selectedHost = m_hosts[ netConf->hostList->currentText() ];
	IRCNetwork *selectedNetwork = m_networks[ netConf->networkList->currentText() ];

	if( selectedNetwork && selectedHost )
	{
		QValueList<IRCHost*>::iterator pos = selectedNetwork->hosts.find( selectedHost );
		if( *pos != selectedNetwork->hosts.back() )
		{
			QValueList<IRCHost*>::iterator nextPos = pos;
			nextPos++; nextPos++;
			selectedNetwork->hosts.insert( nextPos, selectedHost );
			selectedNetwork->hosts.remove( pos );
		}
	}

	int currentPos = netConf->hostList->currentItem();
	netConf->hostList->removeItem( currentPos );
	netConf->hostList->insertItem( selectedHost->host, ++currentPos );
	netConf->hostList->setSelected( currentPos, true );
}



#include "ircprotocol.moc"

// vim: set noet ts=4 sts=4 sw=4:
