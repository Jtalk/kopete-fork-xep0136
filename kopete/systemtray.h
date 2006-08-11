/*
    systemtray.h  -  Kopete Tray Dock Icon

    Copyright (c) 2002      by Nick Betcher           <nbetcher@kde.org>
    Copyright (c) 2002-2003 by Martijn Klingens       <klingens@kde.org>
    Copyright (c) 2003      by Olivier Goffart        <ogoffart @ kde.org>

    Kopete    (c) 2002-2005 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef SYSTEMTRAY_H
#define SYSTEMTRAY_H

#include <QtGui/QIcon>
#include <QtGui/QMovie>

#include <ksystemtrayicon.h>

#include "kopetemessageevent.h"

class QTimer;
class QPoint;
class KMenu;
class KActionMenu;

/**
 * @author Nick Betcher <nbetcher@kde.org>
 *
 * NOTE: This class is for use ONLY in libkopete! It is not public API, and
 *       is NOT supposed to remain binary compatible in the future!
 */
class KopeteSystemTray : public KSystemTrayIcon
{
	Q_OBJECT

public:
	/**
	 * Retrieve the system tray instance
	 */
	static KopeteSystemTray* systemTray( QWidget* parent = 0);

	~KopeteSystemTray();

	// One method, multiple interfaces :-)
	void startBlink( const QString &icon );
	void startBlink( const QIcon &icon );
	void startBlink( QMovie *movie );
	void startBlink();

	void stopBlink();
	bool isBlinking() const { return mIsBlinking; };

Q_SIGNALS:
	void aboutToShowMenu(KMenu *am);

private Q_SLOTS:
	void onActivation(QSystemTrayIcon::ActivationReason reason);

	void slotBlink();
	void slotNewEvent(Kopete::MessageEvent*);
	void slotEventDone(Kopete::MessageEvent *);
	void slotConfigChanged();
	void slotReevaluateAccountStates();

private:
	KopeteSystemTray( QWidget* parent );
	QString squashMessage( const Kopete::Message& msgText );

	QTimer *mBlinkTimer;
	QIcon mKopeteIcon;
	QIcon mBlinkIcon;
	QMovie *mMovie;

	bool mIsBlinkIcon;
	bool mIsBlinking;

	static KopeteSystemTray* s_systemTray;

	QList<Kopete::MessageEvent*> mEventList;
};

#endif

// vim: set noet ts=4 sts=4 sw=4:

