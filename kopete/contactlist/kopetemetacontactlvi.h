/*
    kopetemetacontactlvi.h - Kopete Meta Contact KListViewItem

    Copyright (c) 2002-2003 by Olivier Goffart        <ogoffart@tiscalinet.be>
    Copyright (c) 2002-2003 by Martijn Klingens       <klingens@kde.org>
    Copyright (c) 2002      by Duncan Mac-Vicar P     <duncan@kde.org>

    Kopete    (c) 2002-2003 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef __kopetemetacontactlvi_h__
#define __kopetemetacontactlvi_h__

#include <qobject.h>
#include <qpixmap.h>
#include <qptrdict.h>

#include <klistview.h>


class KAction;
class KListAction;
class KopeteAccount;
class KopeteMetaContact;

class AddContactPage;
class KopeteContact;
class KopeteGroupViewItem;
class KopeteGroup;
class KopeteEvent;

/**
 * @author Martijn Klingens <klingens@kde.org>
 */
class KopeteMetaContactLVI : public QObject, public KListViewItem
{
	Q_OBJECT

public:
	KopeteMetaContactLVI( KopeteMetaContact *contact, KopeteGroupViewItem *parent );
	KopeteMetaContactLVI( KopeteMetaContact *contact, QListViewItem *parent );
	KopeteMetaContactLVI( KopeteMetaContact *contact, QListView *parent );
	/**
	 * Copy constructor.
	 * Makes a second view of the same meta contact, if the meta contact
	 * resides in multiple groups.
	 */
//	KopeteMetaContactLVI( KopeteMetaContactLVI *other, QListViewItem *parent );
	~KopeteMetaContactLVI();

	/**
	 * metacontact this visual item represents
	 */
	KopeteMetaContact *metaContact() const
	{ return m_metaContact; };

	/**
	 * true if the item is at top level and not under a group
	 */
	bool isTopLevel() const;

	/**
	 * parent when top-level
	 */
	QListView *parentView() const { return m_parentView; };

	/**
	 * parent when not top-level
	 */
	KopeteGroupViewItem *parentGroup() const { return m_parentGroup; };

//	virtual void setup();
	virtual void paintCell ( QPainter *p, const QColorGroup &cg, int column, int width, int align );

	/* Duncan experiment */
/*	virtual void setColor( const QColor &color );
	virtual void setColor( const QString &color );
	virtual void setText( int column, const QString &text );
	QColor color();
	QString colorName();*/


	void movedToGroup(KopeteGroup * );
	void rename( const QString& name );

	KopeteGroup *group();

	/**
	 * Returns the KopeteContact of the small little icon at the point p
	 * @param p must be in the list view item's coordinate system.
	 * Returns a null pointer if p is not on a small icon.
	 * (This is used for e.g. the context-menu of a contact when
	 * right-clicking an icon, or the tooltips)
	 */
	KopeteContact *contactForPoint( const QPoint &p ) const;

	/**
	 * Returns the QRect small little icon used for the KopeteContact.
	 * The behaviour is undefined if @param c doesn't point to a valid
	 * KopeteContact for this list view item.
	 * The returned QRect is using the list view item's coordinate
	 * system and should probably be transformed into the list view's
	 * coordinates before being of any practical use.
	 * Note that the returned Rect is always vertically stretched to fill
	 * the full list view item's height, only the width is relative to
	 * the actual icon width.
	 */
	QRect contactRect( const KopeteContact *c ) const;

	/**
	 * Returns the first X position used by contact icons.
	 * Returns the entire width of the LVI if there are no KopeteContacts.
	 */
	uint firstContactIconX() const;

	/**
	 * Returns the last X position used by contact icons, i.e. the first
	 * position AFTER the last icon.
	 * Returns the entire width of the LVI if there are no KopeteContacts.
	 */
	uint lastContactIconX() const;

	bool isGrouped() const;

public slots:
	/**
	 * Call the meta contact's execute as I don't want to expose m_contact
	 * directly.
	 */
	void execute() const;

	void catchEvent(KopeteEvent *);

	void slotRename();

	void updateVisibility();

private slots:
	void slotUpdateIcons();
	void slotContactStatusChanged(KopeteContact *);

	void slotDisplayNameChanged();

	void slotAddTemporaryContact();

	void slotAddToNewGroup();
	void slotIdleStateChanged();

	/**
	 * maybe remove this and find a better way to check for pref-changes
	 */
	void slotConfigChanged();

	void slotEventDone(KopeteEvent* );
	void slotBlink();

protected:
	virtual void okRename(int col);

private:
	void initLVI();
	QString key( int column, bool ascending ) const;


	KopeteMetaContact *m_metaContact;

	// Used actions in the context menu
	KListAction *m_actionMove;
	KListAction *m_actionCopy;

	KopeteGroupViewItem *m_parentGroup;
	QListView *m_parentView;
	bool m_isTopLevel;

	int m_pixelWide;

	/*KopeteOnlineStatus::OnlineStatus*/ unsigned int m_oldStatus;
	QString m_oldStatusIcon;

	KopeteEvent *m_event;
	QTimer *mBlinkTimer;

	QPtrDict<KopeteAccount> m_addContactActions;

	bool mIsBlinkIcon;
	int m_blinkLeft;
};

#endif

// vim: set noet ts=4 sts=4 sw=4:

