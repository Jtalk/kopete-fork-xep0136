/*
    testbedcontact.cpp - Kopete Testbed Protocol

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

#include "testbedcontact.h"

#include <kaction.h>
#include <kdebug.h>
#include <klocale.h>

#include "kopeteaccount.h"
#include "kopetemessagemanagerfactory.h"
#include "kopetemetacontact.h"

#include "testbedaccount.h"
#include "testbedfakeserver.h"
#include "testbedprotocol.h"

TestbedContact::TestbedContact( KopeteAccount* _account, const QString &uniqueName, 
		const TestbedContactType type, const QString &displayName, KopeteMetaContact *parent )
: KopeteContact( _account, uniqueName, parent )
{
	kdDebug( 14210 ) << k_funcinfo << " uniqueName: " << uniqueName << ", displayName: " << displayName << endl;
	m_type = type;
	setDisplayName( displayName );
	m_msgManager = 0L;

	setOnlineStatus( TestbedProtocol::protocol()->testbedOffline );
}

TestbedContact::~TestbedContact()
{
}

bool TestbedContact::isReachable()
{
    return true;
}

void TestbedContact::serialize( QMap< QString, QString >& serializedData, QMap< QString, QString >& addressBookData )
{
    QString value;
	switch ( m_type )
	{
	case Null: 
		value = "null";
	case Echo:
		value = "echo";
	}
	serializedData["contactType"] = value;
}

KopeteMessageManager* TestbedContact::manager( bool )
{
	kdDebug( 14210 ) << k_funcinfo << endl;
	if ( m_msgManager )
	{
		return m_msgManager;
	}
	else
	{
		QPtrList<KopeteContact> contacts;
		contacts.append(this);
		m_msgManager = KopeteMessageManagerFactory::factory()->create(account()->myself(), contacts, protocol());
		connect(m_msgManager, SIGNAL(messageSent(KopeteMessage&, KopeteMessageManager*)),
				this, SLOT( sendMessage( KopeteMessage& ) ) );
		connect(m_msgManager, SIGNAL(destroyed()), this, SLOT(slotMessageManagerDestroyed()));
		return m_msgManager;
	}
}

KActionCollection *TestbedContact::customContextMenuActions()
{
	m_actionCollection = new KActionCollection( this, "userColl" );
	m_actionPrefs = new KAction(i18n( "&Contact Settings" ), 0, this, 
			SLOT( showContactSettings( )), m_actionCollection, "contactSettings" );

	return m_actionCollection;
}

void TestbedContact::showContactSettings()
{
	//TestbedContactSettings* p = new TestbedContactSettings( this );
	//p->show();
}

void TestbedContact::sendMessage( KopeteMessage &message )
{
	kdDebug( 14210 ) << k_funcinfo << endl;
	// convert to the what the server wants
	// For this 'protocol', there's nothing to do
	// send it
	static_cast<TestbedAccount *>( account() )->server()->sendMessage( 
			message.to().first()->contactId(), 
			message.plainBody() );
	// give it back to the manager to display
	manager()->appendMessage( message );
	// tell the manager it was sent successfully
	manager()->messageSucceeded();
}

void TestbedContact::receivedMessage( const QString &message )
{
	// Create a KopeteMessage 
	KopeteMessage *newMessage;
	KopeteContactPtrList contactList;
	account();
	contactList.append( account()->myself() );
	newMessage = new KopeteMessage( this, contactList, message, KopeteMessage::Inbound );
	
	// Add it to the manager
	manager()->appendMessage (*newMessage);

	delete newMessage;
}

void TestbedContact::slotMessageManagerDestroyed()
{
	//FIXME: the chat window was closed?  Take appropriate steps.
	m_msgManager = 0L;
}
