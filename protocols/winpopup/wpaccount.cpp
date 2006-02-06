/***************************************************************************
                          wpaccount.cpp  -  WP Plugin
                             -------------------
    begin                : Fri Apr 26 2002
    copyright            : (C) 2002 by Gav Wood
    email                : gav@indigoarchive.net

    Based on code from   : (C) 2002 by Duncan Mac-Vicar Prett
    email                : duncan@kde.org
 ***************************************************************************

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// QT Includes
#include <qregexp.h>
#include <QIcon>

// KDE Includes
#include <kdebug.h>
#include <kmenu.h>
#include <klocale.h>
#include <kconfig.h>

// Kopete Includes

// Local Includes
#include "wpaccount.h"

class KMenu;

WPAccount::WPAccount(WPProtocol *parent, const QString &accountID, const char *name) : Kopete::Account(parent, accountID, name)
{
//	kDebug(14170) <<  "WPAccount::WPAccount()" << endl;

	// setup internals
	QString theHostName = accountID;

	// we need this before initActions
	setMyself( new WPContact(this, theHostName, theHostName, 0) );

//	if (excludeConnect()) connect(Kopete::OnlineStatus::Online); // ??

	QObject::connect(this, SIGNAL(settingsChanged()), parent, SLOT(slotSettingsChanged()));
}

// Destructor
WPAccount::~WPAccount()
{
}

const QStringList WPAccount::getGroups()
{
	return static_cast<WPProtocol *>(protocol())->getGroups();
}

const QStringList WPAccount::getHosts(const QString &Group)
{
	return static_cast<WPProtocol *>(protocol())->getHosts(Group);
}

void WPAccount::slotSettingsChanged()
{
//	kDebug(14170) <<  "WPAccount::slotSettingsChanged()" << endl;
	emit(settingsChanged());
}

bool WPAccount::checkHost(const QString &Name)
{
//	kDebug() << "WPAccount::checkHost: " << Name << endl;
	if (Name.toUpper() == "LOCALHOST") {
		// Assume localhost is always there, but it will not appear in the samba output.
		// Should never happen as localhost is now forbidden as contact, just for safety. GF
		return true;
	} else {
		return static_cast<WPProtocol *>(protocol())->checkHost(Name);
	}
}

bool WPAccount::createContact(const QString &contactId, Kopete::MetaContact *parentContact )
{
//	kDebug(14170) << "[WPAccount::createContact] contactId: " << contactId << endl;

	if (!contacts()[contactId]) {
		WPContact *newContact = new WPContact(this, contactId, parentContact->displayName(), parentContact);
		return newContact != 0;
	} else {
		kDebug(14170) << "[WPAccount::addContact] Contact already exists" << endl;
	}

	return false;
}

void WPAccount::slotGotNewMessage(const QString &Body, const QDateTime &Arrival, const QString &From)
{
//	kDebug(14170) <<  "WPAccount::slotGotNewMessage(" << Body << ", " << Arrival.toString() << ", " << From << ")" << endl;

	// Ignore messages from own host or IPs.
	// IPs can not be matched to an account anyway.
	// This should happen rarely but they make kopete crash.
	// The reason for this seems to be in ChatSessionManager? GF
	QRegExp ip("\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}");

//	kDebug(14170) << "ip.search: " << From << " match: " << ip.search(From) << endl;

	if (From == accountId() || ip.search(From) > -1) {
		kDebug(14170) << "Ignoring message from own host/account or IP." << endl;
		return;
	}

	if (isConnected()) {
		if (!isAway()) {
			if(!contacts()[From]) {
				addContact(From, From, 0, Kopete::Account::DontChangeKABC);
			}
			static_cast<WPContact *>(contacts()[From])->slotNewMessage(Body, Arrival);
		}
		else {
			static_cast<WPProtocol *>(protocol())->sendMessage(theAwayMessage, From);
		}
	 } else {
		// What to do with offline received messages?
		kDebug(14170) << "WinPopup: That's strange - we got a message while offline! Ignoring." << endl;
	}
}

void WPAccount::connect(const Kopete::OnlineStatus &)
{
//	kDebug(14170) <<  "WPAccount::Connect()" << endl;
	myself()->setOnlineStatus(static_cast<WPProtocol *>(protocol())->WPOnline);
}

void WPAccount::disconnect()
{
//	kDebug(14170) <<  "WPAccount::Disconnect()" << endl;
	myself()->setOnlineStatus(static_cast<WPProtocol *>(protocol())->WPOffline);
}

/* I commented this code because deleting myself may have *dangerous* side effect, for example, for the status tracking.
void WPAccount::updateAccountId()
{
	delete myself();
	theInterface->setHostName(accountId());
	myself() = new WPContact(this, accountId(), accountId(), 0);
}*/

void WPAccount::setAway(bool status, const QString &awayMessage)
{
//	kDebug(14170) <<  "WPAccount::setAway()" << endl;

	theAwayMessage = awayMessage.isNull() ? i18n("I'm away at the moment.") : awayMessage;

//	if(!isConnected())
//		theInterface->goOnline();
	myself()->setOnlineStatus(status ? static_cast<WPProtocol *>(protocol())->WPAway : static_cast<WPProtocol *>(protocol())->WPOnline);
}

KActionMenu* WPAccount::actionMenu()
{
	kDebug(14170) <<  "WPAccount::actionMenu()" << endl;

	WPProtocol *theProtocol = static_cast<WPProtocol *>(protocol());
	KActionMenu *theActionMenu = new KActionMenu(accountId() , myself()->onlineStatus().iconFor(this), 0);
	theActionMenu->popupMenu()->addTitle( QIcon(myself()->icon()), i18n("WinPopup (%1)").arg(accountId()));

	if (theProtocol)
	{
		theActionMenu->insert(new KAction("Online", QIcon(theProtocol->WPOnline.iconFor(this)), 0,
			this, SLOT(connect()), 0, "actionGoAvailable"));
		theActionMenu->insert(new KAction("Away", QIcon(theProtocol->WPAway.iconFor(this)), 0,
			this, SLOT(goAway()), 0, "actionGoAway"));
		// One can not really go offline manually - appears online as long as samba server is running. GF
//		theActionMenu->insert(new KAction("Offline", QIcon(theProtocol->WPOffline.iconFor(this)), 0,
//			this, SLOT(disconnect()), theActionMenu, "actionGoOffline"));
	}

	return theActionMenu;
}

void WPAccount::slotSendMessage(const QString &Body, const QString &Destination)
{
	kDebug(14170) << "WPAccount::slotSendMessage(" << Body << ", " << Destination << ")" << endl;

	static_cast<WPProtocol *>(protocol())->sendMessage(Body, Destination);
}

void WPAccount::setOnlineStatus(const Kopete::OnlineStatus &status, const QString &reason)
{
	if (myself()->onlineStatus().status() == Kopete::OnlineStatus::Offline && status.status() == Kopete::OnlineStatus::Online)
		connect( status );
	else if (myself()->onlineStatus().status() != Kopete::OnlineStatus::Offline && status.status() == Kopete::OnlineStatus::Offline)
		disconnect();
	else if (myself()->onlineStatus().status() != Kopete::OnlineStatus::Offline && status.status() == Kopete::OnlineStatus::Away)
		setAway( true, reason );
}

#include "wpaccount.moc"

// vim: set noet ts=4 sts=4 sw=4:
// kate: tab-width 4; indent-width 4; replace-trailing-space-save on;
