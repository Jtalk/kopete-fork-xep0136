/*
    yahoocontact.h - Yahoo Contact

    Copyright (c) 2003 by Matt Rogers <mattrogers@sbcglobal.net>
    Copyright (c) 2002 by Duncan Mac-Vicar Prett <duncan@kde.org>

    Portions based on code by Bruno Rodrigues <bruno.rodrigues@litux.org>

    Copyright (c) 2002 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef YAHOOCONTACT_H
#define YAHOOCONTACT_H

/* QT Includes */
#include <qtimer.h>

/* KDE Includes */

/* Kopete Includes */
#include "kopetecontact.h"
#include "kopetemetacontact.h"
#include "kopeteonlinestatus.h"

/* Local Includes */
#include "yahooprotocol.h"
#include "yahoostatus.h"

class YahooProtocol;
class KopeteMessageManager;

class YahooContact : public KopeteContact
{
	Q_OBJECT
public:
	YahooContact(KopeteAccount *account, const QString &userId, const QString &fullName, KopeteMetaContact *metaContact);

	virtual bool isOnline() const;
	virtual bool isReachable();
	virtual KActionCollection *customContextMenuActions();
	virtual KopeteMessageManager *manager( bool canCreate = false );
	virtual void serialize(QMap<QString, QString> &serializedData, QMap<QString, QString> &addressBookData);

	void setYahooStatus(YahooStatus::Status, const QString & = "", int = 0);

public slots:
	virtual void slotUserInfo();
	virtual void slotSendFile();

	/**
	 * Must be called after the contact list has been recieved
	 * or it doesn't work well!
	 */
	void syncToServer();

private slots:
	void slotMessageManagerDestroyed();
	void slotSendMessage(KopeteMessage &);
	void slotTyping(bool);

private:
	QString m_userId;
	YahooStatus m_status;
	KopeteMessageManager *m_manager;
};

#endif

// vim: set noet ts=4 sts=4 sw=4:

