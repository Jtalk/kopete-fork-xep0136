/*
    kircclient.h - IRC Client

    Copyright (c) 2002      by Nick Betcher <nbetcher@kde.org>
    Copyright (c) 2003      by Jason Keirstead <jason@keirstead.org>
    Copyright (c) 2003-2005 by Michel Hermier <michel.hermier@wanadoo.fr>

    Kopete    (c) 2002-2005 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef KIRCCLIENT_H
#define KIRCCLIENT_H

#include "kircsocket.h"

namespace KIrc
{

class ClientCommandHandler;
class Message;

/**
 * @author Nick Betcher <nbetcher@kde.org>
 * @author Michel Hermier <michel.hermier@wanadoo.fr>
 * @author Jason Keirstead <jason@keirstead.org>
 */
class Client
	: public KIrc::Socket
{
	Q_OBJECT

public:
	Client(QObject *parent = 0);
	~Client();

public: // READ properties accessors.

public slots: // WRITE properties accessors.

public:
	bool isDisconnected() const KDE_DEPRECATED;
	bool isConnected() const KDE_DEPRECATED;

	Entity::Ptr server();

	ClientCommandHandler *clientCommandHandler();

signals:
	/**
	 * Emit a received message.
	 * The received message could have been translated to your locale.
	 *
	 * @param type the message type.
	 * @param from the originator of the message.
	 * @param to is the list of entities that are related to this message.
	 * @param msg the message (usually translated).
	 *
	 * @note Most of the following numeric messages should be deprecated, and call this method instead.
	 *	 Most of the methods, using it, update KIrc::Entities.
	 *	 Lists based messages are sent via dedicated API, therefore they don't use this.
	 */
	// @param args the args to apply to this message.
	void receivedMessage(	MessageType type,
				const Entity::Ptr &from,
				const Entity::List &to,
				const QString &msg);

private slots:
	void authentify();

private:
	Q_DISABLE_COPY(Client)

	class Private;
	Private * const d;
};

}

#endif

