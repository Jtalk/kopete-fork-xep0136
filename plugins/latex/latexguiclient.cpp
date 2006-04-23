/*
    latexguiclient.cpp

    Kopete Latex plugin

    Copyright (c) 2003-2005 by Olivier Goffart <ogoffart @ kde.org>

    Kopete    (c) 2003-2005 by the Kopete developers  <kopete-devel@kde.org>

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

#include <kaction.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <qimage.h>
#include <qregexp.h>

#include "kopetechatsession.h"
#include "kopeteview.h"
#include "kopetemessage.h"

#include "latexplugin.h"
#include "latexguiclient.h"

LatexGUIClient::LatexGUIClient( Kopete::ChatSession *parent )
: QObject( parent), KXMLGUIClient( parent )
{
	setInstance( LatexPlugin::plugin()->instance() );
	connect( LatexPlugin::plugin(), SIGNAL( destroyed( QObject * ) ), this, SLOT( deleteLater() ) );

	m_manager = parent;

	KAction *previewAction = new KAction( KIcon("latex"), i18n( "Preview Latex Images" ), actionCollection(), "latexPreview" );
	previewAction->setShortcut( KShortcut(Qt::CTRL + Qt::Key_L) );
	connect(previewAction, SIGNAL( triggered(bool) ), this, SLOT( slotPreview() ) );
	
	setXMLFile( "latexchatui.rc" );
}

LatexGUIClient::~LatexGUIClient()
{
}

void LatexGUIClient::slotPreview()
{
	if ( !m_manager->view() )
		return;

	Kopete::Message msg = m_manager->view()->currentMessage();
	QString messageText = msg.plainBody();
	if(!messageText.contains("$$")) //we haven't found any latex strings
	{
		KMessageBox::sorry(reinterpret_cast<QWidget*>(m_manager->view()) , i18n("There are no latex in the message you are typing.  The latex formula must be included between $$ and $$ "),	i18n("No Latex Formula") );
		return;
	}

	msg=Kopete::Message( msg.from() , msg.to() ,
						 i18n("<b>Preview of the latex message :</b> <br />%1", msg.plainBody()),
						 Kopete::Message::Internal , Kopete::Message::RichText);
	m_manager->appendMessage(msg) ;
}


#include "latexguiclient.moc"

// vim: set noet ts=4 sts=4 sw=4:

