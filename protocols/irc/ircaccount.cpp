/*
    ircaccount.cpp - IRC Account

    Copyright (c) 2002      by Nick Betcher <nbetcher@kde.org>
    Copyright (c) 2003      by Jason Keirstead <jason@keirstead.org>

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
#include <qlayout.h>
#include <kaction.h>
#include <kaboutdata.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kinputdialog.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kconfig.h>
#include <kglobal.h>

#include "kopeteaway.h"
#include "kopeteawayaction.h"
#include "kopetecontactlist.h"
#include "kopetemetacontact.h"
#include "kopetecommandhandler.h"

#include "ircaccount.h"
#include "ircprotocol.h"
#include "irccontactmanager.h"
#include "ircservercontact.h"
#include "ircchannelcontact.h"
#include "ircusercontact.h"
#include "ksparser.h"
#include "channellist.h"

ChannelListDialog::ChannelListDialog( KIRC *engine, const QString &caption, QObject *target, const char* slotJoinChan )
 : KDialogBase( 0, "channel_list_widget", false, caption, Close )
{
	m_engine = engine;
	m_list = new ChannelList( this, engine );

	connect( m_list, SIGNAL( channelDoubleClicked( const QString & ) ),
		target, slotJoinChan );

	connect( m_list, SIGNAL( channelDoubleClicked( const QString & ) ),
		this, SLOT( slotChannelDoubleClicked( const QString & ) ) );

	new QHBoxLayout( m_list, 0, spacingHint() );

	setInitialSize( QSize( 500, 400 ) );
	setMainWidget( m_list );
	show();
}

void ChannelListDialog::clear()
{
	m_list->clear();
}

void ChannelListDialog::search()
{
	m_list->search();
}

void ChannelListDialog::slotChannelDoubleClicked( const QString & )
{
	close();
}

IRCAccount::IRCAccount(IRCProtocol *protocol, const QString &accountId)
	: KopeteAccount(protocol, accountId)
{
	m_manager = 0L;
	m_protocol = protocol;
	m_channelList = 0L;

	m_networkName = pluginData( m_protocol, QString::fromLatin1("NetworkName"));
	mNickName = pluginData( m_protocol, QString::fromLatin1("NickName"));
	QString codecMib = pluginData( m_protocol, QString::fromLatin1("Codec"));

	if( m_networkName.isEmpty() && QRegExp( "[^#+&][^\\s]+@[\\w-\\.]+:\\d+" ).exactMatch( accountId ) )
	{
		/*
		Test for backwards compatability. If the account ID matches this,
		then we have an old account. First, try to find it in an existing
		network, and use that. if we can't, then add a new network.
		*/

		mNickName = accountId.section('@',0,0);
		QString serverInfo = accountId.section('@',1);
		QString hostName = serverInfo.section(':',0,0);

		for( QDictIterator<IRCNetwork> it( m_protocol->networks() ); it.current(); ++it )
		{
			IRCNetwork *net = it.current();
			for( QValueList<IRCHost*>::iterator it2 = net->hosts.begin(); it2 != net->hosts.end(); ++it2 )
			{
				if( (*it2)->host == hostName )
				{
					m_networkName = net->name;
					break;
				}
			}

			if( !m_networkName.isEmpty() )
				break;
		}

		if( m_networkName.isEmpty() )
		{
			/* Could not find this host. Add it to the networks structure */

			IRCNetwork *net = new IRCNetwork;
			net->name = i18n("Imported Network");
			net->description = i18n("Network imported from previous version of Kopete");

			IRCHost *host = new IRCHost;
			host->host = hostName;
			host->port = serverInfo.section(':',1).toInt();
			host->password = password();
			host->ssl = false;

			net->hosts.append( host );
			m_protocol->networks().insert( net->name, net );

			m_networkName = net->name;

			setPluginData(m_protocol, QString::fromLatin1( "NickName" ), mNickName );
			setPluginData(m_protocol, QString::fromLatin1( "NetworkName" ), m_networkName );
		}
	}

	slotUpdateNetwork();

	triedAltNick = false;

	m_engine = new KIRC(this);
	if( !codecMib.isEmpty() )
	{
		mCodec = QTextCodec::codecForMib( codecMib.toInt() );
		m_engine->setDefaultCodec( mCodec );
	}
	else
		mCodec = 0L;

	QMap< QString, QString> replies = customCtcpReplies();
	for( QMap< QString, QString >::ConstIterator it = replies.begin(); it != replies.end(); ++it )
		m_engine->addCustomCtcp( it.key(), it.data() );

	QString version=i18n("Kopete IRC Plugin %1 [http://kopete.kde.org]").arg(kapp->aboutData()->version());
	m_engine->setVersionString( version  );

	QObject::connect(m_engine, SIGNAL(successfullyChangedNick(const QString &, const QString &)),
			this, SLOT(successfullyChangedNick(const QString &, const QString &)));

	QObject::connect(m_engine, SIGNAL(incomingFailedServerPassword()),
			this, SLOT(slotFailedServerPassword()));

	QObject::connect(m_engine, SIGNAL(incomingNickInUse(const QString &)),
			this, SLOT(slotNickInUseAlert( const QString &)) );

	QObject::connect(m_engine, SIGNAL(incomingFailedNickOnLogin(const QString &)),
			this, SLOT(slotNickInUse( const QString &)) );

	QObject::connect(m_engine, SIGNAL(userJoinedChannel(const QString &, const QString &)),
		this, SLOT(slotJoinedUnknownChannel(const QString &, const QString &)));

	QObject::connect(m_engine, SIGNAL(connectedToServer()),
		this, SLOT(slotConnectedToServer()));

	QObject::connect(m_engine, SIGNAL(successfulQuit()),
		this, SLOT(slotDisconnected()));

	QObject::connect(m_engine, SIGNAL(disconnected()),
		this, SLOT(slotDisconnected()));

	QObject::connect(m_engine, SIGNAL(incomingServerLoadTooHigh()),
		this, SLOT(slotServerBusy()));

	m_contactManager = new IRCContactManager(mNickName, this);
	setMyself( m_contactManager->mySelf() );
	m_myServer = m_contactManager->myServer();

	mAwayAction = new KopeteAwayAction ( i18n("Set Away"),
		m_protocol->m_UserStatusAway.iconFor( this ), 0, this,
		SLOT(slotGoAway( const QString & )), this );

	currentHost = 0;
}

IRCAccount::~IRCAccount()
{
//	kdDebug(14120) << k_funcinfo << mServer << " " << engine() << endl;
	if ( engine()->isConnected() )
		engine()->quitIRC(i18n("Plugin Unloaded"), true);

	delete m_contactManager;
	delete m_engine;

	if( m_channelList )
		m_channelList->delayedDestruct();
}

void IRCAccount::loaded()
{
	m_engine->setUserName(userName());

}

void IRCAccount::slotNickInUse( const QString &nick )
{
	QString altNickName = altNick();
	if( triedAltNick || altNickName.isEmpty() )
	{
		QString newNick = KInputDialog::getText( i18n( "IRC Plugin" ),
			i18n( "The nickname %1 is already in use. Please enter an alternate nickname:" ).arg( nick ), nick );

		m_engine->changeNickname( newNick );
	}
	else
	{
		triedAltNick = true;
		m_engine->changeNickname( altNickName );
	}
}

void IRCAccount::slotNickInUseAlert( const QString &nick )
{
	KMessageBox::error(0l, i18n("The nickname %1 is already in use").arg(nick), i18n("IRC Plugin"));
}

void IRCAccount::setAltNick( const QString &altNick )
{
	setPluginData(protocol(), QString::fromLatin1( "altNick" ), altNick);
}

const QString IRCAccount::altNick() const
{
	return pluginData(protocol(), QString::fromLatin1("altNick"));
}

const QString IRCAccount::networkName() const
{
	return m_networkName;
}

void IRCAccount::setUserName( const QString &userName )
{
	m_engine->setUserName(userName);
	setPluginData(protocol(), QString::fromLatin1( "userName" ), userName);
}

const QString IRCAccount::userName() const
{
	return pluginData(protocol(), QString::fromLatin1("userName"));
}

void IRCAccount::slotUpdateNetwork()
{
	IRCNetwork *net = m_protocol->networks()[ m_networkName ];
	if( net )
		m_network = net;
	else
		m_network = 0L;
}

void IRCAccount::setNetwork( const QString &network )
{
	m_networkName = network;
	setPluginData(protocol(), QString::fromLatin1( "NetworkName" ), m_networkName );
	slotUpdateNetwork();
}

void IRCAccount::setNickName( const QString &nick )
{
	mNickName = nick;
	setPluginData(protocol(), QString::fromLatin1( "NickName" ), mNickName );
	mySelf()->setNickName( mNickName );
}

void IRCAccount::setCodec( QTextCodec *codec )
{
	mCodec = codec;
	setPluginData( protocol(), QString::fromLatin1( "Codec" ), QString::number(codec->mibEnum()) );
	if( mCodec )
		m_engine->setDefaultCodec( mCodec );
}

QTextCodec *IRCAccount::codec() const
{
	return mCodec;
}

void IRCAccount::setDefaultPart( const QString &defaultPart )
{
	setPluginData( protocol(), QString::fromLatin1( "defaultPart" ), defaultPart );
}

void IRCAccount::setDefaultQuit( const QString &defaultQuit )
{
	setPluginData( protocol(), QString::fromLatin1( "defaultQuit" ), defaultQuit );
}

const QString IRCAccount::defaultPart() const
{
	QString partMsg = pluginData(protocol(), QString::fromLatin1("defaultPart"));
	if( partMsg.isEmpty() )
		return QString::fromLatin1("Kopete %1 : http://kopete.kde.org").arg( kapp->aboutData()->version() );
	return partMsg;
}

const QString IRCAccount::defaultQuit() const
{
	QString quitMsg = pluginData(protocol(), QString::fromLatin1("defaultQuit"));
	if( quitMsg.isEmpty() )
		return QString::fromLatin1("Kopete %1 : http://kopete.kde.org").arg(kapp->aboutData()->version());
	return quitMsg;
}

void IRCAccount::setCustomCtcpReplies( const QMap< QString, QString > &replies ) const
{
	QStringList val;
	for( QMap< QString, QString >::ConstIterator it = replies.begin(); it != replies.end(); ++it )
	{
		m_engine->addCustomCtcp( it.key(), it.data() );
		val.append( QString::fromLatin1("%1=%2").arg( it.key() ).arg( it.data() ) );
	}

	KConfig *config = KGlobal::config();
	config->setGroup( configGroup() );
	config->writeEntry( "CustomCtcp", val );
	config->sync();
}

const QMap< QString, QString > IRCAccount::customCtcpReplies() const
{
	QMap< QString, QString > replies;
	QStringList replyList;

	KConfig *config = KGlobal::config();
	config->setGroup( configGroup() );
	replyList = config->readListEntry( "CustomCtcp" );

	for( QStringList::Iterator it = replyList.begin(); it != replyList.end(); ++it )
		replies[ (*it).section('=', 0, 0 ) ] = (*it).section('=', 1 );

	return replies;
}

void IRCAccount::setConnectCommands( const QStringList &commands ) const
{
	KConfig *config = KGlobal::config();
	config->setGroup( configGroup() );
	config->writeEntry( "ConnectCommands", commands );
	config->sync();
}

const QStringList IRCAccount::connectCommands() const
{
	KConfig *config = KGlobal::config();
	config->setGroup( configGroup() );
	return config->readListEntry( "ConnectCommands" );
}

KActionMenu *IRCAccount::actionMenu()
{
	QString menuTitle = QString::fromLatin1( " %1 <%2> " ).arg( accountId() ).arg( myself()->onlineStatus().description() );

	KActionMenu *mActionMenu = new KActionMenu( accountId(),myself()->onlineStatus().iconFor(this), this, "IRCAccount::mActionMenu" );
	mActionMenu->popupMenu()->insertTitle( myself()->onlineStatus().iconFor( myself() ), menuTitle );

	mActionMenu->insert( new KAction ( i18n("Go Online"), m_protocol->m_UserStatusOnline.iconFor( this ), 0, this, SLOT(connect()), mActionMenu ) );
	mActionMenu->insert( mAwayAction );
	mActionMenu->insert( new KAction ( i18n("Go Offline"), m_protocol->m_UserStatusOffline.iconFor( this ), 0, this, SLOT(disconnect()), mActionMenu ) );
	mActionMenu->popupMenu()->insertSeparator();
	mActionMenu->insert( new KAction ( i18n("Join Channel..."), "", 0, this, SLOT(slotJoinChannel()), mActionMenu ) );
	mActionMenu->insert( new KAction ( i18n("Search Channels..."), "", 0, this, SLOT(slotSearchChannels()), mActionMenu ) );
	mActionMenu->insert( new KAction ( i18n("Show Server Window"), "", 0, this, SLOT(slotShowServerWindow()), mActionMenu ) );

	return mActionMenu;
}

void IRCAccount::connect()
{
	if( m_engine->isConnected() )
	{
		if( isAway() )
			setAway( false );
	}
	else if( m_engine->isDisconnected() )
	{
		const QValueList<IRCHost*> &hosts = m_network->hosts;
		if( hosts.count() == 0 )
		{
			KMessageBox::queuedMessageBox(
				0L, KMessageBox::Error,
				i18n("The network associated with this account, <b>%1</b>, no longer exists. Please"
				" ensure that the account has a valid network."),
				i18n("Network No Longer Exists"), 0 );
		}
		else
		{
			if( currentHost == hosts.count() )
				currentHost = 0;

			IRCHost *host = hosts[ currentHost++ ];
			m_engine->connectToServer( host->host, host->port, mySelf()->nickName() );
		}
	}
}

void IRCAccount::slotConnectedToServer()
{
	//Check who is online
	m_contactManager->checkOnlineNotifyList();

	QStringList m_connectCommands = connectCommands();
	for( QStringList::Iterator it = m_connectCommands.begin(); it != m_connectCommands.end(); ++it )
	{
		KopeteMessageManager *manager = myServer()->manager();
		KopeteCommandHandler::commandHandler()->processMessage( *it, manager );
	}
}

void IRCAccount::slotJoinedUnknownChannel( const QString &user, const QString &channel )
{
	kdDebug(14120) << user << " joins " << channel << ", me=" << m_contactManager->mySelf()->nickName().lower() << endl;
	QString nickname = user.section('!', 0, 0);
	if ( nickname.lower() == m_contactManager->mySelf()->nickName().lower() )
		m_contactManager->findChannel( channel )->startChat();
}

void IRCAccount::slotDisconnected()
{
	triedAltNick = false;
	mySelf()->setOnlineStatus( m_protocol->m_UserStatusOffline );
}

void IRCAccount::disconnect()
{
	quit();
}

void IRCAccount::slotServerBusy()
{
      KMessageBox::queuedMessageBox(
		0L, KMessageBox::Error, i18n("The IRC server is currently too busy to respond to this request."),
		i18n("Server is busy"), 0
	);
}

void IRCAccount::slotSearchChannels()
{
	if( !m_channelList )
	{
		m_channelList = new ChannelListDialog( m_engine,
			i18n("Channel list for %1").arg( m_engine->currentHost() ), this,
			SLOT( slotJoinNamedChannel( const QString & ) ) );
	}
	else
		m_channelList->clear();

	m_channelList->show();
}

void IRCAccount::listChannels()
{
	slotSearchChannels();
	m_channelList->search();
}

void IRCAccount::quit( const QString &quitMessage )
{
	kdDebug(14120) << "Quitting IRC: " << quitMessage << endl;

	// TODO: Add a thing to save a custom quit message in the account wizard,
	// and use that value here if set.
	if( quitMessage.isNull() || quitMessage.isEmpty() )
		m_engine->quitIRC( defaultQuit() );
	else
		m_engine->quitIRC( quitMessage );
}

void IRCAccount::setAway( bool isAway, const QString &awayMessage )
{
	kdDebug(14120) << k_funcinfo << isAway << " " << awayMessage << endl;
	if(m_engine->isConnected())
	{
		static_cast<IRCUserContact *>( myself() )->setAway( isAway );
		engine()->setAway( isAway, awayMessage );
	}
}

/*
 * Ask for server password, and reconnect
 */
void IRCAccount::slotFailedServerPassword()
{
	// JLN
	QString servPass = KopeteAccount::password();
	m_engine->setPassword(servPass);
	connect();
}
void IRCAccount::slotGoAway( const QString &reason )
{
	setAway( true, reason );
}

void IRCAccount::slotShowServerWindow()
{
	m_myServer->startChat();
}

bool IRCAccount::isConnected()
{
	return (myself()->onlineStatus().status() == KopeteOnlineStatus::Online);
}


void IRCAccount::successfullyChangedNick(const QString &/*oldnick*/, const QString &newnick)
{
	kdDebug(14120) << k_funcinfo << "Changing nick to " << newnick << endl;
	mNickName = newnick;
	mySelf()->setNickName( mNickName );
	/*myself()->manager()->setDisplayName( static_cast<IRCUserContact *>( myself() )->caption() );*/
}

bool IRCAccount::addContactToMetaContact( const QString &contactId, const QString &displayName,
	 KopeteMetaContact *m )
{
//	kdDebug(14120) << k_funcinfo << contactId << "|" << displayName << endl;
	//FIXME: I think there are too many tests in this functions.  This function should be called ONLY by
	// KopeteAccount::addContact, where all test are already done. Can a irc developer look at this?   -Olivier
	IRCContact *c;

	if( !m )
	{//This should NEVER happen
		m = new KopeteMetaContact();
		KopeteContactList::contactList()->addMetaContact(m);
		m->setDisplayName( displayName );
	}

	if ( contactId.startsWith( QString::fromLatin1("#") ) )
		c = static_cast<IRCContact*>( contactManager()->findChannel(contactId, m) );
	else
	{
		m_contactManager->addToNotifyList( contactId );
		c = static_cast<IRCContact*>( contactManager()->findUser(contactId, m) );
	}

	if( c->metaContact() != m )
	{//This should NEVER happen
		KopeteMetaContact *old = c->metaContact();
		c->setMetaContact( m );
		KopeteContactPtrList children = old->contacts();
		if( children.isEmpty() )
			KopeteContactList::contactList()->removeMetaContact( old );
	}
	else if( c->metaContact()->isTemporary() ) //FIXME: if the metacontact is temporary, that mean this is a temporary contact
		m->setTemporary(false);

	return true;
}

void IRCAccount::slotJoinNamedChannel( const QString &chan )
{
	contactManager()->findChannel( chan )->startChat();
}

void IRCAccount::slotJoinChannel()
{
	if(!isConnected())
		return;

	QString chan = KInputDialog::getText( i18n( "IRC Plugin" ),
		i18n( "Please enter name of the channel you want to join:" ), QString::null);

	if( !chan.isNull() )
	{
		if( QRegExp( QString::fromLatin1("^[#!+&][^\\s,:]+$") ).search( chan ) != -1 )
			contactManager()->findChannel( chan )->startChat();
		else
			KMessageBox::error(0l, i18n("<qt>\"%1\" is an invalid channel. Channels must start with '#','!','+', or '&'.</qt>").arg(chan), i18n("IRC Plugin"));
	}
}

void IRCAccount::appendMessage( MessageType type, const QString &message )
{
	if( type == Ignore )
		return;

}

IRCUserContact *IRCAccount::mySelf() const
{
	return static_cast<IRCUserContact *>( myself() );
}

IRCServerContact *IRCAccount::myServer() const
{
	return m_myServer;
}

#include "ircaccount.moc"

// vim: set noet ts=4 sts=4 sw=4:

