/*
    kopetecontactaction.cpp - KAction for selecting a Kopete::Contact

    Copyright (c) 2003 by Martijn Klingens       <klingens@kde.org>

    Kopete    (c) 2003 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include "kopetecontactaction.h"

#include "kopetemetacontact.h"
#include "kopetecontact.h"
#include "kopeteonlinestatus.h"

namespace Kopete
{
namespace UI
{

ContactAction::ContactAction( Kopete::Contact *contact, KActionCollection* parent )
: KAction( KIcon( contact->onlineStatus().iconFor( contact ) ),
           contact->metaContact()->displayName(), parent,
           contact->contactId() )
{
	setData( contact );
	connect( this, SIGNAL( triggered( bool ) ),
	         this, SLOT( slotTriggered( bool ) ) );
}

void ContactAction::slotTriggered( bool checked )
{
	Kopete::Contact* contact = reinterpret_cast<Kopete::Contact*>(data().value<void*>());
	emit triggered( contact, checked );
}

}
}
#include "kopetecontactaction.moc"

// vim: set noet ts=4 sts=4 sw=4:
