/*
    kopetecontactlistview.cpp

    Kopete Contactlist GUI

    Copyright (c) 2001-2002 by Duncan Mac-Vicar Prett <duncan@kde.org>
    Copyright (c) 2002      by Nick Betcher           <nbetcher@usinternet.com>
    Copyright (c) 2002      by Stefan Gehn            <metz AT gehn.net>
    Copyright (c) 2002-2004 by Olivier Goffart        <ogoffart@tiscalinet.be>
    Copyright (c) 2002-2003 by Martijn Klingens       <klingens@kde.org>
    Copyright (c) 2004      by Richard Smith          <kde@metafoo.co.uk>

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

#include "kopetecontactlistview.h"
#include "kopeteuiglobal.h"

#include <qcursor.h>
#include <qdragobject.h>
#include <qheader.h>
#include <qstylesheet.h>
#include <qtimer.h>
#include <qtooltip.h>
#include <qguardedptr.h>

#include <kaction.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmainwindow.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kurldrag.h>
#include <kmultipledrag.h>
#include <kabc/stdaddressbook.h>
#include <kabc/vcardconverter.h>

#include <kdeversion.h>
#include <kinputdialog.h>

#include "addcontactwizard.h"
#include "addcontactpage.h"
#include "kopeteaccount.h"
#include "kopeteaccountmanager.h"
#include "kopetecontactlist.h"
#include "kopeteevent.h"
#include "kopetegroup.h"
#include "kopetegroupviewitem.h"
#include "kopetemetacontact.h"
#include "kopetemetacontactlvi.h"
#include "kopeteprefs.h"
#include "kopeteprotocol.h"
#include "kopetestatusgroupviewitem.h"
#include "kopetestdaction.h"
#include "kopetemessagemanagerfactory.h"
#include "kopetecontact.h"
#include "kopetemessagemanager.h" //needed to send the URL
#include "kopetemessage.h"       //needed to send the URL
#include "kopeteglobal.h"
#include "kopetelviprops.h"

#include <memory>

class ContactListViewStrategy;

class KopeteContactListViewPrivate
{
public:
	std::auto_ptr<ContactListViewStrategy> viewStrategy;

	void updateViewStrategy( KListView *view );
};

class ContactListViewStrategy
{
public:
	ContactListViewStrategy( KListView *view )
	 : _listView( view )
	{
		view->clear();
	}
	virtual ~ContactListViewStrategy() {}
	KListView *listView() { return _listView; }
	void addCurrentItems()
	{
		// Add the already existing groups now
		QPtrList<KopeteGroup> grps = KopeteContactList::contactList()->groups();
		for ( QPtrListIterator<KopeteGroup> groupIt( grps ); groupIt.current(); ++groupIt )
			addGroup( groupIt.current() );

		// Add the already existing meta contacts now
		QPtrList<KopeteMetaContact> metaContacts = KopeteContactList::contactList()->metaContacts();
		for ( QPtrListIterator<KopeteMetaContact> it( metaContacts ); it.current(); ++it )
			addMetaContact( it.current() );
	}

	virtual void addMetaContact( KopeteMetaContact *mc ) = 0;
	virtual void removeMetaContact( KopeteMetaContact *mc ) = 0;

	virtual void addGroup( KopeteGroup * ) {}

	virtual void addMetaContactToGroup( KopeteMetaContact *mc, KopeteGroup *gp ) = 0;
	virtual void removeMetaContactFromGroup( KopeteMetaContact *mc, KopeteGroup *gp ) = 0;
	virtual void moveMetaContactBetweenGroups( KopeteMetaContact *mc, KopeteGroup *from, KopeteGroup *to ) = 0;

	virtual void metaContactStatusChanged( KopeteMetaContact *mc ) = 0;

protected:
	// work around QListView design stupidity.
	// GroupViewItem will be QListView-derived, or QListViewItem-derived.
	template<typename GroupViewItem>
	void addMetaContactToGroupInner( KopeteMetaContact *mc, GroupViewItem *gpi )
	{
		// check if the contact isn't already in the group
		for( QListViewItem *item = gpi->firstChild(); item; item = item->nextSibling() )
			if ( KopeteMetaContactLVI *mci = dynamic_cast<KopeteMetaContactLVI*>(item) )
				if ( mci->metaContact() == mc )
					return;
		(void) new KopeteMetaContactLVI( mc, gpi );
	}

	template<typename GroupViewItem>
	void removeMetaContactFromGroupInner( KopeteMetaContact *mc, GroupViewItem *gpi )
	{
		for( QListViewItem *item = gpi->firstChild(); item; item = item->nextSibling() )
			if ( KopeteMetaContactLVI *mci = dynamic_cast<KopeteMetaContactLVI*>(item) )
				if ( mci->metaContact() == mc )
					delete mci;
	}

private:
	KListView *_listView;
};

class ArrangeByGroupsViewStrategy : public ContactListViewStrategy
{
public:
	ArrangeByGroupsViewStrategy( KListView *view )
	 : ContactListViewStrategy( view )
	{
		addCurrentItems();
	}

	void addMetaContact( KopeteMetaContact *mc )
	{
		// create group items
		KopeteGroupList list = mc->groups();
		for ( KopeteGroup *gp = list.first(); gp; gp = list.next() )
			// will check to see if the contact is already in the group.
			// this is inefficient but makes this function idempotent.
			addMetaContactToGroup( mc, gp );
	}
	void removeMetaContact( KopeteMetaContact *mc )
	{
		// usually, the list item will be deleted when the KMC is. however, we still
		// need to make sure that the item count of the groups is correct.
		// as a bonus, this allows us to remove a MC from the contact list without deleting it.
		KopeteGroupList list = mc->groups();
		for ( KopeteGroup *gp = list.first(); gp; gp = list.next() )
			removeMetaContactFromGroup( mc, gp );
	}

	void addGroup( KopeteGroup *group )
	{
		(void) findOrCreateGroupItem( group );
	}

	void addMetaContactToGroup( KopeteMetaContact *mc, KopeteGroup *gp )
	{
		if ( KopeteGroupViewItem *gpi = findOrCreateGroupItem( gp ) )
			addMetaContactToGroupInner( mc, gpi );
		else
			addMetaContactToGroupInner( mc, listView() );
	}
	void removeMetaContactFromGroup( KopeteMetaContact *mc, KopeteGroup *gp )
	{
		if ( gp->type() == KopeteGroup::TopLevel )
			removeMetaContactFromGroupInner( mc, listView() );
		else if ( KopeteGroupViewItem *gpi = findGroupItem( gp ) )
		{
			removeMetaContactFromGroupInner( mc, gpi );

			// update the group's display of its number of children.
			// TODO: make the KopeteGroupViewItem not need this, by overriding insertItem and takeItem
			gpi->refreshDisplayName();

			// remove the temporary group if it's empty
			if ( gpi->childCount() == 0 )
				if ( gp->type() == KopeteGroup::Temporary )
					delete gpi;
		}
	}
	void moveMetaContactBetweenGroups( KopeteMetaContact *mc, KopeteGroup *from, KopeteGroup *to )
	{
		// TODO: use takeItem and insertItem, and mci->movedGroup
		addMetaContactToGroup( mc, to );
		removeMetaContactFromGroup( mc, from );
	}
	void metaContactStatusChanged( KopeteMetaContact * ) {}

private:
	KopeteGroupViewItem *findGroupItem( KopeteGroup *gp )
	{
		if ( gp->type() == KopeteGroup::TopLevel ) return 0;
		for( QListViewItem *item = listView()->firstChild(); item; item = item->nextSibling() )
			if ( KopeteGroupViewItem *gvi = dynamic_cast<KopeteGroupViewItem*>(item) )
				if ( gvi->group() == gp )
					return gvi;
		return 0;
	}
	KopeteGroupViewItem *findOrCreateGroupItem( KopeteGroup *gp )
	{
		if ( gp->type() == KopeteGroup::TopLevel ) return 0;
		if ( KopeteGroupViewItem *item = findGroupItem(gp) )
			return item;
		KopeteGroupViewItem *gpi = new KopeteGroupViewItem( gp, listView() );
		// TODO: store as plugin data the expandedness of a group
		// currently this requires a 'plugin' for the main UI.
		gpi->setOpen( gp->isExpanded() );
		return gpi;
	}
};

class ArrangeByPresenceViewStrategy : public ContactListViewStrategy
{
public:
	ArrangeByPresenceViewStrategy( KListView *view )
	 : ContactListViewStrategy( view )
	 , m_onlineItem( new KopeteStatusGroupViewItem( KopeteOnlineStatus::Online, listView() ) )
	 , m_offlineItem( new KopeteStatusGroupViewItem( KopeteOnlineStatus::Offline, listView() ) )
	 , m_temporaryItem( 0 )
	{
		m_onlineItem->setOpen( true );
		m_offlineItem->setOpen( true );
		addCurrentItems();
	}

	void removeMetaContact( KopeteMetaContact *mc )
	{
		// there's only three places we put metacontacts: online, offline and temporary.
		removeMetaContactFromGroupInner( mc, m_onlineItem );
		removeMetaContactFromGroupInner( mc, m_offlineItem );
		if ( m_temporaryItem )
			removeMetaContactFromGroupInner( mc, m_temporaryItem );
	}

	void addMetaContact( KopeteMetaContact *mc )
	{
		updateMetaContact( mc );
	}
	void addMetaContactToGroup( KopeteMetaContact *mc, KopeteGroup * )
	{
		updateMetaContact( mc );
	}
	void removeMetaContactFromGroup( KopeteMetaContact *mc, KopeteGroup * )
	{
		updateMetaContact( mc );
	}
	void moveMetaContactBetweenGroups( KopeteMetaContact *mc, KopeteGroup *, KopeteGroup * )
	{
		updateMetaContact( mc );
	}
	void metaContactStatusChanged( KopeteMetaContact *mc )
	{
		updateMetaContact( mc );
	}
private:
	void updateMetaContact( KopeteMetaContact *mc )
	{
		// split into a ...Inner function and this one to make the short-circuiting logic easier
		updateMetaContactInner( mc );

		// FIXME: these items should do this for themselves...
		m_onlineItem->setText(0,i18n("Online contacts (%1)").arg(m_onlineItem->childCount()));
		m_offlineItem->setText(0,i18n("Offline contacts (%1)").arg(m_offlineItem->childCount()));
	}
	void updateMetaContactInner( KopeteMetaContact *mc )
	{
		// this function basically *is* the arrange-by-presence strategy.
		// given a metacontact, it removes it from any existing incorrect place and adds
		// it to the correct place. usually it does this with takeItem and insertItem.

		// if the metacontact is temporary, it should be only in the temporary group
		if ( mc->isTemporary() )
		{
			removeMetaContactFromGroupInner( mc, m_onlineItem );
			removeMetaContactFromGroupInner( mc, m_offlineItem );

			// create temporary item on demand
			if ( !m_temporaryItem )
			{
				m_temporaryItem = new KopeteGroupViewItem( KopeteGroup::temporary(), listView() );
				m_temporaryItem->setOpen( true );
			}

			addMetaContactToGroupInner( mc, m_temporaryItem );
			return;
		}

		// if it's not temporary, it should not be in the temporary group
		if ( m_temporaryItem )
		{
			removeMetaContactFromGroupInner( mc, m_temporaryItem );

			// remove temporary item if empty
			if ( m_temporaryItem && m_temporaryItem->childCount() == 0 )
			{
				delete m_temporaryItem;
				m_temporaryItem = 0;
			}
		}

		// check if the contact is already in the correct "group"
		QListViewItem *currentGroup = mc->isOnline() ? m_onlineItem : m_offlineItem;
		for( QListViewItem *lvi = currentGroup->firstChild(); lvi; lvi = lvi->nextSibling() )
			if ( KopeteMetaContactLVI *kc = dynamic_cast<KopeteMetaContactLVI*>( lvi ) )
				if ( kc->metaContact() == mc )
					return;

		// item not found in the right group; look for it in the other group
		QListViewItem *oppositeGroup = mc->isOnline() ? m_offlineItem : m_onlineItem;
		for( QListViewItem *lvi = oppositeGroup->firstChild(); lvi; lvi = lvi->nextSibling() )
		{
			if ( KopeteMetaContactLVI *kc = dynamic_cast<KopeteMetaContactLVI*>( lvi ) )
			{
				if ( kc->metaContact() == mc )
				{
					// found: move it over to the right group
					oppositeGroup->takeItem(kc);
					currentGroup->insertItem(kc);
					return;
				}
			}
		}

		// item not found in either online neither offline groups: add it
		(void) new KopeteMetaContactLVI( mc, currentGroup );
	}

	KopeteStatusGroupViewItem *m_onlineItem, *m_offlineItem;
	KopeteGroupViewItem *m_temporaryItem;
};

void KopeteContactListViewPrivate::updateViewStrategy( KListView *view )
{
	// this is a bit nasty, but this function needs changing if we add
	// more view strategies anyway, so it should be fine.
	bool bSortByGroup = (bool)dynamic_cast<ArrangeByGroupsViewStrategy*>(viewStrategy.get());
	if ( !viewStrategy.get() || KopetePrefs::prefs()->sortByGroup() != bSortByGroup )
	{
		// delete old strategy first...
		viewStrategy.reset( 0 );
		// then create and store a new one
		if ( KopetePrefs::prefs()->sortByGroup() )
			viewStrategy.reset( new ArrangeByGroupsViewStrategy(view) );
		else
			viewStrategy.reset( new ArrangeByPresenceViewStrategy(view) );
	}
}

// returns the next item in a depth-first descent of the list view.
// much like QLVI::itemBelow but does not depend on visibility of items, etc.
static QListViewItem *nextItem( QListViewItem *item )
{
	if ( QListViewItem *it = item->firstChild() )
		return it;
	while ( item && !item->nextSibling() )
		item = item->parent();
	if ( !item )
		return 0;
	return item->nextSibling();
}



KopeteContactListView::KopeteContactListView( QWidget *parent, const char *name )
 : Kopete::UI::ListView::ListView( parent, name )
{
	d = new KopeteContactListViewPrivate;
	m_undo=0L;
	m_redo=0L;

	mShowAsTree = KopetePrefs::prefs()->treeView();
	if ( mShowAsTree )
	{
		setRootIsDecorated( true );
	}
	else
	{
		setRootIsDecorated( false );
		setTreeStepSize( 0 );
	}

	d->updateViewStrategy( this );

	setFullWidth( true );

	connect( this, SIGNAL( contextMenu( KListView *, QListViewItem *, const QPoint & ) ),
	         SLOT( slotContextMenu( KListView *, QListViewItem *, const QPoint & ) ) );
	connect( this, SIGNAL( expanded( QListViewItem * ) ),
	         SLOT( slotExpanded( QListViewItem * ) ) );
	connect( this, SIGNAL( collapsed( QListViewItem * ) ),
	         SLOT( slotCollapsed( QListViewItem * ) ) );
	connect( this, SIGNAL( executed( QListViewItem *, const QPoint &, int ) ),
	         SLOT( slotExecuted( QListViewItem *, const QPoint &, int ) ) );
	connect( this, SIGNAL( selectionChanged() ), SLOT( slotViewSelectionChanged() ) );
	connect( this, SIGNAL( itemRenamed( QListViewItem * ) ),
	         SLOT( slotItemRenamed( QListViewItem * ) ) );

	connect( KopetePrefs::prefs(), SIGNAL( saved() ), SLOT( slotSettingsChanged() ) );

	connect( KopeteContactList::contactList(), SIGNAL( selectionChanged() ),
	         SLOT( slotListSelectionChanged() ) );
	connect( KopeteContactList::contactList(),
	         SIGNAL( metaContactAdded( KopeteMetaContact * ) ),
	         SLOT( slotMetaContactAdded( KopeteMetaContact * ) ) );
	connect( KopeteContactList::contactList(),
	         SIGNAL( metaContactDeleted( KopeteMetaContact * ) ),
	         SLOT( slotMetaContactDeleted( KopeteMetaContact * ) ) );
	connect( KopeteContactList::contactList(), SIGNAL( groupAdded( KopeteGroup * ) ),
	         SLOT( slotGroupAdded( KopeteGroup * ) ) );

	connect( KopeteMessageManagerFactory::factory(), SIGNAL( newEvent( KopeteEvent * ) ),
	         this, SLOT( slotNewMessageEvent( KopeteEvent * ) ) );

	connect( this, SIGNAL( dropped( QDropEvent *, QListViewItem *, QListViewItem * ) ),
	         this, SLOT( slotDropped( QDropEvent *, QListViewItem *, QListViewItem * ) ) );
			 
	connect( &undoTimer, SIGNAL(timeout()) , this, SLOT (slotTimeout() ) );

	addColumn( i18n( "Contacts" ), 0 );  //add an unique colums to add every contact
	header()->hide(); // and hide the ugly header which show the single word  "Contacts"

	setAutoOpen( true );
	setDragEnabled( true );
	setAcceptDrops( true );
	setItemsMovable( false );
	setDropVisualizer( false );
	setDropHighlighter( true );
	setSelectionMode( QListView::Extended );

	// Load in the user's initial settings
	slotSettingsChanged();
}

void KopeteContactListView::initActions( KActionCollection *ac )
{
	actionUndo = KStdAction::undo( this , SLOT( slotUndo() ) , ac );
	actionRedo = KStdAction::redo( this , SLOT( slotRedo() ) , ac );
	actionUndo->setEnabled(false);
	actionRedo->setEnabled(false);


	new KAction( i18n( "Create New Group..." ), 0, 0, this, SLOT( addGroup() ),
		ac, "AddGroup" );

	actionSendMessage = KopeteStdAction::sendMessage(
		this, SLOT( slotSendMessage() ), ac, "contactSendMessage" );
	actionStartChat = KopeteStdAction::chat( this, SLOT( slotStartChat() ),
		ac, "contactStartChat" );

	actionRemoveFromGroup = KopeteStdAction::deleteContact(
		this, SLOT( slotRemoveFromGroup() ), ac, "contactRemoveFromGroup" );
	actionRemoveFromGroup->setText( i18n("Remove From Group") );

	actionMove = KopeteStdAction::moveContact( this, SLOT( slotMoveToGroup() ),
		ac, "contactMove" );
	actionCopy = KopeteStdAction::copyContact( this, SLOT( slotCopyToGroup() ),
		ac, "contactCopy" );
	actionRemove = KopeteStdAction::deleteContact( this, SLOT( slotRemove() ),
		ac, "contactRemove" );
	actionSendEmail = new KAction( i18n( "Send Email..." ), QString::fromLatin1( "mail_generic" ),
		0, this, SLOT(  slotSendEmail() ), ac, "contactSendEmail" );
	actionRename = new KAction( i18n( "Rename" ), "filesaveas", 0,
		this, SLOT( slotRename() ), ac, "contactRename" );
	actionSendFile = KopeteStdAction::sendFile( this, SLOT( slotSendFile() ),
		ac, "contactSendFile" );

	actionAddContact = new KActionMenu( i18n( "&Add Contact" ),
		QString::fromLatin1( "bookmark_add" ), ac , "contactAddContact" );
	actionAddContact->popupMenu()->insertTitle( i18n("Select Account") );

	actionAddTemporaryContact = new KAction( i18n( "Add to Your Contact List" ), "bookmark_add", 0,
		this, SLOT( slotAddTemporaryContact() ), ac, "contactAddTemporaryContact" );

	// TEMPORARY FOR TESTING - will be carried out on load once complete
	actionSyncKABC = new KAction( i18n( "Sync KABC..." ), QString::fromLatin1( "kaddressbook" ),
		0, this, SLOT(  slotSyncKABC() ), ac, "contactSyncKABC" );

	connect( KopeteContactList::contactList(), SIGNAL( metaContactSelected( bool ) ), this, SLOT( slotMetaContactSelected( bool ) ) );

	QPtrList<KopeteAccount> accounts = KopeteAccountManager::manager()->accounts();
	for( KopeteAccount *acc = accounts.first() ; acc ; acc = accounts.next() )
	{
		KAction *action = new KAction( acc->accountId(), acc->accountIcon(), 0, this, SLOT( slotAddContact() ), acc );
		actionAddContact->insert( action );
	}

	actionProperties = new KAction( i18n( "&Properties" ), "", Qt::Key_Alt + Qt::Key_Return,
		this, SLOT( slotProperties() ), ac, "contactProperties" );

	// Update enabled/disabled actions
	slotViewSelectionChanged();
}

KopeteContactListView::~KopeteContactListView()
{
	delete d;
}

void KopeteContactListView::slotMetaContactAdded( KopeteMetaContact *mc )
{
	d->viewStrategy->addMetaContact( mc );

	connect( mc, SIGNAL( addedToGroup( KopeteMetaContact *, KopeteGroup * ) ),
		SLOT( slotAddedToGroup( KopeteMetaContact *, KopeteGroup * ) ) );
	connect( mc, SIGNAL( removedFromGroup( KopeteMetaContact *, KopeteGroup * ) ),
		SLOT( slotRemovedFromGroup( KopeteMetaContact *, KopeteGroup * ) ) );
	connect( mc, SIGNAL( movedToGroup( KopeteMetaContact *, KopeteGroup *, KopeteGroup * ) ),
		SLOT( slotMovedToGroup( KopeteMetaContact *, KopeteGroup *, KopeteGroup * ) ) );
	connect( mc, SIGNAL( onlineStatusChanged( KopeteMetaContact *, KopeteOnlineStatus::OnlineStatus ) ),
		SLOT( slotContactStatusChanged( KopeteMetaContact * ) ) );
}

void KopeteContactListView::slotMetaContactDeleted( KopeteMetaContact *mc )
{
	d->viewStrategy->removeMetaContact( mc );
}

void KopeteContactListView::slotMetaContactSelected( bool sel )
{
	bool set = sel;

	if( sel )
	{
		KopeteMetaContact *kmc = KopeteContactList::contactList()->selectedMetaContacts().first();
		set = sel && kmc->isReachable();
		actionAddTemporaryContact->setEnabled( sel && kmc->isTemporary() );

		// TODO: make available for several contacts
		actionRemoveFromGroup->setEnabled( sel && (kmc->groups().count()>1 )  );
	}
	else
	{
		actionAddTemporaryContact->setEnabled(false);
		actionRemoveFromGroup->setEnabled(false);
	}

	actionSendMessage->setEnabled( set );
	actionStartChat->setEnabled( set );
	actionMove->setEnabled( sel ); // TODO: make available for several contacts
	actionCopy->setEnabled( sel ); // TODO: make available for several contacts
	actionAddContact->setEnabled( sel );
}

void KopeteContactListView::slotAddedToGroup( KopeteMetaContact *mc, KopeteGroup *to )
{
	d->viewStrategy->addMetaContactToGroup( mc, to );
}

void KopeteContactListView::slotRemovedFromGroup( KopeteMetaContact *mc, KopeteGroup *from )
{
	d->viewStrategy->removeMetaContactFromGroup( mc, from );
}

void KopeteContactListView::slotMovedToGroup( KopeteMetaContact *mc,
	KopeteGroup *from, KopeteGroup *to )
{
	d->viewStrategy->moveMetaContactBetweenGroups( mc, from, to );
}

void KopeteContactListView::removeContact( KopeteMetaContact *c )
{
	d->viewStrategy->removeMetaContact( c );
}

void KopeteContactListView::addGroup()
{
	QString groupName =
		KInputDialog::getText( i18n( "New Group" ),
			i18n( "Please enter the name for the new group:" ) );

	if ( !groupName.isEmpty() )
		addGroup( groupName );
}

void KopeteContactListView::addGroup( const QString &groupName )
{
	d->viewStrategy->addGroup( KopeteContactList::contactList()->getGroup(groupName) );
}

void KopeteContactListView::slotGroupAdded( KopeteGroup *group )
{
	d->viewStrategy->addGroup( group );
}

void KopeteContactListView::slotExpanded( QListViewItem *item )
{
	KopeteGroupViewItem *groupLVI = dynamic_cast<KopeteGroupViewItem *>( item );
	if ( groupLVI )
	{
		groupLVI->group()->setExpanded( true );
		groupLVI->updateIcon();
	}
}

void KopeteContactListView::slotCollapsed( QListViewItem *item )
{
	KopeteGroupViewItem *groupLVI = dynamic_cast<KopeteGroupViewItem*>( item );
	if ( groupLVI )
	{
		groupLVI->group()->setExpanded( false );
		groupLVI->updateIcon();
	}
}

void KopeteContactListView::slotContextMenu( KListView * /*listview*/,
	QListViewItem *item, const QPoint &point )
{
	// FIXME: this code should be moved to the various list view item classes.
	KopeteMetaContactLVI *metaLVI = dynamic_cast<KopeteMetaContactLVI *>( item );
	KopeteGroupViewItem  *groupvi = dynamic_cast<KopeteGroupViewItem *>( item );

	if ( item && !item->isSelected() )
	{
		clearSelection();
		item->setSelected( true );
	}

	if ( !item )
		clearSelection();

	int nb = KopeteContactList::contactList()->selectedMetaContacts().count() +
		KopeteContactList::contactList()->selectedGroups().count();

	KMainWindow *window = dynamic_cast<KMainWindow *>(topLevelWidget());
	if ( !window )
	{
		kdError( 14000 ) << k_funcinfo << "Main window not found, unable to display context-menu; "
			<< "Kopete::UI::Global::mainWidget() = " << Kopete::UI::Global::mainWidget() << endl;
		return;
	}

	if ( metaLVI && nb == 1 )
	{
		int px = mapFromGlobal( point ).x() - ( header()->sectionPos( header()->mapToIndex( 0 ) ) +
			treeStepSize() * ( item->depth() + ( rootIsDecorated() ? 1 : 0 ) ) + itemMargin() );
		int py = mapFromGlobal( point ).y() - itemRect( item ).y() - header()->height();

		//kdDebug( 14000 ) << k_funcinfo << "x: " << px << ", y: " << py << endl;
		KopeteContact *c = metaLVI->contactForPoint( QPoint( px, py ) ) ;
		if ( c )
		{
			KPopupMenu *p = c->popupMenu();
			connect( p, SIGNAL( aboutToHide() ), p, SLOT( deleteLater() ) );
			p->popup( point );
		}
		else
		{
			KPopupMenu *popup = dynamic_cast<KPopupMenu *>(
				window->factory()->container( "contact_popup", window ) );
			if ( popup )
			{
				QString title = i18n( "Translators: format: '<nickname> (<online status>)'", "%1 (%2)" ).
					arg( metaLVI->metaContact()->displayName(), metaLVI->metaContact()->statusString() );

				if ( title.length() > 43 )
					title = title.left( 40 ) + QString::fromLatin1( "..." );

				if ( popup->title( 0 ).isNull() )
					popup->insertTitle ( title, 0, 0 );
				else
					popup->changeTitle ( 0, title );

				// Submenus for separate contact actions
				bool sep = false;  //FIXME: find if there is already a separator in the end - Olivier
				QPtrList<KopeteContact> it = metaLVI->metaContact()->contacts();
				for( KopeteContact *c = it.first(); c; c = it.next() )
				{
					if( sep )
					{
						popup->insertSeparator();
						sep = false;
					}

					KPopupMenu *contactMenu = it.current()->popupMenu();
					connect( popup, SIGNAL( aboutToHide() ), contactMenu, SLOT( deleteLater() ) );
					QString nick=c->property(Kopete::Global::Properties::self()->nickName()).value().toString();
					QString text= nick.isEmpty() ?  c->contactId() : i18n( "Translators: format: '<displayName> (<id>)'", "%2 <%1>" ). arg( c->contactId(), nick );

					if ( text.length() > 41 )
						text = text.left( 38 ) + QString::fromLatin1( "..." );

					popup->insertItem( c->onlineStatus().iconFor( c, 16 ), text , contactMenu );
				}

				popup->popup( point );
			}
		}
	}
	else if ( groupvi && nb == 1 )
	{
		KPopupMenu *popup = dynamic_cast<KPopupMenu *>(
			window->factory()->container( "group_popup", window ) );
		if ( popup )
		{
			QString title = groupvi->group()->displayName();
			if ( title.length() > 32 )
				title = title.left( 30 ) + QString::fromLatin1( "..." );

			if( popup->title( 0 ).isNull() )
				popup->insertTitle( title, 0, 0 );
			else
				popup->changeTitle( 0, title );

			popup->popup( point );
		}
	}
	else if ( nb >= 1 )
	{
		KPopupMenu *popup = dynamic_cast<KPopupMenu *>(
			window->factory()->container( "contactlistitems_popup", window ) );
		if ( popup )
			popup->popup( point );
	}
	else
	{
		KPopupMenu *popup = dynamic_cast<KPopupMenu *>(
			window->factory()->container( "contactlist_popup", window ) );
		if ( popup )
		{
			if ( popup->title( 0 ).isNull() )
				popup->insertTitle( i18n( "Kopete" ), 0, 0 );

			popup->popup( point );
		}
	}
}

void KopeteContactListView::slotShowAddContactDialog()
{
	( new AddContactWizard( Kopete::UI::Global::mainWidget() ) )->show();
}

void KopeteContactListView::slotSettingsChanged( void )
{
	mShowAsTree = KopetePrefs::prefs()->treeView();
	if ( mShowAsTree )
	{
		setRootIsDecorated( true );
		setTreeStepSize( 20 );
	}
	else
	{
		setRootIsDecorated( false );
		setTreeStepSize( 0 );
	}

	// maybe setEffects should read these from KopetePrefs itself?
	Kopete::UI::ListView::Item::setEffects( KopetePrefs::prefs()->contactListAnimation(),
	                                        KopetePrefs::prefs()->contactListFading(),
	                                        KopetePrefs::prefs()->contactListFolding() );

	d->updateViewStrategy( this );

	slotUpdateAllGroupIcons();
	update();
}

void KopeteContactListView::slotUpdateAllGroupIcons()
{
	// FIXME: groups can (should?) do this for themselves
	// HACK: assume all groups are top-level. works for now, until the fixme above is dealt with
	for ( QListViewItem *it = firstChild(); it; it = it->nextSibling() )
		if ( KopeteGroupViewItem *gpi = dynamic_cast<KopeteGroupViewItem*>( it ) )
			gpi->updateIcon();
}

void KopeteContactListView::slotExecuted( QListViewItem *item, const QPoint &p, int /* col */ )
{
	item->setSelected( false );
	KopeteMetaContactLVI *metaContactLVI = dynamic_cast<KopeteMetaContactLVI *>( item );

	QPoint pos = viewport()->mapFromGlobal( p );
	KopeteContact *c = 0L;
	if ( metaContactLVI )
	{
		// Try if we are clicking a protocol icon. If so, open a direct
		// connection for that protocol
		QRect r = itemRect( item );
		QPoint relativePos( pos.x() - r.left() - ( treeStepSize() *
			( item->depth() + ( rootIsDecorated() ? 1 : 0 ) ) +
			itemMargin() ), pos.y() - r.top() );
		c = metaContactLVI->contactForPoint( relativePos );
		if( c )
			c->execute();
		else
			metaContactLVI->execute();
	}
}

void KopeteContactListView::slotContactStatusChanged( KopeteMetaContact *mc )
{
	d->viewStrategy->metaContactStatusChanged( mc );
}

void KopeteContactListView::slotDropped(QDropEvent *e, QListViewItem *, QListViewItem *after)
{
	if(!acceptDrag(e))
		return;

	KopeteMetaContactLVI *dest_metaLVI=dynamic_cast<KopeteMetaContactLVI*>(after);
	KopeteGroupViewItem *dest_groupLVI=dynamic_cast<KopeteGroupViewItem*>(after);
	QPtrListIterator<KopeteMetaContactLVI> it( m_selectedContacts );

	while ( it.current() )
	{
		KopeteContact *source_contact=0L;
		KopeteMetaContactLVI *source_metaLVI = it.current();
		++it;

		if(source_metaLVI)
			source_contact = source_metaLVI->contactForPoint( m_startDragPos );

		if(source_metaLVI  && dest_groupLVI)
		{
			if(source_metaLVI->group() == dest_groupLVI->group())
				return;

			if(source_metaLVI->metaContact()->isTemporary())
			{
				int r=KMessageBox::questionYesNo( Kopete::UI::Global::mainWidget(),
					i18n( "<qt>Would you like to add this contact to your contact list?</qt>" ),
					i18n( "Kopete" ), KStdGuiItem::yes(), KStdGuiItem::no(),
					"addTemporaryWhenMoving" );

				if( r == KMessageBox::Yes )
				{
					source_metaLVI->metaContact()->setTemporary( false, dest_groupLVI->group() );
					
					insertUndoItem( new UndoItem( UndoItem::MetaContactAdd , source_metaLVI->metaContact(),dest_groupLVI->group()  ) );
				}
			}
			else
			{
				source_metaLVI->metaContact()->moveToGroup(source_metaLVI->group() , dest_groupLVI->group() );

				insertUndoItem( new UndoItem( UndoItem::MetaContactCopy , source_metaLVI->metaContact() , dest_groupLVI->group() ) );
				
				UndoItem *u=new UndoItem( UndoItem::MetaContactRemove, source_metaLVI->metaContact(), source_metaLVI->group() );
				u->isStep=false;
				insertUndoItem(u);
			}
		}
		else if(source_metaLVI  && !dest_metaLVI && !dest_groupLVI)
		{
			if ( source_metaLVI->group()->type() == KopeteGroup::TopLevel )
				return;

			if(source_metaLVI->metaContact()->isTemporary())
			{
				int r=KMessageBox::questionYesNo( Kopete::UI::Global::mainWidget(),
					i18n( "<qt>Would you like to add this contact to your contact list?</qt>" ),
					i18n( "Kopete" ), KStdGuiItem::yes(), KStdGuiItem::no(),
					"addTemporaryWhenMoving" );

				if ( r == KMessageBox::Yes )
				{
					source_metaLVI->metaContact()->setTemporary( false, KopeteGroup::topLevel() );

					insertUndoItem( new UndoItem(UndoItem::MetaContactAdd, source_metaLVI->metaContact() ) );
				}
			}
			else
			{
				/*kdDebug(14000) << "KopeteContactListView::slotDropped : moving the meta contact "
					<< source_metaLVI->metaContact()->displayName() << " to top-level " << endl;*/
				source_metaLVI->metaContact()->moveToGroup( source_metaLVI->group(), KopeteGroup::topLevel() );
				
				insertUndoItem( new UndoItem( UndoItem::MetaContactCopy , source_metaLVI->metaContact() , KopeteGroup::topLevel() ) );
				
				UndoItem *u=new UndoItem( UndoItem::MetaContactRemove, source_metaLVI->metaContact(), source_metaLVI->group() );
				u->isStep=false;
				insertUndoItem(u);
			}
		}
		else if(source_contact && dest_metaLVI) //we are moving a contact to another metacontact
		{
			if(source_metaLVI->metaContact()->isTemporary())
			{
				int r=KMessageBox::questionYesNo( Kopete::UI::Global::mainWidget(),
					i18n( "<qt>Would you like to add this contact to your contact list?</qt>" ),
					i18n( "Kopete" ), KStdGuiItem::yes(), KStdGuiItem::no(),
					"addTemporaryWhenMoving" );

				if( r == KMessageBox::Yes )
				{
					source_contact->setMetaContact(dest_metaLVI->metaContact());
					
					UndoItem *u=new UndoItem;
					u->type=UndoItem::ContactAdd;
					u->args << source_contact->protocol()->pluginId() << source_contact->account()->accountId() << source_contact->contactId();
					insertUndoItem(u);
				}
			}
			else
			{
				//kdDebug(14000) << "KopeteContactListView::slotDropped : moving the contact "
				//	<< source_contact->contactId()	<< " to metacontact " <<
				//	dest_metaLVI->metaContact()->displayName() << endl;
				
				
				UndoItem *u=new UndoItem;
				u->type=UndoItem::MetaContactChange;
				u->metacontact=source_metaLVI->metaContact();
				u->group=source_metaLVI->group();
				u->args << source_contact->protocol()->pluginId() << source_contact->account()->accountId() << source_contact->contactId();
				u->args << source_metaLVI->metaContact()->displayName();
				insertUndoItem(u);
				
				source_contact->setMetaContact(dest_metaLVI->metaContact());
			}
		}
	}
/*	else if(source_groupLVI && dest_groupLVI)
	{
		if(source_groupLVI->group()->parentGroup()  == dest_groupLVI->group() )
			return;
		source_groupLVI->group()->setParentGroup( dest_metaLVI->group() );
	}
 	else if(source_groupLVI && !dest_groupLVI && dest_metaLVI)
	{
		if(source_groupLVI->group()->parentGroup() == KopeteGroup::toplevel)
			return;
		source_groupLVI->group()->setParentGroup( KopeteGroup::toplevel );
	}*/

	if( e->provides( "text/uri-list" ) )
	{
		if ( !QUriDrag::canDecode( e ) )
		{
			e->ignore();
			return;
		}

		KURL::List urlList;
		KURLDrag::decode( e, urlList );

		QPoint p=contentsToViewport(e->pos());
		int px = p.x() - ( header()->sectionPos( header()->mapToIndex( 0 ) ) +
			treeStepSize() * ( dest_metaLVI->depth() + ( rootIsDecorated() ? 1 : 0 ) ) + itemMargin() );
		int py = p.y() - itemRect( dest_metaLVI ).y();
		KopeteContact *c = dest_metaLVI->contactForPoint( QPoint( px, py ) ) ;

		for ( KURL::List::Iterator it = urlList.begin(); it != urlList.end(); ++it )
		{
			if( (*it).isLocalFile() )
			{ //send a file
				if(c)
					c->sendFile( *it );
				else
					dest_metaLVI->metaContact()->sendFile( *it );
			}
			else
			{ //this is a URL, send the URL in a message
				if(!c)
				{
					// We need to know which contact was chosen as the preferred
					// in order to message it
					c = dest_metaLVI->metaContact()->execute();
				}

				if (!c)
					return;

				KopeteMessage msg(c->account()->myself(), c, (*it).url(),
					KopeteMessage::Outbound);
				c->manager(true)->sendMessage(msg);
			}
		}
		e->acceptAction();
	}

}

bool KopeteContactListView::acceptDrag(QDropEvent *e) const
{
	QListViewItem *source=currentItem();
	QListViewItem *parent;
	QListViewItem *afterme;
	// Due to a little design problem in KListView::findDrop() we can't
	// call it directly from a const method until KDE 4.0, but as the
	// method is in fact const we can of course get away with a
	// const_cast...
	const_cast<KopeteContactListView *>( this )->findDrop( e->pos(), parent, afterme );

	KopeteMetaContactLVI *dest_metaLVI=dynamic_cast<KopeteMetaContactLVI*>(afterme);

	if( const_cast<const QWidget *>( e->source() ) == this )
	{
		KopeteMetaContactLVI *source_metaLVI=dynamic_cast<KopeteMetaContactLVI*>(source);
		KopeteGroupViewItem *dest_groupLVI=dynamic_cast<KopeteGroupViewItem*>(afterme);
		KopeteContact *source_contact=0L;

		if(source_metaLVI)
			source_contact = source_metaLVI->contactForPoint( m_startDragPos );

		if( source_metaLVI && dest_groupLVI && !source_contact)
		{	//we are moving a metacontact to another group
			if(source_metaLVI->group() == dest_groupLVI->group())
				return false;
			if ( dest_groupLVI->group()->type() == KopeteGroup::Temporary )
				return false;
	//		if(source_metaLVI->metaContact()->isTemporary())
	//			return false;
			return true;
		}
		else if(source_metaLVI  && !dest_metaLVI && !dest_groupLVI && !source_contact)
		{	//we are moving a metacontact to toplevel
			if ( source_metaLVI->group()->type() == KopeteGroup::TopLevel )
				return false;
	//		if(source_metaLVI->metaContact()->isTemporary())
	//			return false;

			return true;
		}
		else if(source_contact && dest_metaLVI) //we are moving a contact to another metacontact
		{
			if(source_contact->metaContact() == dest_metaLVI->metaContact() )
				return false;
			if(dest_metaLVI->metaContact()->isTemporary())
				return false;
			return true;
		}
/*		else if(source_groupLVI && dest_groupLVI) //we are moving a group to another group
		{
			if(dest_groupLVI->group() == KopeteGroup::temporary)
				return false;
			if(source_groupLVI->group() == KopeteGroup::temporary)
				return false;
			if(source_groupLVI->group()->parentGroup()  == dest_groupLVI->group() )
				return false;
			KopeteGroup *g=dest_groupLVI->group()->parentGroup();
			while(g && g != KopeteGroup::toplevel)
			{
				if(g==source_groupLVI->group())
					return false;
				g=g->parentGroup();
			}
			return true;
		}
		else if(source_groupLVI && !dest_groupLVI && dest_metaLVI) //we are moving a group to toplevel
		{
			if(source_groupLVI->group() == KopeteGroup::temporary)
				return false;
			if(source_groupLVI->group()->parentGroup() == KopeteGroup::toplevel)
				return false;
			return true;
		}*/
	}
	else
	{
		if ( e->provides( "text/uri-list" )  && dest_metaLVI &&
			dest_metaLVI->metaContact()->canAcceptFiles() )
		{ //we are sending a file
			QPoint p=contentsToViewport(e->pos());
			int px = p.x() - ( header()->sectionPos( header()->mapToIndex( 0 ) ) +
				treeStepSize() * ( dest_metaLVI->depth() + ( rootIsDecorated() ? 1 : 0 ) ) + itemMargin() );
			int py = p.y() - itemRect( dest_metaLVI ).y();
			KopeteContact *c = dest_metaLVI->contactForPoint( QPoint( px, py ) ) ;

			if( c ? !c->isReachable() : !dest_metaLVI->metaContact()->isReachable() )
				return false; //If the pointed contact is not reachable, abort

			if( c ? c->canAcceptFiles() : dest_metaLVI->metaContact()->canAcceptFiles()  )
			{
				// If the pointed contact, or the metacontact if no contact are
				// pointed can accept file, return true in everycase
				return true;
			}
			else
			{
				if ( !QUriDrag::canDecode( e ) )
					return false;
				KURL::List urlList;
				KURLDrag::decode( e, urlList );

				for ( KURL::List::Iterator it = urlList.begin(); it != urlList.end(); ++it )
				{
					if( (*it).isLocalFile() )
						return false; //we can't send links if a locale file is in link
				}
				return true; //we will send a link
			}
		}
		else
		{
			QString text;
			QTextDrag::decode(e, text);
			kdDebug(14000) << k_funcinfo << "drop with mimetype:" <<
				e->format() << " data as text:" << text << endl;
		}

	}

	return false;
}

void KopeteContactListView::findDrop(const QPoint &pos, QListViewItem *&parent,
	QListViewItem *&after)
{
	//Since KDE 3.1.1 ,  the original find Drop return 0L for afterme if the group is open.
	//This woraround allow us to keep the highlight of the item, and give always a correct position
	parent=0L;
	QPoint p (contentsToViewport(pos));
	after=itemAt(p);
}


void KopeteContactListView::contentsMousePressEvent( QMouseEvent *e )
{
	KListView::contentsMousePressEvent( e );
	if (e->button() == LeftButton )
	{
		QPoint p=contentsToViewport(e->pos());
		QListViewItem *i=itemAt( p );
		if( i )
		{
			//Maybe we are starting a drag?
			//memorize the position to know later if the user move a small contacticon

			int px = p.x() - ( header()->sectionPos( header()->mapToIndex( 0 ) ) +
				treeStepSize() * ( i->depth() + ( rootIsDecorated() ? 1 : 0 ) ) + itemMargin() );
			int py = p.y() - itemRect( i ).y();

			m_startDragPos = QPoint( px , py );
		}
	}
}

void KopeteContactListView::slotNewMessageEvent(KopeteEvent *event)
{
	KopeteMessage msg=event->message();
	//only for single chat
	if(msg.from() && msg.to().count()==1)
	{
		KopeteMetaContact *m=msg.from()->metaContact();
		if(!m)
			return;

		for ( QListViewItem *item = firstChild(); item; item = nextItem(item) )
			if ( KopeteMetaContactLVI *li = dynamic_cast<KopeteMetaContactLVI*>(item) )
				if ( li->metaContact() == m )
					li->catchEvent(event);
	}
}

QDragObject *KopeteContactListView::dragObject()
{
	// Discover what the drag started on.
	// If it's a MetaContactLVI, it was either on the MCLVI itself
	// or on one of the child contacts
	// Once we know this,
	// we set the pixmap for the drag to the MC's pixmap
	// or the child contact's small icon

	QListViewItem *currentLVI = currentItem();
	if( !currentLVI )
		return 0L;

	KopeteMetaContactLVI *metaLVI = dynamic_cast<KopeteMetaContactLVI*>( currentLVI );
	if( !metaLVI )
		return 0L;

	QPixmap pm;
	KopeteContact *c = metaLVI->contactForPoint( m_startDragPos );
        KMultipleDrag *drag = new KMultipleDrag( this );
	drag->addDragObject( new QStoredDrag("application/x-qlistviewitem", 0L ) );

	if ( c ) 	// dragging a contact
		pm = c->onlineStatus().iconFor( c, 12 ); // FIXME: fixed icon scaling
	else		// dragging a metacontact
	{
		// FIXME: first start at rendering the whole MC incl small icons
		// into a pixmap to drag - anyone know how to complete this?
		//QPainter p( pm );
		//source_metaLVI->paintCell( p, cg?, width(), 0, 0 );
		pm = SmallIcon( metaLVI->metaContact()->statusIcon() );
	}

	KABC::Addressee address = KABC::StdAddressBook::self()->findByUid(
		metaLVI->metaContact()->metaContactId()
	);

	if( !address.isEmpty() )
	{
		drag->addDragObject( new QTextDrag( address.fullEmail(), this ) );
		KABC::VCardConverter converter;
		QString vcard = converter.createVCard( address );
		if( !vcard.isNull() )
		{
			QStoredDrag *vcardDrag = new QStoredDrag("text/x-vcard", 0L );
			vcardDrag->setEncodedData( vcard.utf8() );
			drag->addDragObject( vcardDrag );
		}
	}

	//QSize s = pm.size();
	drag->setPixmap( pm /*, QPoint( s.width() , s.height() )*/ );

	return drag;
}

void KopeteContactListView::slotViewSelectionChanged()
{
	QPtrList<KopeteMetaContact> contacts;
	QPtrList<KopeteGroup> groups;

	m_selectedContacts.clear();
	m_selectedGroups.clear();

	QListViewItemIterator it( this );
	while ( it.current() )
	{
		QListViewItem *item = it.current();
		++it;

		if ( item->isSelected() )
		{
			KopeteMetaContactLVI *metaLVI=dynamic_cast<KopeteMetaContactLVI*>(item);
			if(metaLVI)
			{
				m_selectedContacts.append( metaLVI );
				contacts.append( metaLVI->metaContact() );
			}
			KopeteGroupViewItem *groupLVI=dynamic_cast<KopeteGroupViewItem*>(item);
			if(groupLVI)
			{
				m_selectedGroups.append( groupLVI );
				groups.append( groupLVI->group() );
			}
		}
	}

	// will cause slotListSelectionChanged to be called to update our actions.
	KopeteContactList::contactList()->setSelectedItems(contacts , groups);
}

void KopeteContactListView::slotListSelectionChanged()
{
	QPtrList<KopeteMetaContact> contacts = KopeteContactList::contactList()->selectedMetaContacts();
	QPtrList<KopeteGroup> groups = KopeteContactList::contactList()->selectedGroups();

	//TODO: update the list to select the items that should be selected.
	// make sure slotViewSelectionChanged is *not* called.
	updateActionsForSelection( contacts, groups );
}

void KopeteContactListView::updateActionsForSelection(
	QPtrList<KopeteMetaContact> contacts, QPtrList<KopeteGroup> groups )
{
	bool singleContactSelected = groups.isEmpty() && contacts.count() == 1;
	actionSendFile->setEnabled( singleContactSelected && contacts.first()->canAcceptFiles());
	actionAddContact->setEnabled( singleContactSelected && !contacts.first()->isTemporary());
	actionSendEmail->setEnabled( singleContactSelected && !contacts.first()->metaContactId().isEmpty() );
	actionSyncKABC->setEnabled( singleContactSelected && !contacts.first()->metaContactId().isEmpty() );

	if( singleContactSelected )
	{
		actionRename->setText(i18n("Rename Contact"));
		actionRemove->setText(i18n("Remove Contact"));
		actionRename->setEnabled(true);
		actionRemove->setEnabled(true);
	}
	else if( groups.count() == 1 && contacts.isEmpty() )
	{
		actionRename->setText(i18n("Rename Group"));
		actionRemove->setText(i18n("Remove Group"));
		actionRename->setEnabled(true);
		actionRemove->setEnabled(true);
	}
	else
	{
		actionRename->setText(i18n("Rename"));
		actionRemove->setText(i18n("Remove"));
		actionRename->setEnabled(false);
		actionRemove->setEnabled(contacts.count()+groups.count());
	}

	actionMove->setCurrentItem( -1 );
	actionCopy->setCurrentItem( -1 );

	actionProperties->setEnabled( ( groups.count() + contacts.count() ) == 1 );
}

void KopeteContactListView::slotSendMessage()
{
	KopeteMetaContact *m=KopeteContactList::contactList()->selectedMetaContacts().first();
	if(m)
		m->sendMessage();
}

void KopeteContactListView::slotStartChat()
{
	KopeteMetaContact *m=KopeteContactList::contactList()->selectedMetaContacts().first();
	if(m)
		m->startChat();
}

void KopeteContactListView::slotSendFile()
{
	KopeteMetaContact *m=KopeteContactList::contactList()->selectedMetaContacts().first();
	if(m)
		m->sendFile(KURL());
}

 void KopeteContactListView::slotSendEmail()
{
	//I borrowed this from slotSendMessage
	KopeteMetaContact *m=KopeteContactList::contactList()->selectedMetaContacts().first();
	if ( !m->metaContactId().isEmpty( ) ) // check if in kabc
	{
		KABC::Addressee addressee = KABC::StdAddressBook::self()->findByUid( m->metaContactId() );
		if ( !addressee.isEmpty() )
		{
			QString emailAddr = addressee.fullEmail();

			kdDebug( 14000 ) << "Email: " << emailAddr << "!" << endl;
			if ( !emailAddr.isEmpty() )
				kapp->invokeMailer( emailAddr, QString::null );
			else
				KMessageBox::queuedMessageBox( this, KMessageBox::Sorry, i18n( "There is no email address set for this contact in the KDE address book." ), i18n( "No Email Address in Address Book" ) );
		}
		else
			KMessageBox::queuedMessageBox( this, KMessageBox::Sorry, i18n( "This contact was not found in the KDE address book. Check that a contact is selected in the properties dialog." ), i18n( "Not Found in Address Book" ) );
	}
	else
		KMessageBox::queuedMessageBox( this, KMessageBox::Sorry, i18n( "This contact is not associated with a KDE address book entry, where the email address is stored. Check that a contact is selected in the properties dialog." ), i18n( "Not Found in Address Book" ) );
}

void KopeteContactListView::slotSyncKABC()
{
	KopeteMetaContact *m = KopeteContactList::contactList()->selectedMetaContacts().first();
	if( m )
		if ( !m->syncWithKABC() )
			KMessageBox::queuedMessageBox( this, KMessageBox::Information, i18n( "No contacts were added to Kopete from the address book" ), i18n( "No Change" ) );
}


void KopeteContactListView::slotMoveToGroup()
{
	KopeteMetaContactLVI *metaLVI=dynamic_cast<KopeteMetaContactLVI*>(currentItem());
	if(!metaLVI)
		return;
	KopeteMetaContact *m=metaLVI->metaContact();
	KopeteGroup *g=metaLVI->group();

	//FIXME What if two groups have the same name?
	KopeteGroup *to = actionMove->currentItem() ?
		KopeteContactList::contactList()->getGroup( actionMove->currentText() ) :
		KopeteGroup::topLevel();

	if( !to || to->type() == KopeteGroup::Temporary )
		return;

	if(m->isTemporary())
	{
		if( KMessageBox::questionYesNo( Kopete::UI::Global::mainWidget(),
			i18n( "<qt>Would you like to add this contact to your contact list?</qt>" ),
			i18n( "Kopete" ), KStdGuiItem::yes(), KStdGuiItem::no(),
			"addTemporaryWhenMoving" ) == KMessageBox::Yes )
		{
			m->setTemporary(false,to);
			
			insertUndoItem( new UndoItem( UndoItem::MetaContactAdd , m  ) );
		}
	}
	else if( !m->groups().contains( to ) )
	{
		m->moveToGroup( g, to );
		
		insertUndoItem( new UndoItem( UndoItem::MetaContactCopy , m , to ) );
				
		UndoItem *u=new UndoItem( UndoItem::MetaContactRemove, m, g );
		u->isStep=false;
		insertUndoItem(u);
	}

	actionMove->setCurrentItem( -1 );
}

void KopeteContactListView::slotCopyToGroup()
{
	KopeteMetaContact *m =
		KopeteContactList::contactList()->selectedMetaContacts().first();

	if(!m)
		return;

	//FIXME! what if two groups have the same name?
	KopeteGroup *to = actionCopy->currentItem() ?
		KopeteContactList::contactList()->getGroup( actionCopy->currentText() ) :
		KopeteGroup::topLevel();

	if( !to || to->type() == KopeteGroup::Temporary )
		return;

	if( m->isTemporary() )
		return;

	if( !m->groups().contains( to ) )
	{
		m->addToGroup( to );
	
		insertUndoItem( new UndoItem( UndoItem::MetaContactCopy , m , to ) );
	}

	actionCopy->setCurrentItem( -1 );
}

void KopeteContactListView::slotRemoveFromGroup()
{
	KopeteMetaContactLVI *metaLVI=dynamic_cast<KopeteMetaContactLVI*>(currentItem());
	if(!metaLVI)
		return;
	KopeteMetaContact *m=metaLVI->metaContact();

	if(m->isTemporary())
		return;

	m->removeFromGroup( metaLVI->group() );
	
	insertUndoItem( new UndoItem( UndoItem::MetaContactRemove , m , metaLVI->group() ) );
}

void KopeteContactListView::slotRemove()
{
	QPtrList<KopeteMetaContact> contacts = KopeteContactList::contactList()->selectedMetaContacts();
	QPtrList<KopeteGroup> groups = KopeteContactList::contactList()->selectedGroups();

	if(groups.count() + contacts.count() == 0)
		return;

	QStringList items;
	for( KopeteGroup *it = groups.first(); it; it = groups.next() )
	{
		if(!it->displayName().isEmpty())
			items.append( it->displayName() );
	}
	for( KopeteMetaContact *it = contacts.first(); it; it = contacts.next() )
	{
		if(!it->displayName().isEmpty())
			items.append( it->displayName() );
	}

	if( items.count() <= 1 )
	{
		// we are deleting an empty contact
		QString msg;
		if( !contacts.isEmpty() )
		{
			msg = i18n( "<qt>Are you sure you want to remove the contact <b>%1</b>" \
			            " from your contact list?</qt>" )
			      .arg( contacts.first()->displayName() ) ;
		}
		else if( !groups.isEmpty() )
		{
			msg = i18n( "<qt>Are you sure you want to remove the group <b>%1</b> " \
			            "and all contacts that are contained within it?</qt>" )
			      .arg( groups.first()->displayName() );
		}
		else
			return; // this should never happen

		if( KMessageBox::warningContinueCancel( this, msg, i18n( "Remove" ), KGuiItem(i18n("Remove"),"editdelete") ) !=
			KMessageBox::Continue )
		{
			return;
		}
	}
	else
	{
		QString msg = groups.isEmpty() ?
		   i18n( "Are you sure you want to remove these contacts " \
		         "from your contact list?" ) :
		   i18n( "Are you sure you want to remove these groups and " \
		         "contacts from your contact list?" );

		if( KMessageBox::warningContinueCancelList( this, msg, items, i18n("Remove"),
			KGuiItem(i18n("Remove"),"editdelete"), "askRemovingContactOrGroup" )
			!= KMessageBox::Continue )
		{
			return;
		}
	}

	for( KopeteMetaContact *it = contacts.first(); it; it = contacts.next() )
	{
		KopeteContactList::contactList()->removeMetaContact( it );
	}

	for( KopeteGroup *it = groups.first(); it; it = groups.next() )
	{
		QPtrList<KopeteMetaContact> list = it->members();
		for( list.first(); list.current(); list.next() )
		{
			KopeteMetaContact *mc = list.current();
			if(mc->groups().count()==1)
				KopeteContactList::contactList()->removeMetaContact(mc);
			else
				mc->removeFromGroup(it);
		}

		if( !it->members().isEmpty() )
		{
			kdDebug(14000) << "KopeteContactListView::slotRemove(): "
				<< "all subMetaContacts are not removed... Aborting" << endl;
			continue;
		}

		KopeteContactList::contactList()->removeGroup( it );
	}
}

void KopeteContactListView::slotRename()
{
	if ( KopeteMetaContactLVI *metaLVI = dynamic_cast<KopeteMetaContactLVI *>( currentItem() ) )
	{
		metaLVI->setRenameEnabled( 0, true);
		metaLVI->startRename( 0 );
	}
	else if ( KopeteGroupViewItem *groupLVI = dynamic_cast<KopeteGroupViewItem *>( currentItem() ) )
	{
		if ( !KopetePrefs::prefs()->sortByGroup() )
			return;
		groupLVI->setRenameEnabled( 0, true);
		groupLVI->startRename( 0 );
	}
}

void KopeteContactListView::slotAddContact()
{
	if( !sender() )
		return;

	KopeteMetaContact *metacontact =
		KopeteContactList::contactList()->selectedMetaContacts().first();
	KopeteAccount *account = dynamic_cast<KopeteAccount*>( sender()->parent() );

	if( account && metacontact )
	{
		if ( metacontact->isTemporary() )
			return;

		KDialogBase *addDialog = new KDialogBase( this, "addDialog", true,
			i18n( "Add Contact" ), KDialogBase::Ok|KDialogBase::Cancel,
			KDialogBase::Ok, true );

		AddContactPage *addContactPage =
			account->protocol()->createAddContactWidget( addDialog, account );

		if (!addContactPage)
		{
			kdDebug(14000) << k_funcinfo <<
				"Error while creating addcontactpage" << endl;
		}
		else
		{
			addDialog->setMainWidget( addContactPage );
			if( addDialog->exec() == QDialog::Accepted )
			{
				if( addContactPage->validateData() )
					addContactPage->apply( account, metacontact );
			}
		}
		addDialog->deleteLater();
	}
}

void KopeteContactListView::slotAddTemporaryContact()
{
	KopeteMetaContact *metacontact =
		KopeteContactList::contactList()->selectedMetaContacts().first();
	if( metacontact )
	{
/*		int r=KMessageBox::questionYesNo( Kopete::UI::Global::mainWidget(),
			i18n( "<qt>Would you like to add this contact to your contact list?</qt>" ),
			i18n( "Kopete" ), KStdGuiItem::yes(), KStdGuiItem::no(),
			"addTemporaryWhenMoving" );

		if(r==KMessageBox::Yes)*/
		if(metacontact->isTemporary() )
			metacontact->setTemporary( false );
	}
}

void KopeteContactListView::slotProperties()
{
//	kdDebug(14000) << k_funcinfo << "Called" << endl;

	KopeteMetaContactLVI *metaLVI =
		dynamic_cast<KopeteMetaContactLVI *>( currentItem() );
	KopeteGroupViewItem *groupLVI =
		dynamic_cast<KopeteGroupViewItem *>( currentItem() );

	if(metaLVI)
	{

		KopeteMetaLVIProps *propsDialog =
			new KopeteMetaLVIProps( metaLVI, 0L, "propsDialog" );

		propsDialog->exec(); // modal
		delete propsDialog;

		/*
		if( metaLVI->group()->useCustomIcon() )
		{
			metaLVI->updateCustomIcons( mShowAsTree );
		}
		else
		{
		}
		*/
		metaLVI->repaint();
	}
	else if(groupLVI)
	{
		KopeteGVIProps *propsDialog =
			new KopeteGVIProps( groupLVI, 0L, "propsDialog");

		propsDialog->exec(); // modal
		delete propsDialog;

		groupLVI->updateIcon();
	}
}

void KopeteContactListView::slotItemRenamed( QListViewItem */*item*/ )
{
	//everithing is now done in  KopeteMetaContactLVI::rename

/*	KopeteMetaContactLVI *metaLVI = dynamic_cast<KopeteMetaContactLVI *>( item );
	KopeteMetaContact *m= metaLVI ?  metaLVI->metaContact() : 0L ;
	if ( m )
	{
		m->setDisplayName( metaLVI->text( 0 ) );
	}
	else
	{
		//group are handled differently in KopeteGroupViewItem
	//	kdWarning( 14000 ) << k_funcinfo << "Unknown list view item '" << item
	//	                   << "' renamed, ignoring item" << endl;
	}
	*/
}

void KopeteContactListView::insertUndoItem( KopeteContactListView::UndoItem *u)
{
	u->next=m_undo;
	m_undo=u;
	actionUndo->setEnabled(true);
	while(m_redo)
	{
		UndoItem *i=m_redo->next;
		delete m_redo;
		m_redo=i;
	}
	actionRedo->setEnabled(false);
	undoTimer.start(3*60*1000); //the user has 3 minutes to undo.
}


void KopeteContactListView::slotUndo()
{
	bool step = false;
	while(m_undo && !step)
	{
		kdDebug( 14000 ) << k_funcinfo << m_undo->type << "    " << m_undo->args<< endl;
		bool success=false;
		switch (m_undo->type)
		{
		 case UndoItem::MetaContactAdd:
		 {
		 	KopeteMetaContact *m=m_undo->metacontact;
			if(m)
			{
				m->setTemporary(true);
				success=true;
			}
			break;
		 }
		 case UndoItem::MetaContactCopy:
		 {
		 	KopeteMetaContact *m=m_undo->metacontact;
			KopeteGroup *to=m_undo->group;
			if( m && to )
			{
				m->removeFromGroup( to );
				success=true;
			}
			break;
		 }
		 case UndoItem::MetaContactRemove:
		 {
		 	KopeteMetaContact *m=m_undo->metacontact;
			KopeteGroup *g=m_undo->group;
			if( m && g )
			{
				m->addToGroup( g );
				success=true;
			}
			break;
		 }
		 case UndoItem::MetaContactRename:
		 {
			KopeteMetaContact *m=m_undo->metacontact;
			if( m )
			{
				const QString old=m->displayName();
				if( m_undo->args[0].isEmpty() )
					m->setTrackChildNameChanges(true);
				else
					m->setDisplayName( m_undo->args[0] );
				m_undo->args[0]=old;
				success=true;
			}
			break;
		 }
		 case UndoItem::GroupRename:
		 {
			if( m_undo->group  )
			{
				const QString old=m_undo->group->displayName();
				m_undo->group->setDisplayName( m_undo->args[0] );
				m_undo->args[0]=old;
				success=true;
			}
			break;
		 }
		 case UndoItem::MetaContactChange:
		 {
		 	KopeteContact *c=0;
			KopeteMetaContact *m0=KopeteContactList::contactList()->findContact(m_undo->args[0] , m_undo->args[1], m_undo->args[2] ) ;
			if(m0)
				c=m0->findContact(m_undo->args[0] , m_undo->args[1], m_undo->args[2] );
			if(c)
			{
				success=true;
				if(m_undo->metacontact)
					c->setMetaContact(m_undo->metacontact);
				else
				{
					KopeteMetaContact *m=new KopeteMetaContact;
					m->addToGroup(m_undo->group);
					m->setDisplayName(m_undo->args[3]);
					c->setMetaContact(m);
					KopeteContactList::contactList()->addMetaContact(m);
				}
				m_undo->metacontact=m0; //for the redo
			}
			break;
		 }
		 case UndoItem::ContactAdd:
		 {
			KopeteContact *c=0;
			KopeteMetaContact *m0=KopeteContactList::contactList()->findContact(m_undo->args[0] , m_undo->args[1], m_undo->args[2] ) ;
			if(m0)
				c=m0->findContact(m_undo->args[0] , m_undo->args[1], m_undo->args[2] );
			if(c)
			{
				success=true;
				c->slotDeleteContact();
				m_undo->metacontact=m0;
			}
			break;
		 }
		}
		
		kdDebug( 14000 ) << k_funcinfo << success << endl;
		
		if(success) //the undo item has been correctly performed
		{
			step=m_undo->isStep;
			UndoItem *u=m_undo->next;
			m_undo->next=m_redo;
			m_redo=m_undo;
			m_undo=u;
		}
		else //something has been corrupted, clear all undo items
		{
			while(m_undo) 
			{
				UndoItem *u=m_undo->next;
				delete m_undo;
				m_undo=u;
			}
		}
	}
	actionUndo->setEnabled(m_undo);
	actionRedo->setEnabled(m_redo);
	undoTimer.start(3*60*1000);
}

void KopeteContactListView::slotRedo()
{
	bool step = false;
	while(m_redo && (!step || m_redo->isStep ))
	{
		kdDebug( 14000 ) << k_funcinfo << m_redo->type << "    " << m_redo->args<< endl;
		bool success=false;
		switch (m_redo->type)
		{
		 case UndoItem::MetaContactAdd:
		 {
		 	KopeteMetaContact *m=m_redo->metacontact;
			if(m && m_redo->group)
			{
				m->setTemporary(false,m_redo->group);
				success=true;
			}
			break;
		 }
		 case UndoItem::MetaContactCopy:
		 {
		 	KopeteMetaContact *m=m_redo->metacontact;
			KopeteGroup *to=m_redo->group;
			if( m && to )
			{
				m->addToGroup( to );
				success=true;
			}
			break;
		 }
		 case UndoItem::MetaContactRemove:
		 {
		 	KopeteMetaContact *m=m_redo->metacontact;
			KopeteGroup *g=m_redo->group;
			if( m && g )
			{
				m->removeFromGroup( g );
				success=true;
			}
			break;
		 }
		 case UndoItem::MetaContactRename:
		 {
			KopeteMetaContact *m=m_redo->metacontact;
			if( m )
			{
				const QString old=m->displayName();
				if( m_redo->args[0].isEmpty() )
					m->setTrackChildNameChanges(true);
				else
					m->setDisplayName( m_redo->args[0] );
				m_redo->args[0]=old;
				success=true;
			}
			break;
		 }
		 case UndoItem::GroupRename:
		 {
			if( m_redo->group  )
			{
				const QString old=m_redo->group->displayName();
				m_redo->group->setDisplayName( m_redo->args[0] );
				m_redo->args[0]=old;
				success=true;
			}
			break;
		 }
		 case UndoItem::MetaContactChange:
		 {
		 	KopeteContact *c=0;
			KopeteMetaContact *m0=KopeteContactList::contactList()->findContact(m_redo->args[0] , m_redo->args[1], m_redo->args[2] ) ;
			if(m0)
				c=m0->findContact(m_redo->args[0] , m_redo->args[1], m_redo->args[2] );
			if(c && m_redo->metacontact)
			{
				success=true;
				c->setMetaContact(m_redo->metacontact);
				m_redo->metacontact=m0;
			}
			break;
		 }
		 case UndoItem::ContactAdd:
		 {
			//impossible to add it back
			break;
		 }
		}
		
		kdDebug( 14000 ) << k_funcinfo << success << endl;
		
		if(success) //the undo item has been correctly performed
		{
			step|=m_redo->isStep;
			UndoItem *u=m_redo->next;
			m_redo->next=m_undo;
			m_undo=m_redo;
			m_redo=u;
		}
		else //something has been corrupted, clear all undo items
		{
			while(m_redo) 
			{
				UndoItem *u=m_redo->next;
				delete m_redo;
				m_redo=u;
			}
		}
	}
	actionUndo->setEnabled(m_undo);
	actionRedo->setEnabled(m_redo);
	undoTimer.start(3*60*1000);
}

void KopeteContactListView::slotTimeout()
{
	undoTimer.stop();
	while(m_undo) 
	{
		UndoItem *u=m_undo->next;
		delete m_undo;
		m_undo=u;
	}
	actionUndo->setEnabled(false);
	while(m_redo)
	{
		UndoItem *i=m_redo->next;
		delete m_redo;
		m_redo=i;
	}
	actionRedo->setEnabled(false);
}

#include "kopetecontactlistview.moc"

// vim: set noet ts=4 sts=4 sw=4:
