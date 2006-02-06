/*
   Kopete Oscar Protocol
   ssiauthtask.cpp - SSI Authentication Task

   Copyright (c) 2004 Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>

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

#include "ssiauthtask.h"
#include "ssimanager.h"
#include "transfer.h"
#include "buffer.h"
#include "connection.h"
#include "oscarutils.h"

#include <kdebug.h>

SSIAuthTask::SSIAuthTask( Task* parent )
	: Task( parent )
{
	m_manager = parent->client()->ssiManager();
}

SSIAuthTask::~SSIAuthTask()
{
}

bool SSIAuthTask::forMe( const Transfer* t ) const
{
	const SnacTransfer* st = dynamic_cast<const SnacTransfer*> ( t );
	
	if ( !st )
		return false;
		
	if ( st->snacService() != 0x0013 )
		return false;
		
	switch ( st->snacSubtype() )
	{
		case 0x0015: // Future authorization granted
		case 0x0019: // Authorization request
		case 0x001b: // Authorization reply
		case 0x001c: // "You were added" message
			return true;
			break;
		default:
			return false;
	}
}

bool SSIAuthTask::take( Transfer* t )
{
	if ( forMe( t ) )
	{
		setTransfer( t );
		SnacTransfer* st = dynamic_cast<SnacTransfer*> ( t );
		
		switch ( st->snacSubtype() )
		{
			case 0x0015: // Future authorization granted
				handleFutureAuthGranted();
				break;
			case 0x0019: // Authorization request
				handleAuthRequested();
				break;
			case 0x001b: // Authorization reply
				handleAuthReplied();
				break;
			case 0x001c: // "You were added" message
				handleAddedMessage();
				break;
		}
		setTransfer( 0 );
		return true;
	}
	return false;
}

void SSIAuthTask::grantFutureAuth( const QString& uin, const QString& reason )
{
	FLAP f = { 0x02, 0, 0 };
	SNAC s = { 0x0013, 0x0014, 0x0000, client()->snacSequence() };
	
	Buffer* buf = new Buffer();
	buf->addBUIN( uin.toLatin1() );
	buf->addBSTR( reason.toLatin1() );
	buf->addWord( 0x0000 ); // Unknown
	
	Transfer* t = createTransfer( f, s, buf );
	send( t );
}

void SSIAuthTask::sendAuthRequest( const QString& uin, const QString& reason )
{
	FLAP f = { 0x02, 0, 0 };
	SNAC s = { 0x0013, 0x0018, 0x0000, client()->snacSequence() };
	
	Buffer* buf = new Buffer();
	buf->addBUIN( uin.toLatin1() );
	buf->addBSTR( reason.toLatin1() );
	buf->addWord( 0x0000 ); // Unknown
	
	Transfer* t = createTransfer( f, s, buf );
	send( t );
	
	Oscar::SSI contact = m_manager->findContact( uin );
	if ( contact )
		contact.setWaitingAuth( true );
}

void SSIAuthTask::sendAuthReply( const QString& uin, const QString& reason, bool auth )
{
	FLAP f = { 0x02, 0, 0 };
	SNAC s = { 0x0013, 0x001A, 0x0000, client()->snacSequence() };
	
	Buffer* buf = new Buffer();
	buf->addBUIN( uin.toLatin1() );
	buf->addByte( auth ? 0x01 : 0x00 ); // accepted / declined
	buf->addBSTR( reason.toLatin1() );
	
	Transfer* t = createTransfer( f, s, buf );
	send( t );
}

void SSIAuthTask::handleFutureAuthGranted()
{
	Buffer* buf = transfer()->buffer();
	
	QString uin = Oscar::normalize( buf->getBUIN() );
	QString reason = buf->getBSTR();
	
	buf->getWord(); // 0x0000 - Unknown
	
	kDebug( OSCAR_RAW_DEBUG ) << k_funcinfo << "Future authorization granted from " << uin << endl;
	kDebug( OSCAR_RAW_DEBUG ) << k_funcinfo << "Reason: " << reason << endl;
	emit futureAuthGranted( uin, reason );
}

void SSIAuthTask::handleAuthRequested()
{
	Buffer* buf = transfer()->buffer();
	
	QString uin = Oscar::normalize( buf->getBUIN() );
	QString reason = buf->getBSTR();
	
	buf->getWord(); // 0x0000 - Unknown
	
	kDebug( OSCAR_RAW_DEBUG ) << k_funcinfo << "Authorization requested from " << uin << endl;
	kDebug( OSCAR_RAW_DEBUG ) << k_funcinfo << "Reason: " << reason << endl;
	
	emit authRequested( uin, reason );
}

void SSIAuthTask::handleAuthReplied()
{
	Buffer* buf = transfer()->buffer();
	
	QString uin = Oscar::normalize( buf->getBUIN() );
	bool accepted = buf->getByte();
	QString reason = buf->getBSTR();
	
	if ( accepted )
		kDebug( OSCAR_RAW_DEBUG ) << k_funcinfo << "Authorization request accepted by " << uin << endl;
	else
		kDebug( OSCAR_RAW_DEBUG ) << k_funcinfo << "Authorization request declined by " << uin << endl;
		
	kDebug( OSCAR_RAW_DEBUG ) << k_funcinfo << "Reason: " << reason << endl;
	
	Oscar::SSI sender = m_manager->findContact( uin );
	if ( sender )
		sender.setWaitingAuth( false );
		
	emit authReplied( uin, reason, accepted );
}

void SSIAuthTask::handleAddedMessage()
{
	Buffer* buf = transfer()->buffer();
	
	QString uin = Oscar::normalize( buf->getBUIN() );
	
	kDebug( OSCAR_RAW_DEBUG ) << k_funcinfo << "User " << uin << " added you to the contact list" << endl;
	emit contactAddedYou( uin );
}

#include "ssiauthtask.moc"
//kate: tab-width 4; indent-mode csands;
