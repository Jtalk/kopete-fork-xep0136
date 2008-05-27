/*
    meanwhileaccount.h - meanwhile account

    Copyright (c) 2003-2004 by Sivaram Gottimukkala  <suppandi@gmail.com>
    Copyright (c) 2005      by Jeremy Kerr <jk@ozlabs.org>

    Kopete    (c) 2002-2004 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/
#ifndef MEANWHILEACCOUNT_H
#define MEANWHILEACCOUNT_H

#include <kopetepasswordedaccount.h>
#include "meanwhileprotocol.h"
#include "meanwhileplugin.h"

class MeanwhileSession;

/**
 * A class to handle a single Meanwhile Account.
 */
class MeanwhileAccount : public Kopete::PasswordedAccount
{
    Q_OBJECT
public:
    /**
     * Create a new Meanwhile account
     * @param protocol  The MeanwhileProtocol that this acccount is for
     * @param accountID The (meanwhile) account id of this account
     * @param name      The name of this account
     */
    MeanwhileAccount(MeanwhileProtocol *protocol, const QString &accountID,
            const char *name = 0L);

    ~MeanwhileAccount();

    virtual bool createContact(const QString &contactId,
                        Kopete::MetaContact *parentContact);

    virtual void connectWithPassword(const QString &password);

    virtual void disconnect();

    virtual void disconnect(Kopete::Account::DisconnectReason reason);

	virtual void fillActionMenu( KActionMenu *actionMenu );

    /** Get the server host name */
    QString getServerName();
    /** Get the server port */
    int     getServerPort();
    /** Set the server host name */
    void    setServerName(const QString &server);
    /** Set the server port */
    void    setServerPort(int port);
    /** Provide an information plugin for this account */
    void    setPlugin(MeanwhilePlugin *plugin);

    MeanwhilePlugin *infoPlugin;

    /**
     * Save the current contact list to the server
     */
    void syncContactsToServer();

    /**
     * Get a reference to the meanwhile session object, if one exists
     */
    MeanwhileSession *session();

    /**
     * Get the meanwhile id for this account
     * @return The meanwhile ID for the account
     */
    QString meanwhileId() const;

public slots:
    /**
     * Called by the session to notify that the state has changed
     */
    void slotSessionStateChange(Kopete::OnlineStatus status);

    /**
     * Called by the session when a notification message has been received
     */
    void slotServerNotification(const QString &mesg);

    /** Reimplemented from Kopete::Account */
    void setOnlineStatus(const Kopete::OnlineStatus& status,
            const QString &reason = QString());
    void setAway(bool away, const QString&reason = QString());

private:
    /** Current online status */
    Kopete::OnlineStatus status;

    /** A meanwhile session */
    MeanwhileSession *m_session;

    /* The user id for this account */
    QString m_meanwhileId;
};

#endif
