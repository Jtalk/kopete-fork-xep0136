/*
    kopeteonlinestatusmanager.h

    Copyright (c) 2004-2005 by Olivier Goffart  <ogoffart @ kde.org>

    Kopete    (c) 2004-2005 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#ifndef kopeteonlinestatusmanager_h__
#define kopeteonlinestatusmanager_h__

#include "kopete_export.h"

#include <QObject>
#include <QFlags>

#include "kaction.h"
#include "kactionmenu.h"

class QString;
class QPixmap;
class QColor;
class KActionMenu;

namespace Kopete
{
	class OnlineStatus;
	class Account;
	class Protocol;


/**
 * OnlineStatusManager is a singleton which manage OnlineStatus
 *
 * @author Olivier Goffart
 */
class KOPETE_EXPORT OnlineStatusManager : public QObject
{
	Q_OBJECT
public:
	static OnlineStatusManager* self();
	~OnlineStatusManager();

	/**
	 * Kopete will uses categories to have a more general system than simply globally away.
	 *
	 * Idealy, in each protocol, there should be one status per categories (status may be in several or in none categories
	 *
	 * Idle is the status used for auto-away
	 *
	 * Status number are organised so that make a tree.
	 */
	//please be carrefull when modifying values of status.  read comment in onlineStatus()
	enum Category
	{
		Idle=1<<8,  ExtendedAway=1<<9 , Invisible=1<<10,
		//  \     /             __________/
		  /*1<<4*/    Busy=1<<5,           FreeForChat=1<<6,         /* 1<<7*/
		//   \       /                         /
		     Away=1<<2,                   /* 1<<3 */
		//       \                           /
						Online=1<<1,
		Offline=1
	};
	Q_DECLARE_FLAGS(Categories, Category)


	/**
	 * @see registerOnlineStatus
	 */
	enum Option
	{
		/// The user may set status messages for this online status
		HasStatusMessage = 0x01,
		/// The action of the status will be disabled if the account is offline.
		/// use it if your protocol doesn't support connecting with the status as initial status.
		/// You praticaly shouldn't abuse of that, and automatically set status after connecting if possible
		DisabledIfOffline = 0x02,
		///  The status will not appears in the action menu. Used if you want to register the status for e.g. autoaway,
		///  without letting the user set itself that status
		HideFromMenu = 0x04
	};
	Q_DECLARE_FLAGS(Options, Option)

	/**
	 * You need to register each status an account can be.
	 * Registered statuses will appear in the account menu.
	 *
	 * The Protocol constructor is a good place to call this function.
	 * But if you want, you may use a special OnlineStatus constructor that call this function automatically
	 *
	 * You can set the status to be in the predefined categories.
	 * Ideally, each category should own one status.
	 * A status may be in several categories, or in none.
	 * There shouldn't be more than one status per protocol per categories.
	 *
	 * @param status The status to register
	 * @param caption The caption that will appear in menus (e.g. "Set &Away")
	 * @param categories A bitflag of OnlineStatusManager::Categories
	 * @param options is a bitflag of OnlineStatusManager::Options
	 */
	void registerOnlineStatus(const OnlineStatus& status, const QString &caption, Categories categories=0x00 , Options options=0x0);

	/**
	 * insert "setStatus" actions from the given account to the specified actionMenu.
	 *  (actions have that menu as parent QObject)
	 * they are connected to the Account::setOnlineStatus signal
	 *
	 * Items are stored by status height.
	 *
	 * @param account the account
	 * @param parent  the ActionMenu where action are inserted
	 */
	void createAccountStatusActions( Account *account , KActionMenu *parent);

	/**
	 * return the status of the @p protocol which is in the category @p category
	 *
	 * If no status has been registered in this category, return the one in the category which is the most similair
	 */
	OnlineStatus onlineStatus(Protocol *protocol, Categories category) const;

private:
	friend class OnlineStatus;
	QPixmap cacheLookupByObject( const OnlineStatus &statusFor, const QString& icon, int size, QColor color, bool idle = false);
	QPixmap cacheLookupByMimeSource( const QString &mimeSource );
	QString fingerprint( const OnlineStatus &statusFor, const QString& icon, int size, QColor color, bool idle = false);
	QPixmap* renderIcon( const OnlineStatus &statusFor, const QString& baseicon, int size, QColor color, bool idle = false) const;

signals:
	void iconsChanged();

private slots:
	void slotIconsChanged();

private:
	static OnlineStatusManager *s_self;
	OnlineStatusManager();
	class Private;
	Private *d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(OnlineStatusManager::Categories)
Q_DECLARE_OPERATORS_FOR_FLAGS(OnlineStatusManager::Options)

/**
 * @internal
 */
class OnlineStatusAction : public KAction
{
	Q_OBJECT
  public:
	OnlineStatusAction ( const OnlineStatus& status, const QString &text, const QIcon &pix, QObject *parent );
	~OnlineStatusAction();

  signals:
	void activated( const Kopete::OnlineStatus& status );
  private slots:
	void slotActivated();
  private:
	class Private;
	Private *d;
};

}  //END namespace Kopete

#endif

// vim: set noet ts=4 sts=4 sw=4:

