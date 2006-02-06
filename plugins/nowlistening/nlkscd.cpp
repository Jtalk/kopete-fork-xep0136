/*
    nlkscd.cpp

    Kopete Now Listening To plugin

    Copyright (c) 2002,2003,2004 by Will Stephenson <will@stevello.free-online.co.uk>

    Kopete    (c) 2002,2003,2004 by the Kopete developers  <kopete-devel@kde.org>
	
	Purpose: 
	This class abstracts the interface to KsCD by
	implementing NLMediaPlayer

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include <kdebug.h>
#include <qstringlist.h>
//Added by qt3to4:
#include <QByteArray>

#include "nlmediaplayer.h"

#include "nlkscd.h"

NLKscd::NLKscd( DCOPClient *client ) : NLMediaPlayer()
{
	m_client = client;
	m_type = Audio;
	m_name = "KsCD";
}

void NLKscd::update()
{
	m_playing = false;
	QString newTrack;
	// see if it's registered with DCOP
	if ( m_client->isApplicationRegistered( "kscd" ) )
	{
		// see if it's playing
		QByteArray data, replyData;
		DCOPCString replyType;
		if ( !m_client->call( DCOPCString("kscd"), DCOPCString("CDPlayer"), DCOPCString("playing()"), data,
					replyType, replyData ) )
		{
			// we're talking to a KsCD without the playing() method
			m_playing = true;
//			kDebug( 14307 ) << "NLKscd::update() - KsCD without playing()"
//				<< endl;
		}
		else
		{
			QDataStream reply( &replyData,QIODevice::ReadOnly );
			reply.setVersion(QDataStream::Qt_3_1);
			if ( replyType == "bool" ) {
				reply >> m_playing;
//				kDebug( 14307 ) << "NLKscd::update() - KsCD is " <<
//					( m_playing ? "" : "not " ) << "playing!" << endl;
			}
		}
		// poll it for its current artist 
		if ( !m_client->call( "kscd", "CDPlayer",
					"currentArtist()", data, replyType, replyData ) )
			kDebug( 14307 ) <<  "NLKscd::update() DCOP error"
				<< endl;
		else {
			QDataStream reply( &replyData,QIODevice::ReadOnly );
			reply.setVersion(QDataStream::Qt_3_1);
			if ( replyType == "QString" )
				reply >> m_artist;
			else
				kDebug( 14307 ) << "NLKscd::update() trackList returned unexpected reply type!" << endl;
		}

		//album
		if ( !m_client->call( "kscd", "CDPlayer",
					"currentAlbum()", data, replyType, replyData ) )
			kDebug( 14307 ) <<  "NLKscd::update() DCOP error"
				<< endl;
		else {
			QDataStream reply( &replyData,QIODevice::ReadOnly );
			reply.setVersion(QDataStream::Qt_3_1);
			if ( replyType == "QString" )
				reply >> m_album;
			else
				kDebug( 14307 ) << "NLKscd::update() trackList returned unexpected reply type!" << endl;
		}

		// Get the current track title
		if ( !m_client->call( "kscd", "CDPlayer",
					"currentTrackTitle()", data, replyType, replyData ) )
			kDebug( 14307 ) << "NLKscd::update() - there was some error using DCOP." << endl;
		else {
			QDataStream reply( &replyData,QIODevice::ReadOnly );
			reply.setVersion(QDataStream::Qt_3_1);
			if ( replyType == "QString" ) {
				reply >> newTrack;
				//kDebug( 14307 ) << "the result is: " << newTrack.toLatin1()
				//	<< endl;
			} else
				kDebug( 14307 ) << "NLKscd::update()-  currentTrackTitle "
					<< "returned unexpected reply type!" << endl;
		}
		// if the current track title has changed
		if ( newTrack != m_track )
		{
			m_newTrack = true;
			m_track = newTrack;
		}
		else
			m_newTrack = false;
//		kDebug( 14307 ) << "NLKscd::update() - found kscd - "
//			<< m_track << endl;

	}
//	else
//		kDebug( 14307 ) << "NLKscd::update() - kscd not found" << endl;
}

// vim: set noet ts=4 sts=4 sw=4:
