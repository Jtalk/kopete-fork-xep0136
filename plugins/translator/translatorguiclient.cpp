/*
    translatorguiclient.cpp

    Kopete Translator plugin

    Copyright (c) 2003 by Olivier Goffart <ogoffart@tiscalinet.be>

    Kopete    (c) 2003 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include <qvariant.h>

#include <kdebug.h>
#include <kaction.h>
#include <klocale.h>

#include "kopetemessagemanager.h"
#include "kopeteview.h"
#include "kopetecontact.h"
#include "kopetemetacontact.h"
#include "kopetemessage.h"

#include "translatorplugin.h"
#include "translatorguiclient.h"
#include "translatorlanguages.h"

TranslatorGUIClient::TranslatorGUIClient( KopeteMessageManager *parent, const char *name )
: QObject( parent, name ) , KXMLGUIClient(parent)
{
	connect(TranslatorPlugin::plugin() , SIGNAL( destroyed(QObject*)) , this , SLOT(deleteLater()));

	m_manager=parent;

	//KAction *actionTranslate =
	new KAction( i18n ("Translate"), "locale" , CTRL+Key_T , this, SLOT( slotTranslateChat() ), actionCollection(), "translateCurrentMessage" );

	setXMLFile("translatorchatui.rc");


}

TranslatorGUIClient::~TranslatorGUIClient()
{

}


void TranslatorGUIClient::slotTranslateChat()
{
	if(!m_manager->view())
		return;

	//make sur the kmm will not be deleted when we perform such as actions
	//m_manager->setCanBeDeleted( false );  //FIXME: what was the state before, we have to reset the previous state after

	KopeteMessage msg = m_manager->view()->currentMessage();
	QString body=msg.plainBody();
	if(body.isEmpty())
		return;

	QString src_lang = TranslatorPlugin::plugin()->m_myLang;
	QString dst_lang;

	QPtrList<KopeteContact> list=m_manager->members();
	KopeteMetaContact *to = list.first()->metaContact();
	dst_lang = to->pluginData( TranslatorPlugin::plugin() , "languageKey" );
	if( dst_lang.isEmpty() || dst_lang == "null" )
	{
		kdDebug(14308) << "TranslatorPlugin::slotTranslateChat :  Cannot determine dst Metacontact language (" << to->displayName() << ")" << endl;
		return;
	}
	if ( src_lang == dst_lang )
	{
		kdDebug(14308) << "TranslatorPlugin::slotTranslateChat :  Src and Dst languages are the same" << endl;
		return;
	}

	/* We search for src_dst */

	QStringList s = TranslatorPlugin::plugin()->m_languages->supported( TranslatorPlugin::plugin()->m_service );
	QStringList::ConstIterator i;

	for ( i = s.begin(); i != s.end() ; ++i )
	{
		if ( *i == src_lang + "_" + dst_lang )
		{
			TranslatorPlugin::plugin()->translateMessage( body , src_lang, dst_lang , this , SLOT(messageTranslated(const QVariant&)));
			return;
		}
	}
	kdDebug(14308) << "TranslatorPlugin::slotTranslateChat : "<< src_lang + "_" + dst_lang << " doesn't exists with service " << TranslatorPlugin::plugin()->m_service << endl;
}

void TranslatorGUIClient::messageTranslated(const QVariant& result)
{
	QString translated=result.toString();
	if(translated.isEmpty())
	{
		kdDebug(14308) << "TranslatorPlugin::slotTranslateChat : empty string returned"  << endl;
		return;
	}
	//if the user close the window before the translation arrive, return
	if(!m_manager->view())
		return;

	KopeteMessage msg = m_manager->view()->currentMessage();
	msg.setBody(translated);
	m_manager->view()->setCurrentMessage(msg);
}


#include "translatorguiclient.moc"

// vim: set noet ts=4 sts=4 sw=4:

