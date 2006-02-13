/*
    kopetegroup.cpp - Kopete (Meta)Contact Group

    Copyright (c) 2002-2005 by Olivier Goffart       <ogoffart @ kde.org>
    Copyright (c) 2003      by Martijn Klingens      <klingens@kde.org>

    Kopete    (c) 2002-2005 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include "kopetegroup.h"

#include "kopetecontactlist.h"
#include "kopetemetacontact.h"
#include "kopetecontact.h"
#include "kopetechatsession.h"

#include <klocale.h>

namespace Kopete {

class Group::Private
{
public:
	QString displayName;
	Group::GroupType type;
	bool expanded;
	uint groupId;

	//Unique contact id per metacontact
	static uint uniqueGroupId;
};

Group *Group::s_topLevel  = 0L;
Group *Group::s_temporary = 0L;
Group * Group::topLevel()
{
	if ( !s_topLevel )
		s_topLevel = new Group( i18n( "Top Level" ), Group::TopLevel );

	return s_topLevel;
}

Group * Group::temporary()
{
	if ( !s_temporary )
		s_temporary = new Group( i18n( "Not in your contact list" ), Group::Temporary );

	return s_temporary;
}

uint Group::Private::uniqueGroupId = 0;

Group::Group( const QString &_name, GroupType _type )
	: ContactListElement( ContactList::self() )
{
	d = new Private;
	d->displayName = _name;
	d->type = _type;
	d->expanded = true;
	d->groupId = 0;
}

Group::Group()
	: ContactListElement( ContactList::self() )
{
	d = new Private;
	d->expanded = true;
	d->type = Normal;
	d->groupId = 0;
}

Group::~Group()
{
	if(d->type == TopLevel)
		s_topLevel=0L;
	if(d->type == Temporary)
		s_temporary=0L;
	delete d;
}

QList<MetaContact *> Group::members() const
{
	QList<MetaContact *> groupMembers;
	foreach(MetaContact *mc, ContactList::self()->metaContacts())
	{
		if( mc->groups().contains( const_cast<Group*>(this) ) )
			groupMembers.append(mc);
	}

	return groupMembers;
}

const QDomElement Group::toXML()
{
	QDomDocument group;
	group.appendChild( group.createElement( QString::fromLatin1( "kopete-group" ) ) );
	group.documentElement().setAttribute( QString::fromLatin1( "groupId" ), QString::number( groupId() ) );

	QString type;
	switch ( d->type )
	{
	case Temporary:
		type = QString::fromLatin1( "temporary" );
		break;
	case TopLevel:
		type = QString::fromLatin1( "top-level" );
		break;
	default:
		type = QString::fromLatin1( "standard" ); // == Normal
		break;
	}

	group.documentElement().setAttribute( QString::fromLatin1( "type" ), type );
	group.documentElement().setAttribute( QString::fromLatin1( "view" ), QString::fromLatin1( d->expanded ? "expanded" : "collapsed" )  );

	QDomElement displayName = group.createElement( QString::fromLatin1( "display-name" ) );
	displayName.appendChild( group.createTextNode( d->displayName ) );
	group.documentElement().appendChild( displayName );

	// Store other plugin data
	foreach ( QDomElement  it , ContactListElement::toXML() )
		group.documentElement().appendChild( group.importNode( it, true ) );


	return group.documentElement();
}

bool Group::fromXML( const QDomElement &data )
{
	QString strGroupId = data.attribute( QString::fromLatin1( "groupId" ) );
	if ( !strGroupId.isEmpty() )
	{
		d->groupId = strGroupId.toUInt();
		if ( d->groupId > d->uniqueGroupId )
			d->uniqueGroupId = d->groupId;
	}

	// Don't overwrite type for Temporary and TopLevel groups
	if ( d->type != Temporary && d->type != TopLevel )
	{
		QString type = data.attribute( QString::fromLatin1( "type" ), QString::fromLatin1( "standard" ) );
		if ( type == QString::fromLatin1( "temporary" ) )
		{
			if ( d->type != Temporary )
			{
				s_temporary->fromXML( data );
				return false;
			}
		}
		else if ( type == QString::fromLatin1( "top-level" ) )
		{
			if ( d->type != TopLevel )
			{
				s_topLevel->fromXML( data );
				return false;
			}
		}
		else
		{
			d->type = Normal;
		}
	}

	QString view = data.attribute( QString::fromLatin1( "view" ), QString::fromLatin1( "expanded" ) );
	d->expanded = ( view != QString::fromLatin1( "collapsed" ) );

	QDomNode groupData = data.firstChild();
	while ( !groupData.isNull() )
	{
		QDomElement groupElement = groupData.toElement();
		if ( groupElement.tagName() == QString::fromLatin1( "display-name" ) )
		{
			// Don't set display name for temporary or top-level items
			if ( d->type == Normal )
				d->displayName = groupElement.text();
		}
		else
		{
			Kopete::ContactListElement::fromXML( groupElement );
		}

		groupData = groupData.nextSibling();
	}

	// Sanity checks. We must not have groups without a displayname.
	if ( d->displayName.isEmpty() )
	{
		switch ( d->type )
		{
		case Temporary:
			d->displayName = QString::fromLatin1( "Temporary" );
			break;
		case TopLevel:
			d->displayName = QString::fromLatin1( "Top-Level" );
			break;
		default:
			d->displayName = i18n( "(Unnamed Group)" );
			break;
		}
	}

	//this allows to save data for the top-level group in the top-level group
	return ( d->type == Normal );
}

void Group::setDisplayName( const QString &s )
{
	if ( d->displayName != s )
	{
		QString oldname = d->displayName;
		d->displayName = s;
		emit displayNameChanged( this, oldname );
	}
}

QString Group::displayName() const
{
	return d->displayName;
}

Group::GroupType Group::type() const
{
	return d->type;
}

void Group::setType( GroupType t )
{
	d->type = t;
}

void Group::setExpanded( bool isExpanded )
{
	d->expanded = isExpanded;
}

bool Group::isExpanded() const
{
	return d->expanded;
}

uint Group::groupId() const
{
	if ( d->groupId == 0 )
		d->groupId = ++d->uniqueGroupId;

	return d->groupId;
}


void Group::sendMessage()
{
	Kopete::Contact *c;
	
	if(onlineMembers().isEmpty())
		return;
	c = onlineMembers().first()->preferredContact();
	c->sendMessage();
	if( c->manager( Contact::CanCreate ) )
	{
		connect( c->manager(), SIGNAL( messageSent( Kopete::Message&, Kopete::ChatSession* ) ), this, SLOT( sendMessage( Kopete::Message& ) ));
	}
}

void Group::sendMessage( Message& msg )
{
	QList<MetaContact *> list = onlineMembers();
	ChatSession *cs=msg.manager();
	if(  cs )
	{
		disconnect( cs, SIGNAL( messageSent( Kopete::Message&, Kopete::ChatSession* ) ), this, SLOT( sendMessage( Kopete::Message& ) ) );
	}
	else
		return;
	
	if(list.isEmpty())
		return;
	list.remove( msg.to().first()->metaContact() );
	QListIterator<MetaContact *> it(list);
	while ( it.hasNext() )
	{
		MetaContact *mc = it.next();
		if(mc->isReachable())
		{
			Contact *kcontact=mc->preferredContact();
			if( kcontact->manager( Contact::CanCreate )  )
			{
				//This is hack and stupid.    send message to group should never exist anyway - Olivier 2005-09-11
				// changing the "to" is require, because jabber use it to send the messgae.  Cf BUG 111514
				Message msg2(cs->myself() , kcontact , msg.plainBody() , msg.direction() , Message::PlainText , msg.requestedPlugin() );
				kcontact->manager( Contact::CanCreate )->sendMessage( msg2 );
			}
		}
	}
}

QList<MetaContact *> Group::onlineMembers() const
{
	QList<MetaContact *> list = members();
	QList<MetaContact *>::iterator it=list.begin();
	while ( it!=list.end() )
	{
		if( (*it)->isReachable() && (*it)->isOnline() ) 
			++it; 
		else   
			it=list.erase(it); 
	}
	return list;
}

} //END namespace Kopete 


#include "kopetegroup.moc"



