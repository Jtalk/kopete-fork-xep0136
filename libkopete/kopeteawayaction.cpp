/*
    kopeteaway.cpp  -  Kopete Away Action

    Copyright (c) 2003     Jason Keirstead   <jason@keirstead.org>

    Kopete    (c) 2002-2003 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include <klocale.h>
#include <qstringlist.h>
#include <kinputdialog.h>

#include "kopeteawayaction.h"
#include "kopeteaway.h"

KopeteAwayAction::KopeteAwayAction(const QString &text, const QIconSet &pix, const KShortcut &cut, 
	const QObject *receiver, const char *slot, QObject *parent, const char *name ) :
	KSelectAction(text, pix, cut, receiver, slot, parent, name )
{
	QObject::connect( KopeteAway::getInstance(), SIGNAL( messagesChanged() ), 
		this, SLOT( slotAwayChanged() ) );
		
	QObject::connect( this, SIGNAL( awayMessageSelected( const QString & ) ), 
		receiver, slot );
		
	QObject::connect( this, SIGNAL( activated( int ) ), 
		this, SLOT( slotSelectAway( int ) ) );
	
	reasonCount = 0;

	slotAwayChanged();
}

void KopeteAwayAction::slotAwayChanged()
{
	QStringList awayTitles = KopeteAway::getInstance()->getTitles();
	reasonCount = awayTitles.count();
	awayTitles.append( i18n("Custom Message...") );
	setItems( awayTitles );
	setCurrentItem( -1 );
}

void KopeteAwayAction::slotSelectAway( int index )
{
	KopeteAway *mAway = KopeteAway::getInstance();
	QString awayReason;
	
	if( index < reasonCount )
	{
		QStringList awayTitles = mAway->getTitles();
		awayReason = mAway->getMessage( awayTitles[index] );
	}
	else
		awayReason = KInputDialog::getText( i18n( "Custom Away Message" ), i18n( "Please enter your away reason:" ), QString::null);
	
	if( !awayReason.isEmpty() )
	{
		emit( awayMessageSelected( awayReason ) );
		setCurrentItem( -1 );
	}
}

#include "kopeteawayaction.moc"

