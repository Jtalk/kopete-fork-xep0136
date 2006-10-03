/*
    dispatcher.cpp - msn p2p protocol

    Copyright (c) 2003-2005 by Olivier Goffart        <ogoffart@ kde.org>
    Copyright (c) 2005      by Gregg Edghill          <gregg.edghill@gmail.com>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "dispatcher.h"
#include "incomingtransfer.h"
#include "outgoingtransfer.h"

//#if MSN_WEBCAM
#if 0
#include "webcam.h"
#include <QByteArray>
#endif

using P2P::Dispatcher;
using P2P::Message;
using P2P::TransferContext;
using P2P::IncomingTransfer;
using P2P::OutgoingTransfer;

#include "msnswitchboardsocket.h"

// Kde includes
#include <kdebug.h>
#include <kcodecs.h>
#include <kstandarddirs.h>
#include <ktemporaryfile.h>

// Qt includes
#include <qdatastream.h>
#include <qfile.h>
#include <qregexp.h>
#include <qtextcodec.h>
#include <qtextstream.h>

// Kopete includes
#include <kopetechatsession.h>  // Just for getting the contact
#include <kopeteaccount.h>
#include <kopetetransfermanager.h>

#include <stdlib.h>

Dispatcher::Dispatcher(QObject *parent, const QString& contact, const QStringList &ip)
	: QObject(parent) ,  m_contact(contact) , m_callbackChannel(0l) , m_ip(ip)
{}

Dispatcher::~Dispatcher()
{
	kDebug(14140) << k_funcinfo << endl;

	if(m_callbackChannel)
	{
		delete m_callbackChannel;
		m_callbackChannel = 0l;
	}
}

void Dispatcher::detach(TransferContext* transfer)
{
	m_sessions.remove(transfer->m_sessionId);
	transfer->deleteLater();
}

QString Dispatcher::localContact()
{
	return m_contact;
}

void Dispatcher::requestDisplayIcon(const QString& from, const QString& msnObject)
{
	quint32 sessionId = rand()%0xFFFFFF00 + 4;
	TransferContext* current =
		new IncomingTransfer(from, this, sessionId);

	current->m_branch = P2P::Uid::createUid();
	current->m_callId = P2P::Uid::createUid();
	current->setType(P2P::UserDisplayIcon);
	current->m_object = msnObject;
	// Add the transfer to the list.
	m_sessions.insert(sessionId, current);

	kDebug(14140) << k_funcinfo << "Requesting, " << msnObject << endl;

	QString context = QString::fromUtf8(KCodecs::base64Encode(msnObject.toUtf8()));
	// NOTE remove the \0 character automatically
	// appended to a QCString.
	context.replace("=", QString::null);
	QString content =
			"EUF-GUID: {A4268EEC-FEC5-49E5-95C3-F126696BDBF6}\r\n"
			"SessionID: " + QString::number(sessionId) + "\r\n"
			"AppID: 1\r\n"
			"Context: " + context + "\r\n"
			"\r\n";
	// Send the sending client an invitation message.
	current->sendMessage(INVITE, content);
}

void Dispatcher::sendFile(const QString& path, qint64 fileSize, const QString& to)
{
	// Create a new transfer context that will handle
	// the file transfer.
	quint32 sessionId = rand()%0xFFFFFF00 + 4;
	TransferContext *current =
		new OutgoingTransfer(to, this, sessionId);
	current->m_branch = P2P::Uid::createUid();
	current->m_callId = P2P::Uid::createUid();
	current->setType(P2P::File);
	// Add the transfer to the list.
	m_sessions.insert(sessionId, current);

	// Set the transfer context file.
	current->m_file = new QFile(path);
	// Create the file context data.
	QString context;

	QByteArray header(638, '\0');
	QDataStream writer( &header,QIODevice::WriteOnly);
	writer.setVersion(QDataStream::Qt_3_1);
	writer.setByteOrder(QDataStream::LittleEndian);

	// Write the header length to the stream.
	writer << (qint32)638;
	// Write client version to the stream.
	writer << (qint32)3;
	// Write the file size to the stream.
	writer << fileSize;
	// Write the file transfer flag to the stream.
	// TODO support file preview. For now disable file preview.
	writer << (qint32)1;
	// Write the file name in utf-16 to the stream.
	QTextStream ts(header, QIODevice::WriteOnly);
	ts.setCodec(QTextCodec::codecForName("UTF-16"));
	ts.device()->seek(20);
	ts << path.section('/', -1);
	// NOTE Background Sharing base64 [540..569]
	// TODO add support for background sharing.
	// Write file exchange type to the stream.
	// NOTE File - 0xFFFFFFFF
	// NOTE Background Sharing - 0xFFFFFFFE
	writer.device()->seek(570);
	writer << (quint32)0xFFFFFFFF;

	// Encode the file context header to base64 encoding.
	context = QString::fromUtf8(KCodecs::base64Encode(header));

	// Send an INVITE message to the recipient.
	QString content = "EUF-GUID: {5D3E02AB-6190-11D3-BBBB-00C04F795683}\r\n"
			"SessionID: " + QString::number(sessionId) + "\r\n"
			"AppID: 2\r\n"
			"Context: "   + context + "\r\n"
			"\r\n";
	current->sendMessage(INVITE, content);
}

void Dispatcher::sendImage(const QString& /*fileName*/, const QString& /*to*/)
{
// 	TODO kDebug(14140) << k_funcinfo << endl;
// 	QFile imageFile(fileName);
// 	if(!imageFile.open(QIODevice::ReadOnly))
// 	{
// 		kDebug(14140) << k_funcinfo << "Error opening image file."
// 			<< endl;
// 		return;
// 	}
//
// 	OutgoingTransfer *outbound =
// 		new OutgoingTransfer(to, this, 64);
//
// 	outbound->sendImage(imageFile.readAll());
}

//#if MSN_WEBCAM
#if 0
void Dispatcher::startWebcam(const QString &/*myHandle*/, const QString &msgHandle, bool wantToReceive)
{
	quint32 sessionId = rand()%0xFFFFFF00 + 4;
	Webcam::Who who= wantToReceive ? Webcam::wViewer : Webcam::wProducer;
	TransferContext* current =
			new Webcam(who, msgHandle, this, sessionId);

	current->m_branch = P2P::Uid::createUid();
	current->m_callId = P2P::Uid::createUid();
	current->setType(P2P::WebcamType);
	// Add the transfer to the list.
	m_sessions.insert(sessionId, current);

	//  {4BD96FC0-AB17-4425-A14A-439185962DC8}  <- i want to show you my webcam
	//  {1C9AA97E-9C05-4583-A3BD-908A196F1E92}  <- i want to see your webcam
	QString GUID= (who==Webcam::wProducer) ? "4BD96FC0-AB17-4425-A14A-439185962DC8" : "1C9AA97E-9C05-4583-A3BD-908A196F1E92"  ;

	QString content="EUF-GUID: {"+GUID+"}\r\n"
			"SessionID: "+ QString::number(sessionId)+"\r\n"
			"AppID: 4\r\n"
			"Context: ewBCADgAQgBFADcAMABEAEUALQBFADIAQwBBAC0ANAA0ADAAMAAtAEEARQAwADMALQA4ADgARgBGADgANQBCADkARgA0AEUAOAB9AA==\r\n\r\n";

        // context is the base64 of the utf16 of {B8BE70DE-E2CA-4400-AE03-88FF85B9F4E8}

	current->sendMessage( INVITE , content );
}
#endif



void Dispatcher::slotReadMessage(const QString &from, const QByteArray& stream)
{
	P2P::Message receivedMessage =
		m_messageFormatter.readMessage(stream);

	receivedMessage.source = from;

	if(receivedMessage.contentType == "application/x-msnmsgrp2p")
	{
		if((receivedMessage.header.dataSize == 0)/* && ((receivedMessage.header.flag & 0x02) == 0x02)*/)
		{
			TransferContext *current = 0l;
			QMap<quint32, TransferContext*>::Iterator it = m_sessions.begin();
			for(; it != m_sessions.end(); it++)
			{
				if(receivedMessage.header.ackSessionIdentifier == it.value()->m_identifier){
					current = it.value();
					break;
				}
			}

			if(current){
    			// Inform the transfer object of the acknowledge.
    			current->m_ackSessionIdentifier = receivedMessage.header.identifier;
    			current->m_ackUniqueIdentifier = receivedMessage.header.ackSessionIdentifier;
				current->acknowledged();
			}
			else
			{
				kDebug(14140) << k_funcinfo
					<< "no transfer context with identifier, "
					<< receivedMessage.header.ackSessionIdentifier
					<< endl;
			}
			return;
		}

		if(m_messageBuffer.contains(receivedMessage.header.identifier))
		{
			kDebug(14140) << k_funcinfo
				<< QString("retrieving buffered messsage, %1").arg(receivedMessage.header.identifier)
				<< endl;

			// The message was split, try to reconstruct the message
			// with this received piece.
			Message bufferedMessage = m_messageBuffer[receivedMessage.header.identifier];
			// Remove the buffered message.
			m_messageBuffer.remove(receivedMessage.header.identifier);

			bufferedMessage.body.resize(bufferedMessage.body.size() + receivedMessage.header.dataSize);
			for(quint32 i=0; i < receivedMessage.header.dataSize; i++){
				// Add the remaining message data to the buffered message.
				int offset = receivedMessage.header.dataOffset + i;
				bufferedMessage.body[offset] = receivedMessage.body[i];
			}
			bufferedMessage.header.dataSize += receivedMessage.header.dataSize;
			bufferedMessage.header.dataOffset = 0;

			receivedMessage = bufferedMessage;
		}

		// Dispatch the received message.
		dispatch(receivedMessage);
	}
}

void Dispatcher::dispatch(const P2P::Message& message)

{
	TransferContext *messageHandler = 0l;

	if(message.header.sessionId > 0)
	{
		if(m_sessions.contains(message.header.sessionId)){
			messageHandler = m_sessions[message.header.sessionId];
		}
	}
	else
	{
		QString body =
			QByteArray(message.body.data(), message.header.dataSize);
		QRegExp regex("SessionID: ([0-9]*)\r\n");
		if(regex.indexIn(body) > 0)
		{
			quint32 sessionId = regex.cap(1).toUInt();
			if(m_sessions.contains(sessionId)){
				// Retrieve the message handler associated with the specified session Id.
				messageHandler = m_sessions[sessionId];
			}
		}
		else
		{
			// Otherwise, try to retrieve the message handler
			// based on the acknowlegded unique identifier.
			if(m_sessions.contains(message.header.ackUniqueIdentifier)){
				messageHandler =
					m_sessions[message.header.ackUniqueIdentifier];
			}

			if(!messageHandler)
			{
				// If the message handler still has not been found,
				// try to retrieve the handler based on the call id.
				regex = QRegExp("Call-ID: \\{([0-9A-F\\-]*)\\}\r\n");
				regex.indexIn(body);
				QString callId = regex.cap(1);

				TransferContext *current = 0l;
				QMap<quint32, TransferContext*>::Iterator it = m_sessions.begin();
				for(; it != m_sessions.end(); it++)
				{
					current = it.value();
					if(current->m_callId == callId){
						messageHandler = current;
						break;
					}
				}
			}
		}
	}

	if(messageHandler){
		// Process the received message using the
		// retrieved registered handler.
		messageHandler->m_ackSessionIdentifier = message.header.identifier;
    	messageHandler->m_ackUniqueIdentifier = message.header.ackSessionIdentifier;
		messageHandler->processMessage(message);
	}
	else
	{
		// There are no objects registered, with the retrieved session Id,
		// to handle the received message; default to this dispatcher.

		if(message.header.totalDataSize > message.header.dataOffset + message.header.dataSize)
		{
			// The entire message has not been received;
			// buffer the recevied portion of the original message.
			kDebug(14140) << k_funcinfo
				<< QString("Buffering messsage, %1").arg(message.header.identifier)
				<< endl;
			m_messageBuffer.insert(message.header.identifier, message);
			return;
		}

		QString body =
			QByteArray(message.body.data(), message.header.dataSize);
		kDebug(14140) << k_funcinfo << "received, " << body << endl;

		if(body.startsWith("INVITE"))
		{
			// Retrieve the branch, call id, and session id.
			// These fields will be used later on in the p2p
			// transaction.
			QRegExp regex(";branch=\\{([0-9A-F\\-]*)\\}\r\n");
			regex.indexIn(body);
			QString branch = regex.cap(1);
			regex = QRegExp("Call-ID: \\{([0-9A-F\\-]*)\\}\r\n");
			regex.indexIn(body);
			QString callId = regex.cap(1);
			regex = QRegExp("SessionID: ([0-9]*)\r\n");
			regex.indexIn(body);
			QString sessionId = regex.cap(1);
			// Retrieve the contact that requested the session.
			regex = QRegExp("From: <msnmsgr:([^>]*)>");
			regex.indexIn(body);
			QString from = regex.cap(1);
			// Retrieve the application identifier which
			// is used to determine what type of session
			// is being requested.
			regex = QRegExp("AppID: ([0-9]*)\r\n");
			regex.indexIn(body);
			quint32 applicationId = regex.cap(1).toUInt();

			if(applicationId == 1  || applicationId == 12)
			{                         //the AppID is 12 since Messenger 7.5
				// A contact has requested a session to download
				// a display icon (User Display Icon or CustomEmotion).

				regex = QRegExp("Context: ([0-9a-zA-Z+/=]*)");
				regex.indexIn(body);
				QByteArray msnobj;

				// Decode the msn object from base64 encoding.
				KCodecs::base64Decode(regex.cap(1).toUtf8() , msnobj);
				kDebug(14140) << k_funcinfo << "Contact requested, "
					<< msnobj << endl;

				// Create a new transfer context that will handle
				// the user display icon transfer.
				TransferContext *current =
					new OutgoingTransfer(from, this, sessionId.toUInt());
				current->m_branch = branch;
				current->m_callId = callId;
				current->setType(P2P::UserDisplayIcon);
				// Add the transfer to the list.
				m_sessions.insert(sessionId.toUInt(), current);

				// Determine the display icon being requested.
				QString fileName = objectList.contains(msnobj)
				    ? objectList[msnobj]
					: m_pictureUrl;
				QFile *source = new QFile(fileName);
				// Try to open the source file for reading.
				// If an error occurs, send an internal
				// error message to the recipient.
				if(!source->open(QIODevice::ReadOnly))
				{
					current->error();
					return;
				}

				current->m_file = source;
				// Acknowledge the session request.
				current->acknowledge(message);

				current->m_ackSessionIdentifier = message.header.identifier;
    			current->m_ackUniqueIdentifier = message.header.ackSessionIdentifier;
				// Send a 200 OK message to the recipient.
				QString content = QString("SessionID: %1\r\n\r\n").arg(sessionId);
				current->sendMessage(OK, content);
			}
			else if(applicationId == 2)
			{
				// A contact has requested a session to
				// send a file.

				kDebug(14140) << k_funcinfo << "File transfer invitation." << endl;

				// Create a new transfer context that will handle
				// the file transfer.
				TransferContext *transfer =
					new IncomingTransfer(from, this, sessionId.toUInt());
				transfer->m_branch = branch;
				transfer->m_callId = callId;
				transfer->setType(P2P::File);
				// Add the transfer to the list.
				m_sessions.insert(sessionId.toUInt(), transfer);

				regex = QRegExp("Context: ([0-9a-zA-Z+/=]*)");
				regex.indexIn(body);
				QByteArray context;

				// Decode the file context from base64 encoding.
				KCodecs::base64Decode(regex.cap(1).toUtf8(), context);
				QDataStream reader( &context,QIODevice::ReadOnly);
				reader.setVersion(QDataStream::Qt_3_1);
				reader.setByteOrder(QDataStream::LittleEndian);
				//Retrieve the file info from the context field.
				// File Size [8..15] Int64
				reader.device()->seek(8);
				qint64 fileSize;
				reader >> fileSize;
				// Flag [15..18] Int32
				// 0x00 File transfer with preview data.
				// 0x01 File transfer without preview data.
				// 0x02 Background sharing.
				qint32 flag;
				reader >> flag;
				kDebug(14140) << flag << endl;
				// FileName UTF16 (Unicode) [19..539]
				QByteArray bytes;
				bytes.reserve(520);
				reader.readRawData(bytes.data(), bytes.size());
				QTextStream ts(bytes, QIODevice::ReadOnly);
				ts.setCodec(QTextCodec::codecForName("UTF-16"));
				QString fileName;
				fileName = ts.readLine().toUtf8();

				emit incomingTransfer(from, fileName, fileSize);

   				kDebug(14140) <<
   					QString("%1, %2 bytes.").arg(fileName, QString::number(fileSize))
   					<< endl
   					<< endl;

				// Get the contact that is sending the file.
				Kopete::Contact *contact = getContactByAccountId(from);

				if(contact)
				{
					// Acknowledge the file invitation message.
					transfer->acknowledge(message);

					transfer->m_ackSessionIdentifier = message.header.identifier;
    				transfer->m_ackUniqueIdentifier = message.header.ackSessionIdentifier;

					QObject::connect(Kopete::TransferManager::transferManager(), SIGNAL(accepted(Kopete::Transfer*, const QString&)), transfer, SLOT(slotTransferAccepted(Kopete::Transfer*, const QString&)));
					QObject::connect(Kopete::TransferManager::transferManager(), SIGNAL(refused(const Kopete::FileTransferInfo&)), transfer, SLOT(slotTransferRefused(const Kopete::FileTransferInfo&)));

					// Show the file transfer accept/decline dialog.
					Kopete::TransferManager::transferManager()->askIncomingTransfer(contact, fileName, fileSize, QString::null, sessionId);
				}
				else
				{
					kWarning(14140) << fileName << " from " << from
						<< " has failed; could not retrieve contact from contact list."
						<< endl;
					transfer->m_ackSessionIdentifier = message.header.identifier;
    				transfer->m_ackUniqueIdentifier = message.header.ackSessionIdentifier;
					transfer->sendMessage(ERROR);
				}
			}
			else if(applicationId == 4)
			{
//#if MSN_WEBCAM
#if 0
				regex = QRegExp("EUF-GUID: \\{([0-9a-zA-Z\\-]*)\\}");
				regex.search(body);
				QString GUID=regex.cap(1);

				kDebug(14140) << k_funcinfo << "webcam " << GUID << endl;

				Webcam::Who who;
				if(GUID=="4BD96FC0-AB17-4425-A14A-439185962DC8")
				{  //that mean "I want to send MY webcam"
					who=Webcam::wViewer;
				}
				else if(GUID=="1C9AA97E-9C05-4583-A3BD-908A196F1E92")
				{ //that mean "I want YOU to send YOUR webcam"
					who=Webcam::wProducer;
				}
				else
				{ //unknown GUID
					//current->error();
					kWarning(14140) << k_funcinfo << "Unknown GUID " << GUID << endl;
					return;
				}

				TransferContext *current = new P2P::Webcam(who, from, this, sessionId.toUInt());
				current->m_branch = branch;
				current->m_callId = callId;

					// Add the transfer to the list.
				m_sessions.insert(sessionId.toUInt(), current);
					// Acknowledge the session request.
				current->acknowledge(message);
				QTimer::singleShot(0,current, SLOT(askIncommingInvitation()) );
#endif
			}
		}
		else if(message.header.sessionId == 64)
		{
			// A contact has sent an inkformat (handwriting) gif.
			// NOTE The entire message body is UTF16 encoded.
			QString body = "";
			for (quint32 i=0; i < message.header.totalDataSize; i++){
				if (message.body[i] != QChar('\0')){
					body += QChar(message.body[i]);
				}
			}

			QRegExp regex("Content-Type: ([A-Za-z0-9$!*/\\-]*)");
			regex.indexIn(body);
			QString contentType = regex.cap(1);

			if(contentType == "image/gif")
			{
				IncomingTransfer transfer(message.source, this, message.header.sessionId);
				transfer.acknowledge(message);

				regex = QRegExp("base64:([0-9a-zA-Z+/=]*)");
				regex.indexIn(body);
				QString base64 = regex.cap(1);
				QByteArray image;
// 				Convert from base64 encoding to byte array.
				KCodecs::base64Decode(base64.toUtf8(), image);
// 				Create a temporary file to store the image data.
				KTemporaryFile *ink = new KTemporaryFile();
				ink->setPrefix("inkformatgif-");
				ink->setSuffix(".gif");
				ink->open();
// 				Save the image data to disk.
				ink->write(image);
				ink->close();
				displayIconReceived(ink, "inkformatgif");
				ink = 0l;
			}
		}
	}
}

void Dispatcher::messageAcknowledged(unsigned int correlationId, bool fullReceive)
{
	if(fullReceive)
	{
		TransferContext *current = 0l;
		QMap<quint32, TransferContext*>::Iterator it = m_sessions.begin();
		for(; it != m_sessions.end(); it++)
		{
			current = it.value();
			if(current->m_transactionId == correlationId)
			{
				// Inform the transfer object of the acknowledge.
				current->readyWrite();
				break;
			}
		}
	}
}

Kopete::Contact* Dispatcher::getContactByAccountId(const QString& accountId)
{
	Kopete::Contact *contact = 0l;
	if(parent())
	{
		// Retrieve the contact from the current chat session context.
		Kopete::ChatSession *session = dynamic_cast<Kopete::ChatSession*>(parent()->parent());
		if(session)
		{
			contact = session->account()->contacts()[accountId];
			session->setCanBeDeleted(false);
		}
	}
	return contact;
}

Dispatcher::CallbackChannel::CallbackChannel(MSNSwitchBoardSocket *switchboard)
{
	m_switchboard = switchboard;
}

Dispatcher::CallbackChannel::~CallbackChannel()
{}

quint32 Dispatcher::CallbackChannel::send(const QByteArray& stream)
{
	return m_switchboard->sendCommand("MSG", "D", true, stream, true);
}

Dispatcher::CallbackChannel* Dispatcher::callbackChannel()
{
	if(m_callbackChannel == 0l){
		MSNSwitchBoardSocket *callback = dynamic_cast<MSNSwitchBoardSocket *>(parent());
		if(callback == 0l) return 0l;
		m_callbackChannel = new Dispatcher::CallbackChannel(callback);
	}

	return m_callbackChannel;
}

#include "dispatcher.moc"
