/*
    nowlisteningplugin.cpp

    Kopete Now Listening To plugin

    Copyright (c) 2002,2003 by Will Stephenson <will@stevello.free-online.co.uk>

    Kopete    (c) 2002,2003 by the Kopete developers  <kopete-devel@kde.org>

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
#include <kgenericfactory.h>
#include <kapplication.h>
#include <dcopclient.h>
#include <kaction.h>

#include "config.h"
#include "kopetemessagemanagerfactory.h"
#include "kopetemetacontact.h"
#include "nowlisteningpreferences.h"
#include "nowlisteningplugin.h"
#include "nlmediaplayer.h"
#include "nlkscd.h"
#include "nlnoatun.h"
#include "nljuk.h"
#include "nowlisteningguiclient.h"

#ifdef HAVE_XMMS
#include "nlxmms.h"
#endif

typedef KGenericFactory<NowListeningPlugin> NowListeningPluginFactory;
K_EXPORT_COMPONENT_FACTORY( kopete_nowlistening, NowListeningPluginFactory );

NowListeningPlugin::NowListeningPlugin( QObject *parent, const char *name, const QStringList & /*args*/ )
: KopetePlugin( NowListeningPlugin::instance(), parent, name )
{
	if ( pluginStatic_ )
		kdDebug(14307)<<"####"<<"Now Listening already initialized"<<endl;
	else
		pluginStatic_ = this;

	kdDebug(14307) << k_funcinfo << endl;
	// make these pointers safe until init'd
	m_actionCollection = 0L;
	m_actionWantsAdvert = 0L;
	m_currentMetaContact = 0L;
	m_currentMessageManager = 0L;

	// initialise preferences
	m_prefs = new NowListeningPreferences( "kaboodle", this );

	connect( KopeteMessageManagerFactory::factory(), SIGNAL(
			messageManagerCreated( KopeteMessageManager * )) , SLOT( slotNewKMM(
			KopeteMessageManager * ) ) );

	QIntDict<KopeteMessageManager> sessions =
			KopeteMessageManagerFactory::factory()->sessions();
	QIntDictIterator<KopeteMessageManager> it( sessions );
	for ( ; it.current() ; ++it )
		slotNewKMM( it.current() );

	// get a pointer to the dcop client
	m_client = kapp->dcopClient(); //new DCOPClient();

	// set up known media players
	m_mediaPlayer = new QPtrList<NLMediaPlayer>;
	m_mediaPlayer->setAutoDelete( true );
	m_mediaPlayer->append( new NLKscd( m_client ) );
	m_mediaPlayer->append( new NLNoatun( m_client ) );
	m_mediaPlayer->append( new NLJuk( m_client ) );

#ifdef HAVE_XMMS
	m_mediaPlayer->append( new NLXmms() );
#endif

	// watch for '/media' getting typed
	connect(  KopeteMessageManagerFactory::factory(),
			SIGNAL(  aboutToSend(  KopeteMessage & ) ),
			SLOT(  slotOutgoingMessage(  KopeteMessage & ) ) );
}

NowListeningPlugin::~NowListeningPlugin()
{
	kdDebug(14307) << k_funcinfo << endl;

	delete m_mediaPlayer;

	pluginStatic_ = 0L;
}

void NowListeningPlugin::slotNewKMM(KopeteMessageManager *KMM)
{
	new NowListeningGUIClient(KMM);
}

NowListeningPlugin* NowListeningPlugin::plugin()
{
	return pluginStatic_ ;
}

void NowListeningPlugin::slotOutgoingMessage( KopeteMessage& msg )
{
	QString originalBody = msg.plainBody();
	// look for messages that we've generated and ignore them
	if ( !originalBody.startsWith( m_prefs->header() ) )
	{
		// look for the string '/media'
		if ( originalBody.startsWith( "/media" ) )
		{
			// replace it with media advert
			QString newBody = allPlayerAdvert() + originalBody.right( 
					originalBody.length() - 6 );
			msg.setBody( newBody, KopeteMessage::RichText );
		}	
		return;
	}
}

QString NowListeningPlugin::allPlayerAdvert() const
{
	// generate message for all players
	QString message = "";
	QString perTrack = m_prefs->perTrack();
	
	for ( NLMediaPlayer* i = m_mediaPlayer->first(); i; i = m_mediaPlayer->next() )
	{
		i->update();
		if ( i->playing() )
		{
			kdDebug(14307) << k_funcinfo << i->name() << " is playing" << endl;
			if ( message.isEmpty() )
				message = m_prefs->header();
				
			if (  message != m_prefs->header() ) // > 1 track playing!
				message = message + m_prefs->conjunction();
			message = message + substDepthFirst( i, perTrack, false );
		}
	}
	kdDebug(14307) << k_funcinfo << message << endl;
	return message;
}

QString NowListeningPlugin::substDepthFirst( NLMediaPlayer *player,
		QString in, bool inBrackets ) const
{
	QString track = player->track();
	QString artist = player->artist();
	QString album = player->album();
	QString playerName = player->name();
	
	for ( unsigned int i = 0; i < in.length(); i++ )
	{
		QChar c = in.at( i );
		//kdDebug(14307) << "Now working on:" << in << " char is: " << c << endl;
		if ( c == '(' )
		{
			// find matching bracket
			int depth = 0;
			//kdDebug(14307) << "Looking for ')'" << endl;
			for ( unsigned int j = i + 1; j < in.length(); j++ )
			{
				QChar d = in.at( j );
				//kdDebug(14307) << "Got " << d << endl;
				if ( d == '(' )
					depth++;
				if ( d == ')' )
				{
					// have we found the match?
					if ( depth == 0 )
					{
						// recursively replace contents of matching ()
						QString substitution = substDepthFirst( player,
								in.mid( i + 1, j - i - 1), true ) ;
						in.replace ( i, j - i + 1, substitution );
						// perform substitution and return the result
						i = i + substitution.length() - 1;
						break;
					}
					else
						depth--;
				}
			}
		}
	}
	// no () found, perform substitution!
	// get each string (to) to substitute for (from)
	bool done = false;
	if ( in.contains ( "%track" ) )
	{
		if ( track.isEmpty() )
			track = i18n("Unknown track");

		in.replace( "%track", track );
		done = true;
	}

	if ( in.contains ( "%artist" ) && !artist.isEmpty() )
	{
		if ( artist.isEmpty() )
			artist = i18n("Unknown artist");
		in.replace( "%artist", artist );
		done = true;
	}
	if ( in.contains ( "%album" ) && !album.isEmpty() )
	{
		if ( album.isEmpty() )
			album = i18n("Unknown album");
		in.replace( "%album", album );
		done = true;
	}
	if ( in.contains ( "%player" ) && !playerName.isEmpty() )
	{
		if ( playerName.isEmpty() )
			playerName = i18n("Unknown player");
		in.replace( "%player", playerName );
		done = true;
	}
	// make whether we return anything dependent on whether we
	// were in brackets and if we were, if a substitution was made.
	if ( inBrackets && !done )
		return "";
	else
		return in;
}

void NowListeningPlugin::advertiseToChat( KopeteMessageManager *theChat, QString message )
{
	KopeteContactPtrList pl = theChat->members();

	// get on with it
	kdDebug(14307) << "NowListeningPlugin::advertiseNewTracks() - " <<
		( pl.isEmpty() ? "has no " : "has " ) << "interested recipients: " << endl;
	for ( pl.first(); pl.current(); pl.next() )
		kdDebug(14307) << "NowListeningPlugin::advertiseNewTracks() " << pl.current()->displayName() << endl;
	// if no-one in this KMM wants to be advertised to, don't send
	// any message
	if ( pl.isEmpty() )
		return;
	KopeteMessage msg( theChat->user(),
			pl,
			message,
			KopeteMessage::Outbound,
			KopeteMessage::RichText );
	theChat->sendMessage( msg );
}

NowListeningPlugin* NowListeningPlugin::pluginStatic_ = 0L;

#include "nowlisteningplugin.moc"

// vim: set noet ts=4 sts=4 sw=4:
