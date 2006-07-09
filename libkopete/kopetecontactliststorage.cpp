/*
    Kopete Contact List Storage Base Class

    Copyright  2006      by Matt Rogers <mattr@kde.org>
    Kopete     2002-2006 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/
#include "kopetecontactliststorage.h"

// Qt includes

// KDE includes

// Kopete includes
namespace Kopete
{

class ContactListStorage::Private
{
public:
    Private()
    {}

    Group::List groupList;
    MetaContact::List metaContactList;
};

ContactListStorage::ContactListStorage()
 : d(new Private)
{
}


ContactListStorage::~ContactListStorage()
{
    delete d;
}

Group::List ContactListStorage::groups() const
{
    return d->groupList;
}

MetaContact::List ContactListStorage::contacts() const
{
    return d->metaContactList;
}

void ContactListStorage::addMetaContact(Kopete::MetaContact *contact)
{
    d->metaContactList.append( contact );
}

void ContactListStorage::addGroup(Kopete::Group *group)
{
    d->groupList.append( group );
}

}

//kate: indent-mode cstyle; indent-spaces on; indent-width 4; auto-insert-doxygen on; replace-tabs on
