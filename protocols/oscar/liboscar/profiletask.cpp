/*
  Kopete Oscar Protocol
  profiletask.h - Update the user's profile on the server

  Copyright (c) 2004 Matt Rogers <mattr@kde.org>

  Kopete (c) 2002-2004 by the Kopete developers <kopete-devel@kde.org>

  *************************************************************************
  *                                                                       *
  * This library is free software; you can redistribute it and/or         *
  * modify it under the terms of the GNU Lesser General Public            *
  * License as published by the Free Software Foundation; either          *
  * version 2 of the License, or (at your option) any later version.      *
  *                                                                       *
  *************************************************************************
*/

#include "profiletask.h"

#include <qstring.h>
#include <kdebug.h>

#include "transfer.h"
#include "connection.h"
#include "oscartypes.h"
#include "oscarutils.h"

using namespace Oscar;

ProfileTask::ProfileTask( Task* parent )
		: Task( parent )
{
}


ProfileTask::~ProfileTask()
{
}


bool ProfileTask::forMe( const Transfer* transfer ) const
{
	Q_UNUSED( transfer );
	return false;
}

bool ProfileTask::take( Transfer* transfer )
{
	Q_UNUSED( transfer );
	return false;
}

void ProfileTask::onGo()
{
	sendProfileUpdate();
}

void ProfileTask::setProfileText( const QString& text )
{
	m_profileText = text;
}

void ProfileTask::setAwayMessage( const QString& text )
{
	m_awayMessage = text;
}

void ProfileTask::sendProfileUpdate()
{
	kDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "SEND (CLI_SETUSERINFO/CLI_SET_LOCATION_INFO)" << endl;
	FLAP f = { 0x02, 0, 0 };
	SNAC s = { 0x0002, 0x0004, 0x0000, client()->snacSequence() };
	Buffer *buffer = new Buffer();
	Buffer capBuf;


	if ( client()->isIcq() )
	{
		capBuf.addGuid( oscar_caps[CAP_ICQSERVERRELAY] ); // we support type-2 messages
		capBuf.addGuid( oscar_caps[CAP_ISICQ] ); // I think this is an icq client, but maybe I'm wrong
		//capBuf.addString( oscar_caps[CAP_RTFMSGS], 16 ); // we do incoming RTF messages
	}
	else
	{
		if ( !m_profileText.isNull() )
		{
			static const QString defencoding = "text/aolrtf; charset=\"us-ascii\"";
			buffer->addTLV(0x0001, defencoding.toLatin1());
			buffer->addTLV(0x0002, m_profileText.toLocal8Bit());
			kDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "setting profile = " << m_profileText << endl;
		}

		if ( !m_awayMessage.isNull() )
		{
			static const QString defencoding = "text/aolrtf; charset=\"us-ascii\"";
			buffer->addTLV(0x0003, defencoding.toLatin1());
			buffer->addTLV(0x0004, m_awayMessage.toLocal8Bit());
			kDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "setting away message = " << m_awayMessage << endl;
		}
		capBuf.addGuid( oscar_caps[CAP_SENDFILE] ); // we can do filetransfers! :)
	}
	capBuf.addGuid( oscar_caps[CAP_UTF8] ); // we can send/receive UTF encoded messages
	capBuf.addGuid( oscar_caps[CAP_KOPETE] ); // we are the borg, resistance is futile
	capBuf.addGuid( oscar_caps[CAP_TYPING] ); // we know you're typing something to us!
	capBuf.addGuid( oscar_caps[CAP_BUDDYICON] ); //can you take my picture?

	kDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "adding capabilities, size=" << capBuf.length() << endl;
	buffer->addTLV(0x0005, capBuf.buffer());
	Transfer* st = createTransfer( f, s , buffer );
	send( st );
	setSuccess( 0, QString::null );
	kDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "done." << endl;
}

//kate: tab-width 4; indent-mode csands;
