/*
    msncontact.h - MSN Contact

    Copyright (c) 2002      by Duncan Mac-Vicar Prett <duncan@kde.org>
    Copyright (c) 2002      by Ryan Cumming           <bodnar42@phalynx.dhs.org>
    Copyright (c) 2002      by Martijn Klingens       <klingens@kde.org>
    Copyright (c) 2002-2003 by Olivier Goffart        <ogoffart@tiscalinet.be>

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

#ifndef MSNCONTACT_H
#define MSNCONTACT_H

#include "kopetecontact.h"

#include <kurl.h>

class QListView;
class QListViewItem;
class QPixmap;
class QTimer;

class MSNMessageManager;
class KAction;
class KActionCollection;
class KTempFile;

class KopeteProtocol;
class KopeteOnlineStatus;

class MSNContact : public KopeteContact
{
	Q_OBJECT

public:
	MSNContact( KopeteAccount *account, const QString &id, const QString &displayName, KopeteMetaContact *parent );
	~MSNContact();

	/**
	 * Indicate whether this contact is blocked
	 */
	bool isBlocked() const;
	void setBlocked( bool b );

	/**
	 * Indicate whether this contact is deleted
	 */
/*	bool isDeleted() const;
	void setDeleted( bool d );*/

	/**
	 * Indicate whether this contact is allowed
	 */
	bool isAllowed() const;
	void setAllowed( bool d );

	/**
	 * Indicate whether this contact is on the reversed list
	 */
	bool isReversed() const;
	void setReversed( bool d );

	/**
	 * set one phone number
	 */
	void setInfo(const QString &type, const QString &data);

	/**
	 * The groups in which the user is located on the server.
	 */
	const QMap<uint, KopeteGroup *> & serverGroups() const;

	virtual bool isReachable() { return true; };

	virtual KActionCollection *customContextMenuActions();

	/**
	 * update the server group map
	 */
	void contactRemovedFromGroup( unsigned int group );
	void contactAddedToGroup( uint groupNumber, KopeteGroup *group );

	virtual void serialize( QMap<QString, QString> &serializedData, QMap<QString, QString> &addressBookData );

	/**
	 * Rename contact on server
	 */
	virtual void rename( const QString &newName );

	/**
	 * Returns the MSN Message Manager associated with this contact
	 */
	virtual KopeteMessageManager *manager( bool canCreate = false );


	/**
	 * MSNAccount and MSNSwhitchBoardSocket need to change the displayName of contacts.
	 * Then, we do this function public
	 **/
	void setDisplayName(const QString &Dname);

	/**
	 * Because blocked contact have a small auto-modified status
	 */
	void setOnlineStatus(const KopeteOnlineStatus&);

	/**
	 * set the m_moving
	 * prevent change in the serverside contactlist when moving metacontacts
	 * used when syncing contactlist
	 */
	void setDontSync(bool b);

	QString phoneHome();
	QString phoneWork();
	QString phoneMobile();

	void setObject(const QString &obj);
	QString object() const { return m_obj; }
	KTempFile *displayPicture() const { return m_displayPicture; }

public slots:
	virtual void slotUserInfo();
	virtual void slotDeleteContact();
	virtual void sendFile(const KURL &sourceURL, const QString &altFileName, uint fileSize);

	/**
	 * Every time the kopete's contactlist is modified, we sync the serverlist with it
	 */
	virtual void syncGroups();

	void setDisplayPicture(KTempFile *f) ;

signals:
	void displayPictureChanged();

private slots:
	void slotBlockUser();
	void slotShowProfile();
	void slotSendMail();

	/**
	 * Workaround to make this checkboxe readonly
	 */
	void slotUserInfoDialogReversedToggled();

private:
	QMap<uint, KopeteGroup *> m_serverGroups;

	bool m_blocked;
	bool m_allowed;
//	bool m_deleted;
	bool m_reversed;

	bool m_moving;

	QString m_phoneHome;
	QString m_phoneWork;
	QString m_phoneMobile;
	bool m_phone_mob;

	QString m_obj; //the MSNObject
	KTempFile *m_displayPicture;
};

#endif

// vim: set noet ts=4 sts=4 sw=4:

