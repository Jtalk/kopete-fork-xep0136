/*
    testbedaccount.cpp - Kopete Testbed Protocol

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

#include "testbedaccount.h"

#include <kaction.h>
#include <kdebug.h>
#include <klocale.h>
#include <kpopupmenu.h>

#include "kopetemetacontact.h"

#include "testbedcontact.h"
#include "testbedfakeserver.h"
#include "testbedprotocol.h"


TestbedAccount::TestbedAccount( TestbedProtocol *parent, const QString& accountID, const char *name )
: KopeteAccount ( parent, accountID , name )
{
	// Init the myself contact
	// FIXME: I think we should add a global self metaContact (Olivier)
	m_myself = new TestbedContact( this, accountId(), TestbedContact::Null, accountId(), 0L, KopeteContact::OmitFromKABC );
	m_myself->setOnlineStatus( TestbedProtocol::protocol()->testbedOffline );
	m_server = new TestbedFakeServer();;
}

TestbedAccount::~TestbedAccount()
{
	delete m_server;
}

KActionMenu* TestbedAccount::actionMenu()
{
	KActionMenu *theActionMenu = new KActionMenu(accountId(), myself()->onlineStatus().iconFor(this) , this);
	theActionMenu->popupMenu()->insertTitle(m_myself->icon(), i18n("Testbed (%1)").arg(accountId()));
	// NEED FSCKING GO ONLINE OFFLINE ACTIONS HERE!
	theActionMenu->insert(new KAction (TestbedProtocol::protocol()->testbedOnline.caption(),
		TestbedProtocol::protocol()->testbedOnline.iconFor(this), 0, this, SLOT (slotGoOnline ()), this,
		"actionTestbedConnect"));

	theActionMenu->insert(new KAction (TestbedProtocol::protocol()->testbedAway.caption(),
		TestbedProtocol::protocol()->testbedAway.iconFor(this), 0, this, SLOT (slotGoAway ()), this,
		"actionTestbedAway"));

	theActionMenu->insert(new KAction (TestbedProtocol::protocol()->testbedOffline.caption(),
		TestbedProtocol::protocol()->testbedOffline.iconFor(this), 0, this, SLOT (slotGoOffline ()), this,
		"actionTestbedOfflineDisconnect"));
	
	return theActionMenu;
}

KopeteContact* TestbedAccount::myself() const
{
    return m_myself;
}

bool TestbedAccount::addContactToMetaContact(const QString& contactId, const QString& displayName, KopeteMetaContact* parentContact)
{
	kdDebug ( 14210 ) << k_funcinfo << "contactId: " << contactId << " displayName: " << displayName
			<< endl;
	TestbedContact* newContact = new TestbedContact( this, contactId, TestbedContact::Echo, displayName, parentContact, 
													 KopeteContact::AddToKABC );
	return newContact != 0L;
}

void TestbedAccount::setAway( bool away, const QString& reason )
{
	if ( away )
		slotGoAway();
	else
		slotGoOnline();
}

void TestbedAccount::connect()
{
	kdDebug ( 14210 ) << k_funcinfo << endl;
	m_myself->setOnlineStatus( TestbedProtocol::protocol()->testbedOnline );
	QObject::connect ( m_server, SIGNAL ( messageReceived( const QString & ) ),
			this, SLOT ( receivedMessage( const QString & ) ) );
}

void TestbedAccount::disconnect()
{
	kdDebug ( 14210 ) << k_funcinfo << endl;
	m_myself->setOnlineStatus( TestbedProtocol::protocol()->testbedOffline );
	QObject::disconnect ( m_server, 0, 0, 0 );
}

TestbedFakeServer * TestbedAccount::server()
{
	return m_server;
}

void TestbedAccount::slotGoOnline ()
{
	kdDebug ( 14210 ) << k_funcinfo << endl;

	if (!isConnected ())
		connect ();
	else
		m_myself->setOnlineStatus( TestbedProtocol::protocol()->testbedOnline );
	updateContactStatus();
}
void TestbedAccount::slotGoAway ()
{
	kdDebug ( 14210 ) << k_funcinfo << endl;

	if (!isConnected ())
		connect();
	
	m_myself->setOnlineStatus( TestbedProtocol::protocol()->testbedAway );
	updateContactStatus();
}


void TestbedAccount::slotGoOffline ()
{
	kdDebug ( 14210 ) << k_funcinfo << endl;

	if (isConnected ())
		disconnect ();
	updateContactStatus();
}

void TestbedAccount::receivedMessage( const QString &message )
{
	// Look up the contact the message is from
	QString from;
	TestbedContact* messageSender;
	
	from = message.section( ':', 0, 0 );
	//from = QString::fromLatin1("echo");
	messageSender = static_cast<TestbedContact *>( contacts ()[ from ] );
	
	kdDebug( 14210 ) << k_funcinfo << " got a message from " << from << ", " << messageSender << ", is: " << message << endl;
	// Pass it on to the contact to process and display via a KMM
	messageSender->receivedMessage( message );
}

void TestbedAccount::updateContactStatus()
{
	QDictIterator<KopeteContact> itr( contacts() );
	for ( ; itr.current(); ++itr )
		itr.current()->setOnlineStatus( m_myself->onlineStatus() );
}

