/***************************************************************************
     wpprotocol.h  -  Base class for the Kopete WP protocol
                             -------------------
    begin                : Fri Apr 26 2002
    copyright            : (C) 2002 by Gav Wood
    email                : gav@kde.org

    Based on code from   : (C) 2002 by Duncan Mac-Vicar Prett
    email                : duncan@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __WPPROTOCOL_H
#define __WPPROTOCOL_H


// QT Includes
#include <qpixmap.h>
#include <qptrlist.h>

// KDE Includes

// Kopete Includes
#include "kopetemetacontact.h"
#include "kopeteprotocol.h"
#include "kopeteonlinestatus.h"

// Local Includes
#include "wppreferences.h"
#include "libwinpopup.h"
#include "wpcontact.h"
#include "wpaddcontact.h"
#include "wpaccount.h"

class KPopupMenu;
class KActionMenu;
class KAction;
class WPContact;

/**
 * This is a subclass of the KWinPopup class needed in order to use the virtual
 * methods and communicate nicely with Kopete.
 */
class KopeteWinPopup: public KWinPopup
{
	Q_OBJECT

public slots:
	void slotSendMessage(const QString &Body, const QString &Destination) { sendMessage(Body, Destination); }

signals:
	void newMessage(const QString &Body, const QDateTime &Arrival, const QString &From);

protected:
	virtual void receivedMessage(const QString &Body, const QDateTime &Arrival, const QString &From) { emit newMessage(Body, Arrival, From); }

public:
	KopeteWinPopup(const QString &SMBClientPath, const QString &InitialSearchHost, const QString &HostName, int HostCheckFrequency, int MessageCheckFrequency) :
		KWinPopup(SMBClientPath, InitialSearchHost, HostName, HostCheckFrequency, MessageCheckFrequency) {}
};

/**
 * The actual Protocol class used by Kopete.
 */
class WPProtocol : public KopeteProtocol
{
	Q_OBJECT

// KopeteProtocol overloading
public:
	WPProtocol(QObject *parent, QString name, QStringList);
	~WPProtocol();

	virtual const QString protocolIcon();
	virtual AddContactPage *createAddContactWidget(QWidget *parent) { return new WPAddContact(this, parent); }
	virtual EditAccountWidget *createEditAccountWidget(KopeteAccount *account, QWidget *parent);
	virtual KopeteAccount *createNewAccount(const QString &accountId);

// KopetePlugin overloading
public:
	virtual void deserializeContact(KopeteMetaContact *metaContact, const QMap<QString, QString> &serializedData, const QMap<QString, QString> &addressBookData);
		
// Stuff used internally & by colleague classes
public:
	static WPProtocol *protocol() { return sProtocol; }
	KopeteWinPopup *createInterface(const QString &theHostName);
	void destroyInterface(KopeteWinPopup *theInterface);

	const KopeteOnlineStatus WPOnline;
	const KopeteOnlineStatus WPAway;
	const KopeteOnlineStatus WPOffline;

public slots:
	void slotSettingsChanged(void);			// Callback when settings changed
	void installSamba();					// Modify smb.conf to use winpopup-send.sh script

private:
	WPPreferences *mPrefs;					// Preferences Object
	QPtrList<KopeteWinPopup> theInterfaces;	// List of all the interfaces created
	static WPProtocol *sProtocol;			// Singleton
};

#endif

