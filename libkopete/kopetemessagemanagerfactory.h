/*
    kopetemessagemanagerfactory.h - Creates chat sessions

    Copyright   : (c) 2002 by Duncan Mac-Vicar Prett
    Email       : duncan@kde.org

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef KOPETEMESSAGEMANAGERFACTORY_H
#define KOPETEMESSAGEMANAGERFACTORY_H

#include <qobject.h>
#include <qptrlist.h>
#include <qintdict.h>
#include <qvaluelist.h>

#include "kopetemessagemanager.h"
#include "kopetemessage.h"

/**
 * @author Duncan Mac-Vicar Prett
 */
class KopeteMessageManager;
class KopeteMessage;
class KopeteContact;
class KopeteProtocol;
class KopeteEvent;
class KopeteView;

typedef QPtrList<KopeteContact>        KopeteContactPtrList;
typedef QValueList<KopeteMessage>      KopeteMessageList;
typedef QIntDict<KopeteMessageManager> KopeteMessageManagerDict;

/**
 * KopeteMessageManagerFactory is responsible for creating and tracking KopeteMessageManager
 * instances for each chat.
 */
class KopeteMessageManagerFactory : public QObject
{
	Q_OBJECT

public:
	static KopeteMessageManagerFactory* factory();

	~KopeteMessageManagerFactory();

	/**
	 * Create a new chat session. Provided is the initial list of contacts in
	 * the session. If a session with exactly these contacts already exists,
	 * it will be reused. Otherwise a new session is created.
	 * @param user The local user in the session.
	 * @param chatContacts The list of contacts taking part in the chat.
	 * @param protocol The protocol that the chat is using.
	 * @return A pointer to a new or reused KopeteMessageManager.
	 */
	KopeteMessageManager* create( const KopeteContact *user,
		KopeteContactPtrList chatContacts, KopeteProtocol *protocol);

	/**
	 * Find a chat session, if one exists, that matches the given list of contacts.
	 * @param user The local user in the session.
	 * @param chatContacts The list of contacts taking part in the chat.
	 * @param protocol The protocol that the chat is using.
	 * @return A pointer to an existing KopeteMessageManager, or 0L if none was found.
	 */
	KopeteMessageManager* findKopeteMessageManager( const KopeteContact *user,
		KopeteContactPtrList chatContacts, KopeteProtocol *protocol);

	/**
	 * Registers a KopeteMessageManager (or subclass thereof) with the KopeteMessageManagerFactory
	 */
	void addKopeteMessageManager(KopeteMessageManager *);

	/**
	 * Find the idth KopeteMessageManager that the factory knows of.
	 * @param id The number of the desired KopeteMessageManager.
	 * @returnA pointer to the KopeteMessageManager, or 0 if it was not found.
	 */
	KopeteMessageManager *findKopeteMessageManager( int id );

	/**
	 * Get a list of all open sessions.
	 */
	const KopeteMessageManagerDict& sessions();
	/**
	 * Get a list of all open sessions for a protocol.
	 */
	KopeteMessageManagerDict protocolSessions( KopeteProtocol *);

	/**
	 * Clean sessions for a protocol.
	 */
	void cleanSessions( KopeteProtocol *);

	void removeSession( KopeteMessageManager *session );

	/**
	 * create a new view for the manager.
	 * only the manager should call this fuction
	 */
	KopeteView *createView( KopeteMessageManager * , KopeteMessage::MessageType type );

signals:
	/**
	 * This signal is emitted whenever a message
	 * is about to be displayed by the KopeteChatWindow.
	 * Please remember that both messages sent and
	 * messages received will emit this signal!
	 * Plugins may connect to this signal to change
	 * the message contents before it's going to be displayed.
	 */
	void aboutToDisplay( KopeteMessage& message );

	/**
	 * Plugins may connect to this signal
	 * to manipulate the contents of the
	 * message that is being sent.
	 */
	void aboutToSend( KopeteMessage& message );

	/**
	 * Plugins may connect to this signal
	 * to manipulate the contents of the
	 * message that is being received.
	 */
	void aboutToReceive( KopeteMessage& message );

	/**
	 * A new view has been created
	 */
	void viewCreated( KopeteView * );

	/*
	 *	Request the creation of a new view
	 */
	void requestView(KopeteView*& , KopeteMessageManager * , KopeteMessage::MessageType type );

	/**
	 * the message is ready to be displayed
	 */
	void display( KopeteMessage& message, KopeteMessageManager * );

	/*
	 * obsolete temporary method used by the spellchecking plugin (ugly workaround)
	 */
	void getActiveView(KopeteView*& ); public: void activeView(KopeteView*&v) { emit getActiveView(v); }

private:
	KopeteMessageManagerFactory( QObject* parent = 0, const char* name = 0 );

	int mId;
	KopeteMessageManagerDict mSessionDict;

	static KopeteMessageManagerFactory *s_factory;

};

#endif

// vim: set noet ts=4 sts=4 sw=4:

