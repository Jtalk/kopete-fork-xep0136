/*
    kopetemetacontactlvi.cpp - Kopete Meta Contact K3ListViewItem

    Copyright (c) 2004      by Richard Smith          <kde@metafoo.co.uk>
    Copyright (c) 2002-2004 by Martijn Klingens       <klingens@kde.org>
    Copyright (c) 2002-2005 by Olivier Goffart        <ogoffart@kde.org>
    Copyright (c) 2002      by Duncan Mac-Vicar P     <duncan@kde.org>

    Kopete    (c) 2002-2004 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "kopetemetacontactitem.h"

#include <K3Icon>
#include <KIconLoader>

#include "kopetemetacontact.h"
#include "kopetepicture.h"


KopeteMetaContactItem::KopeteMetaContactItem( Kopete::MetaContact *contact )
: QObject(0),QStandardItem()

{
	m_metaContact = contact;
	setText( m_metaContact->displayName() );
	QPixmap iconPixmap = QPixmap::fromImage( m_metaContact->picture().image() );
	setIcon( QIcon( iconPixmap ) );

	connect( m_metaContact,
	         SIGNAL( displayNameChanged( const QString&, const QString& ) ),
	         this, SLOT( changeDisplayName( const QString&, const QString ) ) );
	connect( m_metaContact, SIGNAL( photoChanged() ),
	         this, SLOT( changePhoto() ) );

}

KopeteMetaContactItem::~KopeteMetaContactItem()
{
	m_metaContact = 0;
}

Kopete::MetaContact* KopeteMetaContactItem::metaContact() const
{
	return m_metaContact;
}

void KopeteMetaContactItem::changeDisplayName( const QString&,
                                               const QString& newName )
{
	if ( !newName.isEmpty() )
		setText( newName );
}

void KopeteMetaContactItem::changePhoto()
{
	QPixmap photoPixmap;
	if ( m_metaContact->picture().isNull() )
		photoPixmap = SmallIcon( m_metaContact->statusIcon(), IconSize(K3Icon::Small) );
	else
		photoPixmap = QPixmap::fromImage( m_metaContact->picture().image() );

	setIcon( QIcon( photoPixmap ) );
}

#include "kopetemetacontactitem.moc"

// vim: set noet ts=4 sts=4 sw=4: