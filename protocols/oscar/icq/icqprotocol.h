/*
  oscarprotocol.h  -  Oscar Protocol Plugin

  Copyright (c) 2003 by Olivier Goffart <ogoffart@tiscalinet.be>

  Kopete    (c) 2003 by the Kopete developers  <kopete-devel@kde.org>

  *************************************************************************
  *                                                                       *
  * This program is free software; you can redistribute it and/or modify  *
  * it under the terms of the GNU General Public License as published by  *
  * the Free Software Foundation; either version 2 of the License, or     *
  * (at your option) any later version.                                   *
  *                                                                       *
  *************************************************************************
  */

#ifndef ICQPROTOCOL_H
#define ICQPROTOCOL_H

#include <qwidget.h>
#include <qmap.h>

#include "kopeteprotocol.h"

class ICQProtocol : public KopeteProtocol
{
	Q_OBJECT

	public:
		ICQProtocol(QObject *parent, const char *name, const QStringList &args);

		/**
		* Return the active instance of the protocol
		* because it's a singleton, can only be used inside ICQ classes, not in oscar lib
		*/
		static ICQProtocol *protocol();

		bool canSendOffline() const;

		void deserializeContact( KopeteMetaContact *metaContact,
			const QMap<QString, QString> &serializedData,
			const QMap<QString, QString> &addressBookData );
		AddContactPage *createAddContactWidget(QWidget *parent, KopeteAccount *account);
		EditAccountWidget *createEditAccountWidget(KopeteAccount *account, QWidget *parent);
		KopeteAccount *createNewAccount(const QString &accountId);

		const KopeteOnlineStatus statusOnline;
		const KopeteOnlineStatus statusFFC;
		const KopeteOnlineStatus statusOffline;
		const KopeteOnlineStatus statusAway;
		const KopeteOnlineStatus statusDND;
		const KopeteOnlineStatus statusNA;
		const KopeteOnlineStatus statusOCC;
		const KopeteOnlineStatus statusConnecting;

		const QMap<int, QString> &genders() { return mGenders; }
		const QMap<int, QString> &countries() { return mCountries; }
		const QMap<int, QString> &languages() { return mLanguages; }

	private:
		void initGenders();
		void initLang();
		void initCountries();

	private:
		static ICQProtocol *protocolStatic_;
		QMap<int, QString> mGenders;
		QMap<int, QString> mCountries;
		QMap<int, QString> mLanguages;
};
#endif
// vim: set noet ts=4 sts=4 sw=4:
