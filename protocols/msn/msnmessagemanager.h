/*
    msnmessagemanager.h - MSN Message Manager

    Copyright (c) 2002 by Olivier Goffart        <ogoffart@tiscalinet.be>

    Kopete    (c) 2002 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef MSNMESSAGEMANAGER_H
#define MSNMESSAGEMANAGER_H

#include "kopetemessagemanager.h"

class MSNSwitchBoardSocket;
class KActionCollection;
class MSNInvitation;
class MSNContact;
class KActionMenu;
class QLabel;


/**
 * @author Olivier Goffart
 */
class MSNMessageManager : public KopeteMessageManager
{
	Q_OBJECT

public:
	MSNMessageManager( KopeteProtocol *protocol, const KopeteContact *user, KopeteContactPtrList others, const char *name = 0 );
	~MSNMessageManager();

	void createChat( const QString &handle, const QString &address, const QString &auth, const QString &ID = QString::null );

	MSNSwitchBoardSocket *service() { return m_chatService; };

	void sendFile( const QString &fileLocation, const QString &fileName,
		long unsigned int fileSize );

	/**
	 * append an invitation in the invitation map, and send the first invitation message
	 */
	void initInvitation(MSNInvitation* invitation);

public slots:
	void slotCloseSession();
	void slotInviteOtherContact();

	void invitationDone( MSNInvitation*  );

	void slotRequestPicture();

private slots:
	void slotMessageSent( KopeteMessage &message, KopeteMessageManager *kmm );
	void slotMessageReceived( KopeteMessage &message );

	void slotUserJoined( const QString &handle, const QString &publicName, bool IRO );
	void slotUserLeft( const QString &handle, const QString &reason );
	void slotSwitchBoardClosed();
	void slotInviteContact( KopeteContact *contact );
	void slotAcknowledgement( unsigned int id, bool ack );
	void slotInvitation( const QString &handle, const QString &msg );

	void slotActionInviteAboutToShow();

	void slotDisplayPictureChanged();

	/**
	 * (debug)
	 */
	void slotDebugRawCommand();
private:

	MSNSwitchBoardSocket *m_chatService;
//	KopeteMessage *m_msgQueued;
	QString otherString;
	KActionMenu *m_actionInvite;
	QPtrList<KAction> m_inviteactions;

	//Messages sent before the ending of the connection are queued
	QValueList<KopeteMessage> m_messagesQueue;
	void sendMessageQueue();

	QMap<unsigned int, KopeteMessage> m_messagesSent;

	QMap<long unsigned int, MSNInvitation*> m_invitations;

	QTime m_awayMessageTime;

	QLabel *m_image;
	
	bool sent;


signals:
	/*
	 * This signal is relayed to the protocol and after, to plugins
	 */
	void invitation(MSNInvitation*& invitation,  const QString &bodyMSG , long unsigned int cookie , MSNMessageManager* msnMM , MSNContact* c );
};

#endif

// vim: set noet ts=4 sts=4 tw=4:

