/*
    Kopete Contact List View

    Copyright (c) 2001-2002 by Duncan Mac-Vicar Prett <duncan@kde.org>
    Copyright (c) 2002      by Nick Betcher           <nbetcher@usinternet.com>
    Copyright (c) 2002      by Stefan Gehn            <metz@gehn.net>
    Copyright (c) 2002-2005 by Olivier Goffart        <ogoffart@kde.org>
    Copyright (c) 2002-2003 by Martijn Klingens       <klingens@kde.org>
    Copyright (c) 2004      by Richard Smith          <kde@metafoo.co.uk>
    Copyright     2007      by Matt Rogers            <mattr@kde.org>

    Kopete    (c) 2002-2007 by the Kopete developers  <kopete-devel@kde.org>

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

#include <QHeaderView>

#include <KDebug>

#include "kopeteuiglobal.h"
#include "kopetecontactlistelement.h"
#include "kopetemetacontact.h"

#include "kopetegroupitem.h"
#include "contactlistmodel.h"
#include "kopeteitembase.h"
#include "kopeteitemdelegate.h"
#include "kopetemetacontactitem.h"


KopeteContactListView::KopeteContactListView( QWidget *parent )
	: QTreeView( parent )
{
	//d = new KopeteContactListViewPrivate;
	header()->hide();

	setModel( new Kopete::UI::ContactListModel( this ) );
	setAlternatingRowColors( true );
	setItemDelegate( new KopeteItemDelegate( this ) );

	connect( this, SIGNAL( activated(const QModelIndex&)),
	         this, SLOT( contactActivated(const QModelIndex&)));

	setEditTriggers( NoEditTriggers );
	// Load in the user's initial settings
	//slotSettingsChanged();
}
/*
void KopeteContactListView::initActions( KActionCollection *ac )
{
	actionUndo = KStandardAction::undo( this , SLOT( slotUndo() ) , ac );
	actionRedo = KStandardAction::redo( this , SLOT( slotRedo() ) , ac );
	actionUndo->setEnabled(false);
	actionRedo->setEnabled(false);


	KAction *actionCreateNewGroup = new KAction( i18n( "Create New Group..." ), ac );
	actionCreateNewGroup->setIcon( KIcon( "user-group-new" ) );
	connect( actionCreateNewGroup, SIGNAL( triggered(bool) ), this, SLOT( addGroup() ) );
        ac->addAction( "AddGroup", actionCreateNewGroup );

	actionSendMessage = KopeteStdAction::sendMessage(
		this, SLOT( slotSendMessage() ), ac, "contactSendMessage" );
	actionStartChat = KopeteStdAction::chat( this, SLOT( slotStartChat() ),
		ac, "contactStartChat" );

	actionMove = new KopeteGroupListAction( i18n( "&Move To" ), QLatin1String( "edit-cut" ),
                                                KShortcut(), this, SLOT( slotMoveToGroup() ), ac );
        ac->addAction( "contactMove", actionMove );
	actionCopy = new KopeteGroupListAction( i18n( "&Copy To" ), QLatin1String( "edit-copy" ),
                                                KShortcut(), this, SLOT( slotCopyToGroup() ), ac );
        ac->addAction( "contactCopy", actionCopy );

	actionMakeMetaContact = new KAction(KIcon("list-add-user"), i18n("Make Meta Contact"), ac);
        ac->addAction( "makeMetaContact", actionMakeMetaContact );
        connect (actionMakeMetaContact, SIGNAL(triggered(bool)), this, SLOT(slotMakeMetaContact()));

	actionRemove = KopeteStdAction::deleteContact( this, SLOT( slotRemove() ), ac );
        ac->addAction( "contactRemove", actionRemove );
	actionSendEmail = new KAction( KIcon("mail-send"), i18n( "Send Email..." ), ac );
        ac->addAction( "contactSendEmail", actionSendEmail );
	connect( actionSendEmail, SIGNAL( triggered(bool) ), this, SLOT( slotSendEmail() ) );
	-* this actionRename is buggy, and useless with properties, removed in kopeteui.rc*-
	actionRename = new KAction( KIcon("edit-rename"), i18n( "Rename" ), ac );
        ac->addAction( "contactRename", actionRename );
	connect( actionRename, SIGNAL( triggered(bool) ), this, SLOT( slotRename() ) );
	actionSendFile = KopeteStdAction::sendFile( this, SLOT( slotSendFile() ),
		ac, "contactSendFile" );

	actionAddContact = new KActionMenu( KIcon( QLatin1String("list-add-user") ), i18n( "&Add Contact" ), ac );
        ac->addAction( "contactAddContact", actionAddContact );
	actionAddContact->menu()->addTitle( i18n("Select Account") );

	actionAddTemporaryContact = new KAction( KIcon("list-add-user"), i18n( "Add to Your Contact List" ), ac );
        ac->addAction( "contactAddTemporaryContact", actionAddTemporaryContact );
	connect( actionAddTemporaryContact, SIGNAL( triggered(bool) ), this, SLOT( slotAddTemporaryContact() ) );

	connect( Kopete::ContactList::self(), SIGNAL( metaContactSelected( bool ) ), this, SLOT( slotMetaContactSelected( bool ) ) );

	connect( Kopete::AccountManager::self(), SIGNAL(accountRegistered( Kopete::Account* )), SLOT(slotAddSubContactActionNewAccount(Kopete::Account*)));
	connect( Kopete::AccountManager::self(), SIGNAL(accountUnregistered( const Kopete::Account* )), SLOT(slotAddSubContactActionAccountDeleted(const Kopete::Account *)));

	actionProperties = new KAction( KIcon("user-properties"), i18n( "&Properties" ), ac );
        ac->addAction( "contactProperties", actionProperties );
	actionProperties->setShortcut( KShortcut(Qt::Key_Alt + Qt::Key_Return) );
	connect( actionProperties, SIGNAL( triggered(bool) ), this, SLOT( slotProperties() ) );

	// Update enabled/disabled actions
	slotViewSelectionChanged();
}
*/

KopeteContactListView::~KopeteContactListView()
{
	//delete d;
}

void KopeteContactListView::contactActivated( const QModelIndex& index )
{
	kDebug(14010) << "contact activated";
	QVariant v = index.data( Kopete::Items::ElementRole );
	if ( index.data( Kopete::Items::TypeRole ) == Kopete::Items::MetaContact )
	{
		kDebug(14010) << "have a metacontact";
		QObject* o = v.value<QObject*>();
		Kopete::MetaContact* mc = qobject_cast<Kopete::MetaContact*>( o );
		if ( mc )
			mc->execute();
	}


}


#include "kopetecontactlistview.moc"

// vim: set noet ts=4 sts=4 sw=4:
