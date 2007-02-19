/*
    dispatcher.h - msn p2p protocol

    Copyright (c) 2003-2005 by Olivier Goffart        <ogoffart@kde.org>
    Copyright (c) 2005      by Gregg Edghill          <gregg.edghill@gmail.com>

    Kopete    (c) 2002-2007 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <QObject>
#include <QStringList>
#include <QMap>

#include "kopete_export.h"

#include "p2p.h"
#include "messageformatter.h"
#include "incomingtransfer.h"
#include "outgoingtransfer.h"


namespace Kopete { class Contact; }
class MSNSwitchBoardSocket;

/**
@author Kopete Developers
*/
namespace P2P{
	class IncomingTransfer;
	class OutgoingTransfer;

	class KOPETE_MSN_SHARED_EXPORT Dispatcher : public QObject
	{	Q_OBJECT
		public:
			Dispatcher(QObject *parent, const QString& contact, const QStringList &ip);
			~Dispatcher();

			void detach(TransferContext* transfer);
			QString localContact();
			void requestDisplayIcon(const QString& from, const QString& msnObject);
			void sendFile(const QString& path, qint64 fileSize, const QString& to);
			void sendImage(const QString& fileName, const QString& to);
			QString m_pictureUrl;
			QMap<QString, QString> objectList;

//#if MSN_WEBCAM
#if 0
			void startWebcam(const QString &myHandle, const QString &msgHandle, bool wantToReceive);
#endif


		public slots:
			void slotReadMessage(const QString &from, const QByteArray& stream);
			void messageAcknowledged(unsigned int correlationId, bool fullReceive);

		signals:
			void sendCommand(const QString &cmd, const QString &args = QString::null, bool addId = true, const QByteArray &body = QByteArray(), bool binary=false);
			void displayIconReceived(KTemporaryFile* file, const QString& msnObject);
			void incomingTransfer(const QString& from, const QString& fileName, qint64 fileSize);

		private:
				class CallbackChannel
				{
					public:
						CallbackChannel(MSNSwitchBoardSocket *switchboard);
						~CallbackChannel();

						quint32 send(const QByteArray& stream);

					private:
						MSNSwitchBoardSocket *m_switchboard;
				};

		public:
			CallbackChannel* callbackChannel();
			/**
			 * IP's of this compiter,  the first one is the one seen by the server.
			 */
			QStringList localIp() { return m_ip; }


		private:
			void dispatch(const P2P::Message& message);
			Kopete::Contact* getContactByAccountId(const QString& accountId);

			P2P::MessageFormatter m_messageFormatter;
			QMap<quint32, P2P::TransferContext*> m_sessions;
			QMap<quint32, P2P::Message> m_messageBuffer;
			QString m_contact;
			CallbackChannel *m_callbackChannel;
			QStringList m_ip;

			friend class P2P::TransferContext;
			friend class P2P::IncomingTransfer;
			friend class P2P::OutgoingTransfer;
	};
}

#endif
