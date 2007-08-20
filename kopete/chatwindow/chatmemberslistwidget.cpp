/*
    chatmemberslistwidget.cpp - Chat Members List Widget

    Copyright (c) 2004      by Richard Smith         <kde@metafoo.co.uk>

    Kopete    (c) 2002-2004 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "chatmemberslistwidget.h"

#include "kopetechatsession.h"
#include "kopetecontact.h"
#include "kopeteonlinestatus.h"
#include "kopeteglobal.h"
#include "kopeteprotocol.h"
#include "kopeteaccount.h"
#include "kopetemetacontact.h"

#include <kabc/stdaddressbook.h>
#include <kabc/addressee.h>
#include <kabc/vcardconverter.h>
#include <kdebug.h>
#include <k3multipledrag.h>

#include <kmenu.h>

#include <q3header.h>


//BEGIN ChatMembersListWidget::ToolTip
/*
class ChatMembersListWidget::ToolTip : public QToolTip
{
public:
	ToolTip( K3ListView *parent )
		: QToolTip( parent->viewport() ), m_listView ( parent )
	{
	}

	virtual ~ToolTip()
	{
		remove( m_listView->viewport() );
	}

	void maybeTip( const QPoint &pos )
	{
		if( Q3ListViewItem *item = m_listView->itemAt( pos ) )
		{
			QRect itemRect = m_listView->itemRect( item );
			if( itemRect.contains( pos ) )
				tip( itemRect, static_cast<ContactItem*>( item )->contact()->toolTip() );
		}
	}

private:
	K3ListView *m_listView;
};
*/
//END ChatMembersListWidget::ToolTip


//BEGIN ChatMembersListWidget::ContactItem

ChatMembersListWidget::ContactItem::ContactItem( ChatMembersListWidget *parent, Kopete::Contact *contact )
	: K3ListViewItem( parent ), m_contact( contact )
{
	QString nick = m_contact->property(Kopete::Global::Properties::self()->nickName().key()).value().toString();
	if ( nick.isEmpty() )
		nick = m_contact->contactId();
	setText( 0, nick );
	setDragEnabled(true);

	connect( m_contact, SIGNAL( propertyChanged( Kopete::PropertyContainer *, const QString &, const QVariant &, const QVariant & ) ),
	         this, SLOT( slotPropertyChanged( Kopete::Contact *, const QString &, const QVariant &, const QVariant & ) ) ) ;

	setStatus( parent->session()->contactOnlineStatus(m_contact) );
	reposition();
}

void ChatMembersListWidget::ContactItem::slotPropertyChanged( Kopete::Contact*,
	const QString &key, const QVariant&, const QVariant &newValue  )
{
	if ( key == Kopete::Global::Properties::self()->nickName().key() )
	{
		setText( 0, newValue.toString() );
		reposition();
	}
}

void ChatMembersListWidget::ContactItem::setStatus( const Kopete::OnlineStatus &status )
{
	setPixmap( 0, status.iconFor( m_contact ) );
	reposition();
}

void ChatMembersListWidget::ContactItem::reposition()
{
	// Qt's listview sorting is pathetic - it's impossible to reposition a single item
	// when its key changes, without re-sorting the whole list. Plus, the whole list gets
	// re-sorted whenever an item is added/removed. So, we do manual sorting.
	// In particular, this makes adding N items O(N^2) not O(N^2 log N).
	Kopete::ChatSession *session = static_cast<ChatMembersListWidget*>( listView() )->session();
	int ourWeight = session->contactOnlineStatus(m_contact).weight();
	Q3ListViewItem *after = 0;

	for ( Q3ListViewItem *it = K3ListViewItem::listView()->firstChild(); it; it = it->nextSibling() )
	{
		ChatMembersListWidget::ContactItem *item = static_cast<ChatMembersListWidget::ContactItem*>(it);
		int theirWeight = session->contactOnlineStatus(item->m_contact).weight();

		if( theirWeight < ourWeight ||
			(theirWeight == ourWeight && item->text(0).localeAwareCompare( text(0) ) > 0 ) )
		{
			break;
		}

		after = it;
	}

	moveItem( after );
}

//END ChatMembersListWidget::ContactItem


//BEGIN ChatMembersListWidget

ChatMembersListWidget::ChatMembersListWidget( QWidget *parent, Kopete::ChatSession *session )
	 : K3ListView( parent ), m_session( session )
{
	// use our own custom tooltips
//	setShowToolTips( false );
//	m_toolTip = new ToolTip( this );

	// set up display: no header
	setAllColumnsShowFocus( true );
	addColumn( QString::null, -1 );	//krazy:exclude=nullstrassign for old broken gcc
	header()->setStretchEnabled( true, 0 );
	header()->hide();

	// list is sorted by us, not by Qt
	setSorting( -1 );

	if ( session )
		setChatSession( session );
}

ChatMembersListWidget::~ChatMembersListWidget()
{
}

void ChatMembersListWidget::slotContextMenu( K3ListView*, Q3ListViewItem *item, const QPoint &point )
{
	if ( ContactItem *contactItem = dynamic_cast<ContactItem*>(item) )
	{
		KMenu *p = contactItem->contact()->popupMenu( session() );
		connect( p, SIGNAL( aboutToHide() ), p, SLOT( deleteLater() ) );
		p->popup( point );
	}
}

void ChatMembersListWidget::slotContactAdded( const Kopete::Contact *contact )
{
	if ( !m_members.contains( contact ) )
		m_members.insert( contact, new ContactItem( this, const_cast<Kopete::Contact*>( contact ) ) );
}

void ChatMembersListWidget::slotContactRemoved( const Kopete::Contact *contact )
{
	kDebug(14000) << k_funcinfo;
	if ( m_members.contains( contact ) && contact != session()->myself() )
	{
		delete m_members[ contact ];
		m_members.remove( contact );
	}
}

void ChatMembersListWidget::slotContactStatusChanged( Kopete::Contact *contact, const Kopete::OnlineStatus &status )
{
	if ( m_members.contains( contact ) )
		m_members[contact]->setStatus( status );
}

void ChatMembersListWidget::slotExecute( Q3ListViewItem *item )
{
	if ( ContactItem *contactItem = dynamic_cast<ContactItem*>(item ) )
	{
		Kopete::Contact *contact=contactItem->contact();

		if(!contact || contact == contact->account()->myself())
			return;
				
		contact->execute();
	}
}

Q3DragObject *ChatMembersListWidget::dragObject()
{
	Q3ListViewItem *currentLVI = currentItem();
	if( !currentLVI )
		return 0L;

	ContactItem *lvi = dynamic_cast<ContactItem*>( currentLVI );
	if( !lvi )
		return 0L;

	Kopete::Contact *c = lvi->contact();
	K3MultipleDrag *drag = new K3MultipleDrag( this );
	drag->addDragObject( new Q3StoredDrag("application/x-qlistviewitem", 0L ) );

	Q3StoredDrag *d = new Q3StoredDrag("kopete/x-contact", 0L );
	d->setEncodedData( QString( c->protocol()->pluginId()+QChar( 0xE000 )+c->account()->accountId()+QChar( 0xE000 )+ c->contactId() ).toUtf8() );
	drag->addDragObject( d );

	KABC::Addressee address = KABC::StdAddressBook::self()->findByUid(c->metaContact()->metaContactId());

	if( !address.isEmpty() )
	{
		drag->addDragObject( new Q3TextDrag( address.fullEmail(), 0L ) );
		KABC::VCardConverter converter;
		QString vcard = converter.createVCard( address );
		if( !vcard.isNull() )
		{
			Q3StoredDrag *vcardDrag = new Q3StoredDrag("text/directory", 0L );
			vcardDrag->setEncodedData( vcard.toUtf8() );
			drag->addDragObject( vcardDrag );
		}
	}

	drag->setPixmap( c->onlineStatus().iconFor(c, 12) );

	return drag;
	return 0l;
}

void ChatMembersListWidget::setChatSession(Kopete::ChatSession *session)
{
	if (m_session)
	{
		// Work on the old session
		// clear the list (foreach is delete-safe as it works on a copy of the container).
		foreach ( Kopete::Contact *it, m_session->members() )
		{
			slotContactRemoved( it );
		}
	}

	// Now work on the new session
	m_session = session;
	
	// add chat members
	slotContactAdded( session->myself() );
	foreach ( Kopete::Contact* it ,  session->members() ) 
		slotContactAdded( it );


	connect( this, SIGNAL( contextMenu( K3ListView*, Q3ListViewItem *, const QPoint &) ),
	         SLOT( slotContextMenu(K3ListView*, Q3ListViewItem *, const QPoint & ) ) );
	connect( this, SIGNAL( executed( Q3ListViewItem* ) ),
	         SLOT( slotExecute( Q3ListViewItem * ) ) );

	connect( session, SIGNAL( contactAdded(const Kopete::Contact*, bool) ),
	         this, SLOT( slotContactAdded(const Kopete::Contact*) ) );
	connect( session, SIGNAL( contactRemoved(const Kopete::Contact*, const QString&, Qt::TextFormat, bool) ),
	         this, SLOT( slotContactRemoved(const Kopete::Contact*) ) );
	connect( session, SIGNAL( onlineStatusChanged( Kopete::Contact *, const Kopete::OnlineStatus & , const Kopete::OnlineStatus &) ),
	         this, SLOT( slotContactStatusChanged( Kopete::Contact *, const Kopete::OnlineStatus & ) ) );
}
//END ChatMembersListWidget

#include "chatmemberslistwidget.moc"

// vim: set noet ts=4 sts=4 sw=4:

