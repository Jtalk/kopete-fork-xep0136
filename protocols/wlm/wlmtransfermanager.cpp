/*
    wlmtransfermanager.cpp - Kopete Wlm Protocol

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

#include <QObject>
#include <kmessagebox.h>
#include <kcodecs.h>

#include <kdebug.h>

#include "wlmtransfermanager.h"
#include "wlmcontact.h"
#include "kopetecontact.h"
#include "kopeteuiglobal.h"


WlmTransferManager::WlmTransferManager (WlmAccount * account1):m_account (account1),
nextID (1)
{
    QObject::connect (&account ()->server ()->cb,
                      SIGNAL (incomingFileTransfer
                              (MSN::SwitchboardServerConnection *,
                               const MSN::fileTransferInvite &)), this,
                      SLOT (incomingFileTransfer
                            (MSN::SwitchboardServerConnection *,
                             const MSN::fileTransferInvite &)));

    QObject::connect (&account ()->server ()->cb,
                      SIGNAL (gotFileTransferProgress
                              (MSN::SwitchboardServerConnection *, 
                               const unsigned int &,
                               const unsigned long long &)), this,
                      SLOT (gotFileTransferProgress
                            (MSN::SwitchboardServerConnection *,
                             const unsigned int &,
                             const unsigned long long &)));

    QObject::connect (&account ()->server ()->cb,
                      SIGNAL (gotFileTransferFailed (MSN::SwitchboardServerConnection *,
                              const unsigned int &)),
                      this,
                      SLOT (gotFileTransferFailed (MSN::SwitchboardServerConnection *,
                              const unsigned int &)));

    QObject::connect (&account ()->server ()->cb,
                      SIGNAL (gotFileTransferSucceeded
                              (MSN::SwitchboardServerConnection *,
                               const unsigned int &)), this,
                      SLOT (gotFileTransferSucceeded (MSN::SwitchboardServerConnection *,
                              const unsigned int &)));

    QObject::connect (&account ()->server ()->cb,
                      SIGNAL (slotfileTransferInviteResponse
                              (MSN::SwitchboardServerConnection *,
                               const unsigned int &, const bool &)), this,
                      SLOT (fileTransferInviteResponse
                            (MSN::SwitchboardServerConnection * ,
                             const unsigned int &, const bool &)));

    connect (Kopete::TransferManager::transferManager (),
             SIGNAL (accepted (Kopete::Transfer *, const QString &)),
             this, SLOT (slotAccepted (Kopete::Transfer *, const QString &)));
    connect (Kopete::TransferManager::transferManager (),
             SIGNAL (refused (const Kopete::FileTransferInfo &)),
             this, SLOT (slotRefused (const Kopete::FileTransferInfo &)));

}

void
WlmTransferManager::fileTransferInviteResponse (MSN::SwitchboardServerConnection * conn,
                                                const unsigned int &sessionID,
                                                const bool & response)
{
    if (response)
    {
        transferSessionData tfd = transferSessions[sessionID];
        Kopete::ContactPtrList chatmembers;
        chatmembers.append (account ()->contacts ()[tfd.to]);
        WlmChatSession *_manager =
            dynamic_cast <
            WlmChatSession *
            >(Kopete::ChatSessionManager::self ()->
              findChatSession (account ()->myself (), chatmembers,
                               account ()->protocol ()));
        if (!_manager)
        {
            _manager =
                new WlmChatSession (account ()->protocol (),
                                    account ()->myself (), chatmembers);
        }

//              Kopete::Transfer* transf = Kopete::TransferManager::transferManager()->addTransfer( Kopete::Contact *contact, const QString& file, const unsigned long size, const QString &recipient , Kopete::FileTransferInfo::KopeteTransferDirection di)
    }
}

WlmTransferManager::~WlmTransferManager ()
{
}

void
WlmTransferManager::incomingFileTransfer (MSN::SwitchboardServerConnection * conn,
                                          const MSN::fileTransferInvite & ft)
{
    Kopete::Contact * contact =
        account ()->contacts ()[ft.userPassport.c_str ()];

    if(!contact)
	    return;

    if (ft.type == MSN::FILE_TRANSFER_WITH_PREVIEW
        || ft.type == MSN::FILE_TRANSFER_WITHOUT_PREVIEW)
    {
        QPixmap preview;
        if (ft.type == MSN::FILE_TRANSFER_WITH_PREVIEW)
        {
            preview.
                loadFromData (KCodecs::
                              base64Decode (QString (ft.preview.c_str ()).
                                            toAscii ()));
        }
        transferSessionData tsd;
        tsd.from = ft.userPassport.c_str ();
        tsd.to = account ()->myself ()->contactId ();
        transferSessions[ft.sessionId] = tsd;
        account ()->chatManager ()->createChat (conn);
        WlmChatSession *chat = account ()->chatManager ()->chatSessions[conn];
        if(chat)
            chat->setCanBeDeleted (false);

        int mTransferId =
            Kopete::TransferManager::transferManager ()->
            askIncomingTransfer (contact,
                                 ft.filename.c_str (),
                                 ft.filesize,
                                 "",
                                 QString::number (ft.sessionId),
                                 preview);
    }
}

void
WlmTransferManager::gotFileTransferProgress (MSN::SwitchboardServerConnection * conn,
                                             const unsigned int &sessionID,
                                             const unsigned long long
                                             &transferred)
{
    Kopete::Transfer * transfer = transferSessions[sessionID].ft;
    if (transfer)
        transfer->slotProcessed (transferred);
}

void
WlmTransferManager::slotAccepted (Kopete::Transfer * ft,
                                  const QString & filename)
{
    Kopete::ContactPtrList chatmembers;

    // grab contactId from the sender
    unsigned int sessionID = ft->info ().internalId ().toUInt ();
    QString from = transferSessions[sessionID].from;

    if (from.isEmpty ())
        return;

    // find an existent session, or create a new one
    chatmembers.append (account ()->contacts ()[from]);
    WlmChatSession *_manager =
        dynamic_cast <
        WlmChatSession *
        >(Kopete::ChatSessionManager::self ()->
          findChatSession (account ()->myself (), chatmembers,
                           account ()->protocol ()));

    if (!_manager)
    {
        _manager =
            new WlmChatSession (account ()->protocol (),
                                account ()->myself (), chatmembers);
    }

    MSN::SwitchboardServerConnection * conn = _manager->getChatService ();
    if (!conn)
        return;

    _manager->setCanBeDeleted (false);
    transferSessions[sessionID].ft = ft;

    connect (ft, SIGNAL (transferCanceled ()), this, SLOT (slotCanceled ()));

    conn->fileTransferResponse (sessionID, filename.toLatin1 ().data (),
                                true);
}

void
WlmTransferManager::slotRefused (const Kopete::FileTransferInfo & fti)
{
    Kopete::ContactPtrList chatmembers;
    chatmembers.append (fti.contact ());
    WlmChatSession *_manager =
        dynamic_cast <
        WlmChatSession *
        >(Kopete::ChatSessionManager::self ()->
          findChatSession (account ()->myself (), chatmembers,
                           account ()->protocol ()));

    if (!_manager)
        return;

    MSN::SwitchboardServerConnection * conn = _manager->getChatService ();
    if (!conn)
        return;

    conn->fileTransferResponse (fti.internalId ().toUInt (), "", false);
}

void
WlmTransferManager::gotFileTransferFailed (MSN::SwitchboardServerConnection * conn,
                                            const unsigned int &sessionID)
{
    Kopete::Transfer * transfer = transferSessions[sessionID].ft;
    if (transfer)
    {
        transfer->slotError (KIO::ERR_ABORTED, "Error");
        transferSessions.remove (sessionID);
    }
}

void
WlmTransferManager::gotFileTransferSucceeded (MSN::SwitchboardServerConnection * conn,
                                            const unsigned int &sessionID)
{
    Kopete::Transfer * transfer = transferSessions[sessionID].ft;
    if (transfer)
    {
        Kopete::ContactPtrList chatmembers;
        if (transfer->info ().direction () ==
            Kopete::FileTransferInfo::Incoming)
            chatmembers.append (account ()->
                                contacts ()[transferSessions[sessionID].
                                            from]);
        else
            chatmembers.append (account ()->
                                contacts ()[transferSessions[sessionID].to]);

        WlmChatSession *_manager =
            dynamic_cast <
            WlmChatSession *
            >(Kopete::ChatSessionManager::self ()->
              findChatSession (account ()->myself (), chatmembers,
                               account ()->protocol ()));
        if (_manager)
            _manager->raiseView ();
        transfer->slotComplete ();
        transferSessions.remove (sessionID);
    }
}

void
WlmTransferManager::slotCanceled ()
{
    kDebug (14210) << k_funcinfo;
    Kopete::Transfer * ft = dynamic_cast < Kopete::Transfer * >(sender ());
    if (!ft)
        return;
    unsigned int sessionID = 0;
    QMap < unsigned int,
      transferSessionData >::iterator i = transferSessions.begin ();
    for (; i != transferSessions.end (); ++i)
        if (i.value ().ft == ft)
            sessionID = i.key ();

    if (!sessionID)
        return;

    transferSessionData session = transferSessions[sessionID];

    Kopete::ContactPtrList chatmembers;
    if (ft->info ().direction () == Kopete::FileTransferInfo::Incoming)
        chatmembers.append (account ()->contacts ()[session.from]);
    else
        chatmembers.append (account ()->contacts ()[session.to]);

    WlmChatSession *_manager =
        dynamic_cast <
        WlmChatSession *
        >(Kopete::ChatSessionManager::self ()->
          findChatSession (account ()->myself (), chatmembers,
                           account ()->protocol ()));

    if (!_manager)
        return;
    _manager->raiseView ();

    MSN::SwitchboardServerConnection * conn = _manager->getChatService ();

    if (!conn)
        return;

    if (sessionID)
    {
        transferSessions.remove (sessionID);
        conn->cancelFileTransfer (sessionID);
    }
}

#include "wlmtransfermanager.moc"
