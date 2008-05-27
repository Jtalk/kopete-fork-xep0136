/*
    Kopete Utils.
    Copyright (c) 2005 Duncan Mac-Vicar Prett <duncan@kde.org>

      isHostReachable function code derived from KDE's HTTP kioslave
        Copyright (c) 2005 Waldo Bastian <bastian@kde.org>

    Kopete    (c) 2002-2003 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include <qmap.h>
//Added by qt3to4:
#include <QPixmap>
#include <QByteArray>

#include <kmessagebox.h>

#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>

#include "kopeteaccount.h"
#include "knotification.h"
#include "kopeteutils_private.h"
#include "kopeteutils.h"
#include "kopeteuiglobal.h"

static const QString notifyConnectionLost_DefaultMessage = i18n("You have been disconnected.");
static const QString notifyConnectionLost_DefaultCaption = i18n("Connection Lost.");
static const QString notifyConnectionLost_DefaultExplanation = i18n("Kopete lost the channel used to talk to the instant messaging system.\nThis can be because either your internet access went down, the service is experiencing problems, or the service disconnected you because you tried to connect with the same account from another location. Try connecting again later.");

static const QString notifyCannotConnect_DefaultMessage = i18n("Cannot connect with the instant messaging server or peers.");
static const QString notifyCannotConnect_DefaultCaption = i18n("Cannot connect.");
static const QString notifyCannotConnect_DefaultExplanation = i18n("This means Kopete cannot reach the instant messaging server or peers.\nThis can be because either your internet access is down or the server is experiencing problems. Try connecting again later.");

namespace Kopete
{
namespace Utils
{

void notify( QPixmap pic, const QString &eventid, const QString &caption, const QString &message, const QString explanation, const QString debugInfo)
{
	Q_UNUSED(caption);

	QStringList actions;
		if ( !explanation.isEmpty() )
			actions  << i18n( "More Information..." );
		kDebug( 14010 ) ;
		KNotification *n = new KNotification( eventid , 0l );
		n->setActions( actions );
		n->setText( message );
		n->setPixmap( pic );
		ErrorNotificationInfo info;
		info.explanation = explanation;
		info.debugInfo = debugInfo;

		NotifyHelper::self()->registerNotification(n, info);
		QObject::connect( n, SIGNAL(activated(unsigned int )) , NotifyHelper::self() , SLOT( slotEventActivated(unsigned int) ) );
		QObject::connect( n, SIGNAL(closed()) , NotifyHelper::self() , SLOT( slotEventClosed() ) );
		
		n->sendEvent();
}

void notifyConnectionLost( const Account *account, const QString caption, const QString message, const QString explanation, const QString debugInfo)
{
	if (!account)
		return;

	notify( account->accountIcon(32), QString::fromLatin1("connection_lost"), caption.isEmpty() ? notifyConnectionLost_DefaultCaption : caption, message.isEmpty() ? notifyConnectionLost_DefaultMessage : message, explanation.isEmpty() ? notifyConnectionLost_DefaultExplanation : explanation, debugInfo);
}

void notifyCannotConnect( const Account *account, const QString explanation, const QString debugInfo)
{
	Q_UNUSED(explanation);

	if (!account)
		return;

	notify( account->accountIcon(), QString::fromLatin1("cannot_connect"), notifyCannotConnect_DefaultCaption, notifyCannotConnect_DefaultMessage, notifyCannotConnect_DefaultExplanation, debugInfo);
}

} // end ns ErrorNotifier
} // end ns Kopete

