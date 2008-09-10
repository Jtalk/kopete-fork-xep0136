/*
    wlmtransfermanager.h - Kopete Wlm Protocol

    Copyright (c) 2008      by Tiago Salem Herrmann <tiagosh@gmail.com>
    Kopete    (c) 2002-2003 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU General Public                   *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#ifndef WLMTRANSFERMANAGER_H
#define WLMTRANSFERMANAGER_H

#include <QObject>
#include "kopetetransfermanager.h"

#include "wlmaccount.h"
#include <msn/msn.h>

class WlmTransferManager:public QObject
{
  Q_OBJECT public:
    struct transferSessionData
    {
        QString from;
        QString to;
        bool incoming;
          Kopete::Transfer * ft;
    };

      WlmTransferManager (WlmAccount * account);
     ~WlmTransferManager ();
    WlmAccount *account ()
    {
        return m_account;
    }
    QMap < unsigned int,
      transferSessionData > *getTransferSessions ()
    {
        return &transferSessions;
    }
    void addTransferSession (unsigned int sessionID, Kopete::Transfer * ft,
                             QString from, QString to)
    {
        transferSessionData tsd;
        tsd.from = from;
        tsd.to = to;
        tsd.ft = ft;
        transferSessions[sessionID] = tsd;
    }

    public slots:void incomingFileTransfer (MSN::SwitchboardServerConnection *
                                            conn,
                                            const MSN::
                                            fileTransferInvite & ft);
    void gotFileTransferProgress (const unsigned int &sessionID,
                                  const unsigned long long &transferred);
    void gotFileTransferFailed (const unsigned int &sessionID);
    void gotFileTransferSucceeded (const unsigned int &sessionID);
    void slotAccepted (Kopete::Transfer * ft, const QString & filename);
    void slotRefused (const Kopete::FileTransferInfo & fti);
    void slotCanceled ();
    void fileTransferInviteResponse (const unsigned int &sessionID,
                                     const bool & response);
  private:
    QMap < unsigned int,
      transferSessionData > transferSessions;
    WlmAccount *m_account;
    unsigned int nextID;
};
#endif
