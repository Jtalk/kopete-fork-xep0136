/*
    kopetestdaction.cpp  -  Kopete Standard Actionds

    Copyright (c) 2001-2002 by Ryan Cumming          <ryan@kde.org>
    Copyright (c) 2002-2003 by Martijn Klingens      <klingens@kde.org>

    Kopete    (c) 2001-2003 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include "kopetestdaction.h"

#include <qapplication.h>

#include <kdebug.h>
#include <kdeversion.h>
#include <kguiitem.h>
#include <klocale.h>
#include <ksettings/dialog.h>
#include <kstdaction.h>
#include <kstdguiitem.h>

#include "kopetecontactlist.h"
#include "kopetegroup.h"

KopeteGroupListAction::KopeteGroupListAction( const QString &text, const QString &pix, const KShortcut &cut, const QObject *receiver,
	const char *slot, QObject *parent, const char *name )
: KListAction( text, pix, cut, parent, name )
{
	connect( this, SIGNAL( activated() ), receiver, slot );

	connect( KopeteContactList::contactList(), SIGNAL( groupAdded( KopeteGroup * ) ), this, SLOT( slotUpdateList() ) );
	connect( KopeteContactList::contactList(), SIGNAL( groupRemoved( KopeteGroup * ) ), this, SLOT( slotUpdateList() ) );
	slotUpdateList();
}

KopeteGroupListAction::~KopeteGroupListAction()
{
}

void KopeteGroupListAction::slotUpdateList()
{
	QStringList groupList;

	// Add groups to our list
	KopeteGroupList groups = KopeteContactList::contactList()->groups();
	for ( KopeteGroup *it = groups.first(); it; it = groups.next() )
	{
		// FIXME: I think handling the i18n for temporary and top level
		//        groups belongs in KopeteGroup instead.
		//        It's now duplicated in KopeteGroupListAction and
		//        KopeteGroupViewItem already - Martijn
		QString displayName;
		switch ( it->type() )
		{
		case KopeteGroup::Temporary:
			// Moving to temporary makes no sense, skip the group
			continue;
		case KopeteGroup::TopLevel:
			displayName = i18n( "(Top-Level)" );
			break;
		default:
			displayName = it->displayName();
			break;
		}
		groupList.append( displayName );
	}

	setItems( groupList );
}

KSettings::Dialog *KopetePreferencesAction::s_settingsDialog = 0L;

KopetePreferencesAction::KopetePreferencesAction( KActionCollection *parent, const char *name )
// FIXME: Pending kdelibs change, uncomment when it's in - Martijn
//#if KDE_IS_VERSION( 3, 1, 90 )
//: KAction( KStdGuiItem::preferences(), 0, 0, 0, parent, name )
//#else
: KAction( KGuiItem( i18n( "Translators: No translation needed for KDE 3.2, it's only used in 3.1", "&Configure Kopete..." ),
	QString::fromLatin1( "configure" ) ), 0, 0, 0, parent, name )
//#endif
{
	connect( this, SIGNAL( activated() ), this, SLOT( slotShowPreferences() ) );
}

KopetePreferencesAction::~KopetePreferencesAction()
{
}

void KopetePreferencesAction::slotShowPreferences()
{
	// FIXME: Use static deleter - Martijn
	if ( !s_settingsDialog )
		s_settingsDialog = new KSettings::Dialog( KSettings::Dialog::Static, qApp->mainWidget() );
	s_settingsDialog->show();
}

KAction* KopeteStdAction::preferences( KActionCollection *parent, const char *name )
{
	return new KopetePreferencesAction( parent, name );
}

KAction* KopeteStdAction::chat( const QObject *recvr, const char *slot, QObject* parent, const char *name )
{
	return new KAction( i18n( "Start &Chat" ), QString::fromLatin1( "mail_generic" ), 0, recvr, slot, parent, name );
}

KAction* KopeteStdAction::sendMessage(const QObject *recvr, const char *slot, QObject* parent, const char *name)
{
	return new KAction( i18n( "&Send Message" ), QString::fromLatin1( "mail_generic" ), 0, recvr, slot, parent, name );
}

KAction* KopeteStdAction::contactInfo(const QObject *recvr, const char *slot, QObject* parent, const char *name)
{
	return new KAction( i18n( "User &Info" ), QString::fromLatin1( "messagebox_info" ), 0, recvr, slot, parent, name );
}

KAction* KopeteStdAction::sendFile(const QObject *recvr, const char *slot, QObject* parent, const char *name)
{
	return new KAction( i18n( "Send &File..." ), QString::fromLatin1( "launch" ), 0, recvr, slot, parent, name );
}

KAction* KopeteStdAction::viewHistory(const QObject *recvr, const char *slot, QObject* parent, const char *name)
{
	return new KAction( i18n("View &History" ), QString::fromLatin1( "history" ), 0, recvr, slot, parent, name );
}

KAction* KopeteStdAction::addGroup(const QObject *recvr, const char *slot, QObject* parent, const char *name)
{
	return new KAction( i18n( "&Create Group" ), QString::fromLatin1( "folder" ), 0, recvr, slot, parent,
		name );
}

KAction* KopeteStdAction::changeMetaContact(const QObject *recvr, const char *slot, QObject* parent, const char *name)
{
	return new KAction( i18n( "Cha&nge Meta Contact..." ), QString::fromLatin1( "move" ), 0, recvr, slot, parent, name );
}

KListAction *KopeteStdAction::moveContact(const QObject *recvr, const char *slot, QObject* parent, const char *name)
{
	return new KopeteGroupListAction( i18n( "&Move To" ), QString::fromLatin1( "editcut" ), 0, recvr, slot, parent, name );
}

KListAction *KopeteStdAction::copyContact( const QObject *recvr, const char *slot, QObject* parent, const char *name )
{
	return new KopeteGroupListAction( i18n( "Cop&y To" ), QString::fromLatin1( "editcopy" ), 0, recvr, slot, parent, name );
}

KAction* KopeteStdAction::deleteContact(const QObject *recvr, const char *slot, QObject* parent, const char *name)
{
	return new KAction( i18n( "&Delete Contact" ), QString::fromLatin1( "edittrash" ), 0, recvr, slot, parent, name );
}

KAction* KopeteStdAction::changeAlias(const QObject *recvr, const char *slot, QObject* parent, const char *name)
{
	return new KAction( i18n( "Change A&lias..." ), QString::fromLatin1( "signature" ), 0, recvr, slot, parent, name );
}

#include "kopetestdaction.moc"

// vim: set noet ts=4 sts=4 sw=4:

