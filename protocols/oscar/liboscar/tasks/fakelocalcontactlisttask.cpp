/*
   Kopete Oscar Protocol
   fakelocalcontactlisttask.cpp

   Copyright (c) 2005 Jan Ritzerfeld <kde@bugs.jan.ritzerfeld.net>
   Copyright (c) 2004 Matt Rogers <mattr@kde.org>

   Kopete (c) 2002-2005 by the Kopete developers <kopete-devel@kde.org>

   *************************************************************************
   *                                                                       *
   * This library is free software; you can redistribute it and/or         *
   * modify it under the terms of the GNU Lesser General Public            *
   * License as published by the Free Software Foundation; either          *
   * version 2 of the License, or (at your option) any later version.      *
   *                                                                       *
   *************************************************************************
*/
#include "fakelocalcontactlisttask.h"

#include <kdebug.h>
#include "buffer.h"
#include "connection.h"
#include "contactmanager.h"
#include "oscarsettings.h"
#include "oscartypes.h"
#include "oscarutils.h"
#include "transfer.h"
#include <QList>

using namespace Oscar;

FakeLocalContactListTask::FakeLocalContactListTask(Task* parent): Task(parent)
{
}


FakeLocalContactListTask::~FakeLocalContactListTask()
{
}


void FakeLocalContactListTask::onGo()
{
	if( !client()->settings()->respectRequireAuth() )
	{
		FLAP f = { 0x02, client()->flapSequence(), 0 };
		SNAC s = { 0x0003, 0x0004, 0x0000, client()->snacSequence() };
		Buffer* buffer = new Buffer();
		
		kDebug( OSCAR_RAW_DEBUG ) << k_funcinfo << "Sending contact list" << endl;
		QList<Contact> contactList = client()->ssiManager()->contactList();
		QList<Contact>::const_iterator cEnd = contactList.constEnd();
		for ( QList<Contact>::const_iterator it = contactList.constBegin(); it != cEnd; ++it )
		{
			kDebug( OSCAR_RAW_DEBUG ) << "Adding contact " << ( *it ).name() << " to CLI_BUDDYLIST_ADD packet" << endl;
			buffer->addBUIN( ( *it ).name().toLatin1() );
		}
		
		Transfer* t = createTransfer( f, s, buffer );
		send( t );
	}
	
	setSuccess( 0, QString::null );
}

//kate: tab-width 4; indent-mode csands;
