/***************************************************************************
                          autoreplaceplugin.cpp  -  description
                             -------------------
    begin                : 20030425
    copyright            : (C) 2003 by Roberto Pariset
    email                : victorheremita@fastwebnet.it
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kgenericfactory.h>

#include "kopetemessagemanagerfactory.h"

#include "autoreplaceplugin.h"
#include "autoreplaceconfig.h"

typedef KGenericFactory<AutoReplacePlugin> AutoReplacePluginFactory;
K_EXPORT_COMPONENT_FACTORY( kopete_autoreplace, AutoReplacePluginFactory );
AutoReplacePlugin * AutoReplacePlugin::pluginStatic_ = 0L;

AutoReplacePlugin::AutoReplacePlugin( QObject *parent, const char * name, const QStringList & )
: KopetePlugin( AutoReplacePluginFactory::instance(), parent, name )
{
	if( !pluginStatic_ )
		pluginStatic_ = this;

	m_prefs = new AutoReplaceConfig;

	connect( KopeteMessageManagerFactory::factory(), SIGNAL( aboutToSend( KopeteMessage & ) ),
		this, SLOT( slotAboutToSend( KopeteMessage & ) ) );

	connect( this, SIGNAL( settingsChanged() ), this, SLOT( slotSettingsChanged() ) );
}

AutoReplacePlugin::~AutoReplacePlugin()
{
	pluginStatic_ = 0L;

	delete m_prefs;
}

AutoReplacePlugin * AutoReplacePlugin::plugin()
{
	return pluginStatic_ ;
}

void AutoReplacePlugin::slotSettingsChanged()
{
	m_prefs->load();
}

void AutoReplacePlugin::slotAboutToSend( KopeteMessage &msg )
{
	if ( ( msg.direction() == KopeteMessage::Outbound && m_prefs->autoReplaceOutgoing() ) ||
		( msg.direction() == KopeteMessage::Inbound && m_prefs->autoReplaceIncoming() ) )
	{
		QString replaced_message = msg.plainBody();
		AutoReplaceConfig::WordsToReplace map = m_prefs->map();

		// replaces all matched words --> try to find a more 'economic' way
		QString match = "\\b(%1)\\b";
		AutoReplaceConfig::WordsToReplace::Iterator it;
		for ( it = map.begin(); it != map.end(); ++it )
			replaced_message.replace( QRegExp( match.arg( QRegExp::escape( it.key() ) ) ), map.find( it.key() ).data() );

		// the message is now the one with replaced words
		msg.setBody( replaced_message, KopeteMessage::PlainText );
	}

	if( msg.direction() == KopeteMessage::Outbound )
	{
		if ( m_prefs->dotEndSentence() )
		{
			QString replaced_message = msg.plainBody();
			// eventually add . at the end of the lines, sent lines only
			replaced_message.replace( QRegExp( "([a-z])$" ), "\\1." );
			// replaced_message.replace(QRegExp( "([\\w])$" ), "\\1." );
			
			// the message is now the one with replaced words
			msg.setBody( replaced_message, KopeteMessage::PlainText );
		}

		if( m_prefs->capitalizeBeginningSentence() )
		{
			QString replaced_message = msg.plainBody();
			// eventually start each sent line with capital letter
			// TODO 	". "	 "? "	 "! " 
			replaced_message[ 0 ] = replaced_message.at( 0 ).upper();
			
			// the message is now the one with replaced words
			msg.setBody( replaced_message, KopeteMessage::PlainText );
		}
	}
}

#include "autoreplaceplugin.moc"

// vim: set noet ts=4 sts=4 sw=4:

