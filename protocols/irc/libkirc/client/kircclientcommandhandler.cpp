/*
    kircclientcommandhandler.cpp - IRC Client Command handler.

    Copyright (c) 2002      by Nick Betcher <nbetcher@kde.org>
    Copyright (c) 2003      by Jason Keirstead <jason@keirstead.org>
    Copyrigth (c) 2003-2007 by Michel Hermier <michel.hermier@gmail.com>

    Kopete    (c) 2002-2007 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "kircclientcommandhandler.moc"

#include "kircclientsocket.h"
#include "kircmessage.h"
#include "kirctransferhandler.h"

#include <kdebug.h>
#include <klocale.h>

#include <QDateTime>
#include <qfileinfo.h>
#include <qregexp.h>
/*
class KIrc::ClientCommandHandler::Private
{
public:
};
*/
using namespace KIrc;

ClientCommandHandler::ClientCommandHandler(QObject *parent)
	: CommandHandler(parent)
	, d(0)
//	, d(new KIrc::ClientCommandHandler::Private)
{
}

ClientCommandHandler::~ClientCommandHandler()
{
//	delete d;
}

void ClientCommandHandler::handleMessage(Message msg)
{
/*
	QStringList errors;

	if (msg.isNumeric())
	{
		if (d->FailedNickOnLogin)
		{
			// this is if we had a "Nickname in use" message when connecting and we set another nick.
			// This signal emits that the nick was accepted and we are now logged in
			//emit successfullyChangedNick(d->Nickname, d->PendingNick);
			d->Nickname = d->PendingNick;
			d->FailedNickOnLogin = false;
		}
		//mr = d->numericCommands[ msg.command().toInt() ];
		// we do this conversion because some dummy servers sends 1 instead of 001
		// numbers are stored as "1" instead of "001" to make convertion faster (no 0 pading).
		mr = d->commands[ QString::number(msg.command().toInt()) ];
	}
	else
		mr = d->commands[ msg.command() ];

	CommandHandler::handleMessage(msg);

	if (mr)
	{
		//errors = mr->operator()(msg);
	}
	else if (msg.isNumeric())
	{
		//kWarning(14120) << "Unknown IRC numeric reply for line:" << msg.raw();
		//emit incomingUnknown(msg.raw());
	}
	else
	{
		//kWarning(14120) << "Unknown IRC command for line:" << msg.raw();
		//emit internalError(UnknownCommand, msg);
	}

	if (!errors.isEmpty())
	{
		//kDebug(14120) << "Method error for line:" << msg.raw();
		//emit internalError(MethodFailed, msg);
	}
*/
}

/*
 * The ctcp commands seems to follow the same message behaviours has normal IRC command.
 * (Only missing the \n\r final characters)
 * So applying the same parsing rules to the messages.
 */
/*
bool Client::invokeCtcpCommandOfMessage(const QMap<QString, MessageRedirector *> &map, Message msg)
{
//	appendMessage( i18n("CTCP %1 REPLY: %2").arg(type).arg(messageReceived) );

	if(msg.hasCtcpMessage() && msg.ctcpMessage().isValid())
	{
		Message &ctcpMsg = msg.ctcpMessage();

		MessageRedirector *mr = map[ctcpMsg.command()];
		if (mr)
		{
			QStringList errors = mr->operator()(msg);

			if (errors.isEmpty())
				return true;

//			kDebug(14120) << "Method error for line:" << ctcpMsg.raw();
//			writeCtcpErrorMessage(msg.prefix(), msg.ctcpRaw(),
//				QString::fromLatin1("%1 internal error(s)").arg(errors.size()));
		}
		else
		{
//			kDebug(14120) << "Unknow IRC/CTCP command for line:" << ctcpMsg.raw();
//			writeCtcpErrorMessage(msg.prefix(), msg.ctcpRaw(), "Unknown CTCP command");

//			emit incomingUnknownCtcp(msg.ctcpRaw());
		}
	}
	else
	{
//		kDebug(14120) << "Message do not embed a CTCP message:" << msg.raw();
	}
	return false;
}
*/


