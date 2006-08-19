/*
    qqaccount.cpp - Kopete QQ Protocol

    Copyright (c) 2003      by Will Stephenson		 <will@stevello.free-online.co.uk>
    Kopete    (c) 2002-2003 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU General Public                   *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/


#include "qqaccount.h"

#include <QTextCodec>
#include <QDateTime>

#include <kaction.h>
#include <kdebug.h>
#include <klocale.h>
#include <kactionmenu.h>
#include <kmenu.h>
#include <kicon.h>

#include "kopetemetacontact.h"
#include "kopetecontactlist.h"
#include "kopetegroup.h"

#include "qqcontact.h"
#include "qqnotifysocket.h"
#include "qqfakeserver.h"
#include "qqprotocol.h"
#include "qqchatsession.h"


QQAccount::QQAccount( QQProtocol *parent, const QString& accountID )
: Kopete::PasswordedAccount ( parent, accountID )
{
	m_notifySocket = 0L;
	m_connectstatus = QQProtocol::protocol()->Offline;
	m_newContactList=false;
 
	// Init the myself contact
	setMyself( new QQContact( this, accountId(), Kopete::ContactList::self()->myself() ) );
}

// The default implementation is TCP
QString QQAccount::serverName()
{
	return configGroup()->readEntry(  "serverName" , "tcpconn3.tencent.com" );
}

uint QQAccount::serverPort()
{
	return configGroup()->readEntry(  "serverPort" , 80 );
}

void QQAccount::connectWithPassword( const QString &password )
{
	kDebug ( 14210 ) << k_funcinfo << "connect with password" << password << endl;
	myself()->setOnlineStatus( QQProtocol::protocol()->qqOnline );
}

/* FIXME: move all things to connectWithPassword */
void QQAccount::connect( const Kopete::OnlineStatus& /* initialStatus */ )
{
	kDebug ( 14210 ) << k_funcinfo << endl;

	// FIXME: add invisible here!

	if ( isConnected() )
	{
		kDebug( 14210 ) << k_funcinfo <<"Ignoring Connect request "
			<< "(Already Connected)" << endl;
		return;
	}

	if ( m_notifySocket )
	{
		kDebug( 14210 ) << k_funcinfo <<"Ignoring Connect request (Already connecting)"  << endl;
		return;
	}
	/* Hard-coded password for debug only */
	m_password = "qqsucks";
	createNotificationServer(serverName(), serverPort());
}

void QQAccount::createNotificationServer( const QString &host, uint port )
{
	if(m_notifySocket) //we are switching from one to another notifysocket.
	{
		//remove every slots to that socket, so we won't delete receive signals
		// from the old socket thinking they are from the new one
		QObject::disconnect( m_notifySocket , 0, this, 0 );
		m_notifySocket->deleteLater(); //be sure it will be deleted
		m_notifySocket=0L;
	}

	myself()->setOnlineStatus( QQProtocol::protocol()->CNT );
	m_notifySocket = new QQNotifySocket( this, m_password );

	QObject::connect( m_notifySocket, SIGNAL( statusChanged( const Kopete::OnlineStatus & ) ),
		SLOT( slotStatusChanged( const Kopete::OnlineStatus & ) ) );
	QObject::connect( m_notifySocket, SIGNAL( newContactList() ),
		SLOT( slotNewContactList() ) );
	QObject::connect( m_notifySocket, SIGNAL( groupNames(const QStringList& )),
		SLOT( slotGroupNamesListed(const QStringList& ) ) );
	
	QObject::connect( m_notifySocket, SIGNAL( contactInGroup(int, char, int)),
		SLOT( slotContactInGroup(int, char, int)) );

	QObject::connect( m_notifySocket, SIGNAL( contactList(const Eva::ContactInfo &) ),
		SLOT( slotContactListed(const Eva::ContactInfo &) ) );

	QObject::connect( m_notifySocket, SIGNAL( contactStatusChanged(const Eva::ContactStatus&) ),
		SLOT( slotContactStatusChanged(const Eva::ContactStatus &) ) );

	QObject::connect( m_notifySocket, SIGNAL( messageReceived( const Eva::MessageHeader&, const Eva::ByteArray& ) ),
		SLOT( slotMessageReceived( const Eva::MessageHeader&, const Eva::ByteArray& ) ) );

	m_notifySocket->connect(host, port);
}

void QQAccount::disconnect()
{
	if ( m_notifySocket )
		m_notifySocket->disconnect();
}

KActionMenu* QQAccount::actionMenu()
{
	KActionMenu *mActionMenu = Kopete::Account::actionMenu();

	mActionMenu->addSeparator();

	KAction *action;
	
	action = new KAction (KIcon("qq_showvideo"), i18n ("Show my own video..."), 0, "actionShowVideo");
	QObject::connect( action, SIGNAL(triggered(bool)), this, SLOT(slotShowVideo()) );
	mActionMenu->addAction(action);
	action->setEnabled( isConnected() );

	return mActionMenu;
}

QQNotifySocket *QQAccount::notifySocket()
{
	return m_notifySocket;
}



void QQAccount::setOnlineStatus(const Kopete::OnlineStatus& status, const Kopete::StatusMessage &reason )
{
	if(status.status()== Kopete::OnlineStatus::Offline)
		disconnect();
	else if ( m_notifySocket )
	{
		// m_notifySocket->setStatus( status );
		//setPersonalMessage( reason );
	}
	else
	{
		kDebug( 14140 ) << k_funcinfo << "start connecting !!" << endl;
		m_connectstatus = status;
		connect( status );
	}
}


void QQAccount::setStatusMessage(const Kopete::StatusMessage& statusMessage)
{
	/* Not implemented in qq */
}


bool QQAccount::createContact(const QString& contactId, Kopete::MetaContact* parentContact)
{
	kDebug( 14140 ) << k_funcinfo << endl;
	QQContact* newContact = new QQContact( this, contactId, parentContact );
	return newContact != 0L;
	
}

QQChatSession * QQAccount::findChatSessionByGuid( const QString& guid )
{
	QQChatSession * chatSession = 0;
	QList<QQChatSession *>::const_iterator it;
	for ( it = m_chatSessions.begin(); it != m_chatSessions.end(); ++it )
	{
		if ( (*it)->guid() == guid )
		{
				chatSession = *it;
				break;
		}
	}
	return chatSession;
}


QQChatSession * QQAccount::chatSession( Kopete::ContactPtrList others, const QString& guid, Kopete::Contact::CanCreateFlags canCreate )
{
	QQChatSession * chatSession = 0;
	do // one iteration misuse of do...while to enable an easy drop-out once we locate a manager
	{
		// do we have a manager keyed by GUID?
		if ( !guid.isEmpty() )
		{
			chatSession = findChatSessionByGuid( guid );
			if ( chatSession )
			{
					kDebug( 14140 ) << k_funcinfo << " found a message manager by GUID: " << guid << endl;
					break;
			}
		}
		// does the factory know about one, going on the chat members?
		chatSession = dynamic_cast<QQChatSession*>(
				Kopete::ChatSessionManager::self()->findChatSession( myself(), others, protocol() ) );
		if ( chatSession )
		{
			kDebug( 14140 ) << k_funcinfo << " found a message manager by members with GUID: " << chatSession->guid() << endl;
			// re-add the returning contact(s) (very likely only one) to the chat
			Kopete::ContactPtrList::const_iterator returningContact;
			for ( returningContact = others.begin(); returningContact != others.end(); returningContact++ )
					chatSession->joined( static_cast<QQContact*> ( *returningContact ) );

			if ( !guid.isEmpty() )
				chatSession->setGuid( guid );
			break;
		}
		// we don't have an existing message manager for this chat, so create one if we may
		if ( canCreate )
		{
			chatSession = new QQChatSession( myself(), others, protocol(), guid );
			kDebug( 14140 ) << k_funcinfo <<
					" created a new message manager with GUID: " << chatSession->guid() << endl;
			m_chatSessions.append( chatSession );
			// listen for the message manager telling us that the user
			//has left the conference so we remove it from our map
			QObject::connect( chatSession, SIGNAL( leavingConference( QQChatSession * ) ),
							SLOT( slotLeavingConference( QQChatSession * ) ) );
			break;
		}
		//kDebug( GROUPWISE_DEBUG_GLOBAL ) << k_funcinfo <<
		//		" no message manager available." << endl;
	}
	while ( 0 );
	return chatSession;
}


void QQAccount::sendMessage(const QString& guid, Kopete::Message& message )
{
	kDebug(14140) << k_funcinfo << "Sending the message to " << guid << endl;
	// TODO: Add font style, font color, font size, font family here
	// translate the QT font to Eva::Font.
	// Currently, just send the plain text.

	// TODO: implement autoreply, font, color
	uint to = message.to().first()->contactId().toUInt();
	// TODO: use guid for the conference
	// TODO: use to for the conversation
	// TODO: Add codec to the member variable, to improve the preformance.
	QTextCodec* m_codec = QTextCodec::codecForName("GB18030");
	QByteArray text = m_codec->fromUnicode( message.plainBody() );
	notifySocket()->sendTextMessage(to, text );
}

void QQAccount::sendInvitation(const QString& guid, const QString& id, const QString& message )
{
	kDebug(14140) << k_funcinfo << "Sending the invitation to" << id << " for group(" << guid  << "):" << message << endl;
}

void QQAccount::slotStatusChanged( const Kopete::OnlineStatus &status )
{
	myself()->setOnlineStatus( status );

	if(m_newContactList)
	{
		// m_newContactList = false;
		// Fetch the group names, later
		m_notifySocket->sendGetGroupNames();
		m_notifySocket->sendDownloadGroups();

		// Fetch the ContactList from the server.
		// m_notifySocket->sendContactList();
		// Fetch the relation of contact <--> group

	
	}
}

void QQAccount::slotGroupNamesListed(const QStringList& ql )
{
	kDebug ( 14210 ) << k_funcinfo << ql << endl;
	// Create the groups if necessary:
	QList<Kopete::Group*> groupList = Kopete::ContactList::self()->groups();
	Kopete::Group *g;
	Kopete::Group *fallback;

	// add the default group as #0 group.
	m_groupList += Kopete::Group::topLevel();
	
	for( QStringList::const_iterator it = ql.begin(); it != ql.end(); it++ )
	{
		foreach(g, groupList)
		{
			if( g->displayName() == *it )
				fallback = g;
			else
			{
				fallback = new Kopete::Group( *it );
				Kopete::ContactList::self()->addGroup( fallback );
			}
			m_groupList += fallback;
		}
	}
}

void QQAccount::slotShowVideo ()
{
	kDebug ( 14210 ) << k_funcinfo << endl;

	if (isConnected ())
		QQWebcamDialog *qqWebcamDialog = new QQWebcamDialog(0, 0);
	updateContactStatus();
}


void QQAccount::slotNewContactList()
{
	kDebug ( 14210 ) << k_funcinfo << endl;
	// remove the allow list.
	// TODO: cleanup QQAccount variables.
	KConfigGroup *config=configGroup();
	// config->writeEntry( "allowList" , QString() );

		// clear all date information which will be received.
		// if the information is not anymore on the server, it will not be received
		foreach ( Kopete::Contact *kc , contacts() )
		{
			QQContact *c = static_cast<QQContact *>( kc );
			c->setBlocked( false );
			c->setAllowed( false );
			c->setReversed( false );
			c->setDeleted( true );
			c->setInfo( "PHH", QString::null );
			c->setInfo( "PHW", QString::null );
			c->setInfo( "PHM", QString::null );
			// c->removeProperty( QQProtocol::protocol()->propGuid );
		}
		m_newContactList=true;
}

void QQAccount::slotContactInGroup(const int qqId, const char type, const int groupId )
{
	kDebug ( 14210 ) << k_funcinfo << endl;
	QString id = QString::number( qqId );
	QQContact *c = static_cast<QQContact *>( contacts()[ id ] );
	if( c )
		; // exited contact.
	else
	{
		Kopete::MetaContact *metaContact = new Kopete::MetaContact();
		c = new QQContact( this, id, metaContact );
		c->setOnlineStatus( QQProtocol::protocol()->Offline );
		Kopete::ContactList::self()->addMetaContact( metaContact );
		metaContact->addToGroup( m_groupList[groupId] );
	}
}
	
void QQAccount::slotContactListed( const Eva::ContactInfo& ci )
{
	// ignore also the myself contact.
	QString id = QString::number( ci.id );
	QString publicName = QString( QByteArray( ci.nick.data(), ci.nick.size() ) );

	if ( id == accountId() )
		return;

	QQContact *c = static_cast<QQContact *>( contacts()[ id ] );
	if( c )
		; // exited contact.
	else
	{
		Kopete::MetaContact *metaContact = new Kopete::MetaContact();
			
		c = new QQContact( this, id, metaContact );
		c->setOnlineStatus( QQProtocol::protocol()->Offline );
			
		if(!publicName.isEmpty() )
			c->setProperty( Kopete::Global::Properties::self()->nickName() , publicName );
		else
			c->removeProperty( Kopete::Global::Properties::self()->nickName() );
			
		Kopete::ContactList::self()->addMetaContact( metaContact );
	}

	return ;
}


void QQAccount::slotContactStatusChanged(const Eva::ContactStatus& cs)
{
	kDebug(14210) << k_funcinfo << "qqId = " << cs.qqId << " from " << cs.ip << ":" << cs.port << " status = " << cs.status << endl;

	QQContact* c = static_cast<QQContact*> (contacts()[ QString::number( cs.qqId ) ]);
	kDebug( 14140 ) << "get the status from " << cs.qqId << endl;
	if (c) 
		c->setOnlineStatus( fromEvaStatus(cs.status) );
}


Kopete::OnlineStatus QQAccount::fromEvaStatus( char es )
{
	Kopete::OnlineStatus status;
	switch( es )
	{
		case Eva::Online : 
			status = Kopete::OnlineStatus( Kopete::OnlineStatus::Online );
			break;

			case Eva::Offline:
				status = Kopete::OnlineStatus( Kopete::OnlineStatus::Offline );
			break;

		case Eva::Away:
			status = Kopete::OnlineStatus( Kopete::OnlineStatus::Away );
			break;

		case Eva::Invisible:
			status = Kopete::OnlineStatus( Kopete::OnlineStatus::Invisible );
			break;
	}
	return status;
}

void QQAccount::updateContactStatus()
{
	QHashIterator<QString, Kopete::Contact*>itr( contacts() );
	for ( ; itr.hasNext(); ) {
		itr.next();
		itr.value()->setOnlineStatus( myself()->onlineStatus() );
	}
}

void QQAccount::receivedMessage( const QString &message )
{
	// Look up the contact the message is from
	QString from;
	QQContact* messageSender;

	from = message.section( ':', 0, 0 );
	Kopete::Contact* contact = contacts()[from];
	messageSender = dynamic_cast<QQContact *>( contact );

	kDebug( 14210 ) << k_funcinfo << " got a message from " << from << ", " << messageSender << ", is: " << message << endl;
	// Pass it on to the contact to process and display via a KMM
	if ( messageSender )
		;
		// messageSender->receivedMessage( message );
	else
		kWarning(14210) << k_funcinfo << "unable to look up contact for delivery" << endl;
}

void QQAccount::slotMessageReceived( const Eva::MessageHeader& header, const Eva::ByteArray& message )
{
	QString from = QString::number(header.sender);
	QString to = QString::number(header.receiver);
	QString msg ( QByteArray(message.data(), message.size()) );
	QDateTime timestamp;
	timestamp.setTime_t(header.timestamp);

	QQContact* sender = static_cast<QQContact*>( contacts()[from] );
	Kopete::ContactPtrList contactList;
	contactList.append( sender );
	QString guid = to +  ":" + from;

	QQChatSession* sess = chatSession( contactList, guid, Kopete::Contact::CanCreate );
	Q_ASSERT( sess );
	Kopete::Message * newMessage = 
			new Kopete::Message( timestamp, sender, contactList, msg,
								 Kopete::Message::Inbound, Kopete::Message::PlainText );
	sess->appendMessage( *newMessage );
}




#include "qqaccount.moc"
