/*
    kopeteiface.h - Kopete DCOP Interface

    Copyright (c) 2002 by Hendrik vom Lehn       <hennevl@hennevl.de>

    Kopete    (c) 2002-2003      by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef KopeteIface_h
#define KopeteIface_h

#include <dcopobject.h>
#include <qstringlist.h>
#include <kurl.h>

/**
 * DCOP interface for kopete
 */
class KopeteIface : virtual public DCOPObject
{
	K_DCOP

public:
	KopeteIface();

k_dcop:
	QStringList contacts();
	QStringList reachableContacts();
	QStringList onlineContacts();
	QStringList fileTransferContacts();
	QStringList contactFileProtocols(const QString &displayName);
	/*void sendFile(const QString &displayName, const KURL &sourceURL,
		const QString &altFileName = QString::null, uint fileSize = 0);*/

	// FIXME: Do we *need* this one? Sounds error prone to me, because
	// nicknames can contain parentheses too.
	// Better add a contactStatus( const QString id ) I'd say - Martijn
	QStringList contactsStatus();

	/**
	 * Open a chat to a contact, and optionally set some initial text
	 */
	 void messageContact( const QString &displayName, const QString &messageText = QString::null );

	/**
	 * Adds a contact with the specified params.
	 *
	 * @param protocolName The name of the protocol this contact is for ("ICQ", etc)
	 * @param accountId The account ID to add the contact to
	 * @param contactId The unique ID for this protocol
	 * @param displayName The displayName of the contact (may equal userId for some protocols
	 * @param groupName The name of the group to add the contact to
	 * @return Weather or not the contact was added successfully
	 */
	bool addContact( const QString &protocolName, const QString &accountId, const QString &contactId,
		const QString &displayName, const QString &groupName = QString::null );

	QStringList accounts();

	void connect(const QString &protocolName, const QString &accountId);
	void disconnect(const QString &protocolName, const QString &accountId);

	bool loadPlugin( const QString& name );
	bool unloadPlugin( const QString& name );
};

#endif



/*
 * Local variables:
 * c-indentation-style: k&r
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 */
// vim: set noet ts=4 sts=4 sw=4:

