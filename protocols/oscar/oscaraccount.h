/*
  oscaraccount.h  -  Oscar Account Class

    Copyright (c) 2002 by Tom Linsky <twl6@po.cwru.edu>
    Copyright (c) 2002 by Chris TenHarmsel <tenharmsel@staticmethod.net>
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

#ifndef OSCARACCOUNT_H
#define OSCARACCOUNT_H

#include <qdict.h>
#include <qstring.h>
#include <qwidget.h>

#include "kopeteaccount.h"
#include "oscarcontact.h"
#include "oscarsocket.h"

class KopeteContact;
class KopeteGroup;

class OscarContact;

class OscarSocket;
class AIMBuddy;
class AIMGroup;

class OscarAccountPrivate;

class OscarAccount : public KopeteAccount
{
	Q_OBJECT

public:
	OscarAccount(KopeteProtocol *parent, const QString &accountID, const char *name=0L, bool isICQ=false);
	virtual ~OscarAccount();

	/*
	 * Connects this account
	 */
	virtual void connect()=0;

	/*
	 * Disconnects this account
	 */
	void disconnect();

	/*
	 * Sets the account away
	 */
	virtual void setAway(bool away, const QString &awayMessage = QString::null)=0;

	/*
	 * Accessor method for our engine object
	 */
	OscarSocket* engine() const;

	/*
	 * Accessor method for the action menu
	 */
	virtual KActionMenu* actionMenu() = 0;

	/*
	 * Gets the next random new buddy num
	 */
	int randomNewBuddyNum();

	/*
	 * Gets the next random new group num
	 */
	int randomNewGroupNum();

	/*
	 * Sets the port we connect to
	 */
	void setServerPort(int port);

	/*
	 * Sets the server we connect to
	 */
	void setServerAddress(const QString &server);

	bool ignoreUnknownContacts() const;
	void setIgnoreUnknownContacts( bool b );

	void setAwayMessage(const QString&);
	const QString &awayMessage();

	/* Pure virtual to be implemented by ICQAccount and AIMAccount
	 * sets the users status and if connected should send a status update to the server
	 * in disconnected state it just updates the local status variable that
	 * gets used on connect
	 */
	virtual void setStatus(const unsigned long status,
		const QString &awayMessage = QString::null) =0;

	// -- MERGED METHODS FROM AIMBUDDYLIST -------------------------------
	// These methods need to be reviewed and better integrated with
	// OscarAccount, for now they are just a direct copy of the code
	enum OscarContactType { AllContacts, ServerSideContacts };

	/**
	 * Adds a group to the contact list and returns the new group
	 */
	AIMGroup *addGroup( int id, const QString &name = QString::null, OscarContactType type = AllContacts );

	/**
	 * Finds a group and returns it. Uses GID
	 */
	AIMGroup *findGroup( int id, OscarContactType type = AllContacts );

	/**
	 * Returns a list of all of the buddies
	 */
	QMap<QString, AIMBuddy * > buddies( OscarContactType type = AllContacts ) const;

	/**
	 * Adds a buddy to the buddy list
	 */
	void addBuddy( AIMBuddy *buddy );

	/**
	 * Adds a buddy to the deny list
	 */
	void addBuddyDeny( AIMBuddy *buddy );

	/**
	 * Finds a buddy in the buddy list. Returns 0L if none found.
	 */
	AIMBuddy *findBuddy( const QString &screenName );

	/**
	 * Finds a group by name
	 */
	AIMGroup *findGroup( const QString &name );

	/**
	 * Removes a buddy from the buddy list
	 */
	void removeBuddy( AIMBuddy *buddy );

private:
	/**
	 * Moves a buddy 'buddy' from group 'from' to group 'to'. Both groups
	 * need to have been created already
	 */
	void moveBuddy( AIMBuddy *buddy, AIMGroup *from, AIMGroup *to );

	/**
	 * Finds a buddy in the buddy list. Returns 0L if none found.
	 */
	AIMBuddy *findBuddy( int buddyId, int gid );

	/**
	 * Removes an entire group (including its child buddies!)
	 */
	void removeGroup( int id );

	/**
	 * Sets the name of a group. If this returns false, that means another group has this same name
	 */
	bool setGroupName( AIMGroup *group, const QString &name );

	/**
	 * Adds a buddy to the permit list
	 */
	void addBuddyPermit( AIMBuddy *buddy );

	/**
	 * Removes a buddy from the permit list
	 */
	void removeBuddyPermit( AIMBuddy *buddy );

	/**
	 * Removes a buddy from the deny list
	 */
	void removeBuddyDeny( AIMBuddy *buddy );

	/**
	 * Returns a list of all the buddies in the deny list
	 */
	QPtrList<AIMBuddy> denyBuddies() const;

	/**
	 * Returns a list of all the buddies in the permit list
	 */
	QPtrList<AIMBuddy> permitBuddies() const;
	// -- END MERGED METHODS FROM AIMBUDDYLIST ---------------------------

signals:
	// -- MERGED SIGNALS FROM AIMBUDDYLIST -------------------------------
	/**
	 * "Why?" you ask, do I have a signal that lets us know such a useless thing?
	 * Well, basically the AOL server can and often does (on certain accounts) send
	 * us contacts with a GID in which the group matching to that GID hasn't been
	 * sent down from the server yet. This gets around that by having OscarProtocol
	 * queue up contacts that have a GID which hasn't been created yet, then when
	 * that group is greated with the cooresponding GID to the contacts which
	 * have been queued up, we add those contacts. That's why we have this signal,
	 * to let OscarProtocol know to try to add certain contacts.
	 */
	void groupAdded( AIMGroup *group );
	// -- END MERGED SIGNALS FROM AIMBUDDYLIST ---------------------------

public slots:
	/*
	 * Slot for telling this account to go offline
	 */
	void slotGoOffline();

protected slots:
	/** Called when we get disconnected */
//	void slotDisconnected();

	/** Called when a group is added for this account */
	void slotGroupAdded(KopeteGroup* group);

	/** Called when the contact list renames a group */
	void slotKopeteGroupRenamed(KopeteGroup *group,
		const QString &oldName);

	/*
	 * Called when the contact list removes a group
	 */
	void slotKopeteGroupRemoved(KopeteGroup *group);

	/*
	 * Called when our status changes on the server
	 */
	void slotOurStatusChanged(const unsigned int newStatus);

	/*
	 * Called when we get a contact list from the server
	 * The parameter is a qmap with the contact names as keys
	 * the value is another map with keys "name", "group"
	 */
	void slotGotServerBuddyList();

	/*
	 * Called when we've received an IM
	 */
	void slotReceivedMessage(const QString &sender, OscarMessage &message, OscarSocket::OscarMessageType type);

	void slotReceivedAwayMessage(const QString &sender, const QString &message);

	/*
	 * Called when we get a request for a direct IM session with @sn
	 */
	void slotGotDirectIMRequest(QString sn);

	/*
	 * Called when there is no activity for a certain amount of time
	 */
	void slotIdleTimeout();

	/*
	 * Displays an error dialog with the given text
	 */
	void slotError(QString errmsg, int errorCode);

	/*
	 * Having received a new server side group, try
	 * to find queued buddies that are members of
	 * this group.
	 * @param group the newly added group.
	 */
	void slotReTryServerContacts();

	void slotLoggedIn();

	void slotDelayedListSync();

protected:
	/*
	 * Adds a contact to a meta contact
	 */
	virtual bool addContactToMetaContact(const QString &contactId,
		const QString &displayName, KopeteMetaContact *parentContact );

	/*
	 * Protocols using Oscar must implement this to perform the instantiation
	 * of their contact for Kopete.  Called by @ref addContactToMetaContact().
	 * @param contactId theprotocol unique id of the contact
	 * @param displayName the display name of the contact
	 * @param parentContact the parent metacontact
	 * @return whether the creation succeeded or not
	 */
	 virtual OscarContact *createNewContact( const QString &contactId, const QString &displayName,
		KopeteMetaContact *parentContact ) =0;

	/*
	 * Adds a contact to the internal list.
	 * This means that the contact is already
	 * on the server-side list
	 */
	virtual void addServerContact(AIMBuddy *buddy);

	/*
	 * Initializes the engine
	 */
	virtual void initEngine(bool);

	/*
	 * Adds a buddy that we queued to
	 * the contact list
	 */
	void addOldContact(AIMBuddy *bud, KopeteMetaContact *meta=0l);

	void syncLocalWithServerBuddyList();
	AIMGroup * findOrCreateGroup( const QString& localGroup );

private:
	OscarAccountPrivate *d;
};

#endif

// vim: set noet ts=4 sts=4 sw=4:

