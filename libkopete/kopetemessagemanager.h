/*
    kopetemessagemanager.h - Manages all chats

    Copyright (c) 2002      by Duncan Mac-Vicar Prett <duncan@kde.org>
    Copyright (c) 2002      by Daniel Stone <dstone@kde.org>
    Copyright (c) 2002      by Martijn Klingens <klingens@kde.org>
    Copyright (c) 2002-2003 by Olivier Goffart <ogoffart@tiscalinet.be>
    Copyright (c) 2003      by Jason Keirstead   <jason@keirstead.org>

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

#ifndef __KOPETEMESSAGEMANAGER_H__
#define __KOPETEMESSAGEMANAGER_H__

#include <qptrlist.h>
#include <qvaluelist.h>
#include <qmap.h>

//FIXME: get ride of this include in header
#include "kopetemessage.h"

class KopeteContact;
class KopeteMessageManager;
class KopeteMessage;
class KopeteEvent;
class KopeteMessageLog;
class KopeteProtocol;
class KopeteView;
class KopeteOnlineStatus;
class KopeteAccount;

typedef QPtrList<KopeteContact>        KopeteContactPtrList;
typedef QValueList<KopeteMessage>        KopeteMessageList;
typedef QPtrList<KopeteMessageManager> KopeteMessageManagerList;

struct  KMMPrivate;

/**
 * @author Duncan Mac-Vicar Prett <duncan@kde.org>
 * @author Daniel Stone <dstone@kde.org>
 * @author Martijn Klingens <klingens@kde.org>
 * @author Olivier Goffart <ogoffart@tiscalinet.be>
 * @author Jason Keirstead   <jason@keirstead.org>
 *
 * The KopeteMessageManager (also called KMM for simplicity) manages a single chat.
 * It is an interface between the protocol, and the chatwindow.
 * Protocol can connect to @ref messageSent() signals to send the message, and can
 * appen received message with @ref messageReceived()
 *
 */
class KopeteMessageManager : public QObject
{
	friend class KopeteMessageManagerFactory;

	Q_OBJECT

public:
	/**
	 * Delete a chat manager instance
	 * You shouldn't delete the KMM yourself. it will be deleted when the chatwindow is closed
	 * see also @ref setCanBeDeleted()
	 */
	~KopeteMessageManager();

	/**
	 * @brief Get a list of all contacts in the session
	 */
	const KopeteContactPtrList& members() const;

	/**
	 * @brief Get the local user in the session
	 * @return the local user in the session, same as account()->myself()
	 */
	const KopeteContact* user() const;

	/**
	 * @brief Get the protocol being used.
	 * @return the protocol
	 */
	KopeteProtocol* protocol() const;

	/**
	 * @brief get the account
	 * @return the account
	 */
	KopeteAccount *account() const ;

	/**
	 * @brief the KMM unique id
	 * @return a unique identifier associated with this manager
	 */
	int mmId() const;

	/**
	 * @brief The caption of the chat
	 * Used for named chats
	 */
	const QString displayName();

	/**
	 * @brief change the displayname
	 * change the display name of the chat
	 */
	void setDisplayName( const QString & );

	/**
	 * @brief set a specified KOS for specified contact in this KMM
	 * Set a special icon for a contact in this kmm only.
	 * by default, all contact have their own status
	 */
	void setContactOnlineStatus( const KopeteContact*, const KopeteOnlineStatus & );

	/**
	 * @brief get the status of a contact.
	 * see @ref setContactOnlineStatus()
	 */
	const KopeteOnlineStatus contactOnlineStatus( const KopeteContact* ) const;

	/**
	 * @brief the manager's view
	 * Return the view for the supplied KopeteMessageManager.  If it already
	 * exists, it will be returned, otherwise, 0L will be returned or a new one
	 * if canCreate=true
	 * @param canCreate create a new one if it does not exist
	 * @param type Specifies the type of view if we have to create one.
	 */
	KopeteView* view(bool canCreate=false  , KopeteMessage::MessageType type = KopeteMessage::Undefined);


signals:
	/**
	 * @brief the KMM will be deleted
	 * Used by a KopeteMessageManager to signal that it is closing.
	 */
	 void closing(KopeteMessageManager *);

	/**
	 * a message will be soon shown in the chatwindow.
	 * See @ref KopeteMessageManagerFactory::aboutToShow() signal
	 */
	void messageAppended( KopeteMessage& msg, KopeteMessageManager * = 0L );
	/**
	 * a message will be soon received
	 * See @ref KopeteMessageManagerFactory::aboutToReceive() signal
	 */
	void messageReceived( KopeteMessage& msg, KopeteMessageManager * = 0L );
	/**
	 * @brief a message is going to be sent
	 * The message is going to be sent.
	 * protocols can connect to this signal to send the message ro the network.
	 * the protocol have also to call @ref appendMessage() and @ref messageSucceeded()
	 * See also @ref KopeteMessageManagerFactory::aboutToSend() signal
	 */
 	void messageSent( KopeteMessage& msg, KopeteMessageManager * = 0L );
	/**
	 * The last message has finaly sucessfully been sent
	 */
	void messageSuccess();

	/**
	 * @brief a new contact is now in the chat
	 */
	void contactAdded(const KopeteContact *, bool supress);
	/**
	 * @brief a contact is no longer in this chat
	 */
	void contactRemoved(const KopeteContact *, const QString& raison);

	/**
	 * @brief a contact in this chat has changed his displayname or his status
	 */
	void contactChanged();

	/**
	 * @brief The name of the chat is changed
	 */
	void displayNameChanged();

	/**
	 * @brief emiting a typing notification
	 * The user is typing a message, or just stopped to typing
	 * the protocol should connect to this signal to signal to others
	 * that the user is typing if the user support this
	 * @param isTyping say if the user is typing or not
	 */
	void typingMsg( bool isTyping );
	/**
	 * Signals that a remote user is typing a message.
	 * the chatwindow connects to this signal to update the statusbar
	 */
	void remoteTyping( const KopeteContact *, bool );

public slots:
	/**
	 * @brief Got a typing notification from a user
	 */
	void receivedTypingMsg( const KopeteContact *c , bool isTyping = true );

	/**
	 * Got a typing notification from a user. This is a convenience version
	 * of the above method that takes a QString contactId instead of a full
	 * KopeteContact
	 */
	void receivedTypingMsg( const QString &contactId, bool isTyping = true );

	/**
	 * Show a message to the chatwindow, or append it to the queue.
	 * This is the function protocols HAVE TO call for both incomming and outgoing messages
	 * if the message must be showed in the chatwindow
	 */
	void appendMessage( KopeteMessage &msg );

	/**
	 * Add a contact to the session
	 * @param c is the contact
	 * @param supress mean the there will be no automatic notifications in the chatwindow.
	 *  (note that i don't like the param supress at all. it is used in irc to show a different notification (with an info text)
	 *   a QStrin info yould be more interesting, but it is also used to don't show the notification when entering in a channel)
	 */
	void addContact( const KopeteContact *c, bool supress = false );

	/**
	 * Remove a contact from the session
	 * @param c is the contact
	 * @param raison is the optional raison message showed in the chatwindow
	 */
	void removeContact( const KopeteContact *c, const QString& raison=QString::null );

	/**
	 * Set if the KMM will be deleted when the chatwindow is deleted. It is usefull if you want
	 * to keep the KMM alive even if the chatwindow is closed.
	 * Warning: if you set it to false, please keep in mind that you have to reset it to true
	 *  later to delete it. In many case, you should never delete yourself the KMM, just call this
	 *  this method.
	 * default is true.
	 * If there are no chatwindow when setting it to true, the kmm will be deleted.
	 */
	void setCanBeDeleted ( bool ) ;

	/**
	 * Send a message to the user
	 */
	void sendMessage(KopeteMessage &message);
	/**
	 * Tell the KMM that the useris typing
	 * This method should be called only by a chatwindow. It emit @ref typingMsg signal
	 */
	void typing(bool t);

	/**
	 * Protocols have to call this method when the last message sent has been correctly sent
	 * This will emit @ref messageSuccess signal. and allow the email window to get closed
	 */
	void messageSucceeded();

private slots:
	void slotUpdateDisplayName();
	void slotViewDestroyed();
	void slotStatusChanged( KopeteContact *c, const KopeteOnlineStatus &status, const KopeteOnlineStatus &oldStatus );
	void slotContactDestroyed(KopeteContact*);

protected:
	/**
	 * Create a message manager. This constructor is private, because the
	 * static factory method createSession() creates the object. You may
	 * not create instances yourself directly!
	 */
	KopeteMessageManager( const KopeteContact *user, KopeteContactPtrList others,
		KopeteProtocol *protocol, int id = 0, const char *name = 0 );

	void setMMId( int );

private:
	KMMPrivate *d;

};

#endif

// vim: set noet ts=4 sts=4 sw=4:

