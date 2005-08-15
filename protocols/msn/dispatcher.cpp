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
using P2P::Dispatcher;
using P2P::Message;
using P2P::TransferContext;
using P2P::IncomingTransfer;
using P2P::OutgoingTransfer;

#include "msnswitchboardsocket.h"

// Kde includes
#include <kdebug.h>
#include <kmdcodec.h>
#include <kstandarddirs.h>
#include <ktempfile.h>

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

#define MKDWORD(VAR, START, VAL) {\
	(VAR)[(START)]=  (char)( ((VAL)&0x000000FF) ) ;  \
	(VAR)[(START)+1]=(char)( ((VAL)&0x0000FF00)  >> 8 ) ; \
	(VAR)[(START)+2]=(char)( ((VAL)&0x00FF0000)  >> 16 ) ; \
	(VAR)[(START)+3]=(char)( ((VAL)&0xFF000000)  >> 24 ) ; \
}

Dispatcher::Dispatcher(QObject *parent, const QString& contact) : QObject(parent)
{
	m_contact = contact;
	m_callbackChannel = 0l;
}

Dispatcher::~Dispatcher()
{
	kdDebug(14140) << k_funcinfo << endl;

	if(m_callbackChannel)
	{
		delete m_callbackChannel;
		m_callbackChannel = 0l;
	}
/*  // NOTE Object deletes it's children.
	TransferContext *current = 0l;
	QMap<Q_UINT32, TransferContext*>::Iterator it = m_sessions.begin();
	for(; it != m_sessions.end(); it++)
	{
		current = it.data();
		delete current;
	}
*/
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
	Q_UINT32 sessionId = rand()%0xFFFFFF00 + 4;
	TransferContext* current =
		new IncomingTransfer(from, this, sessionId);

	current->m_branch = P2P::Uid::createUid();
	current->m_callId = P2P::Uid::createUid();
	current->setType(P2P::UserDisplayIcon);
	current->m_object = msnObject;
	// Add the transfer to the list.
	m_sessions.insert(sessionId, current);

	kdDebug(14140) << k_funcinfo << "Requesting, " << msnObject << endl;

	QString context = QString::fromUtf8(KCodecs::base64Encode(msnObject.utf8()));
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

void Dispatcher::sendFile(const QString& path, Q_INT64 fileSize, const QString& to)
{
	// Create a new transfer context that will handle
	// the file transfer.
	Q_UINT32 sessionId = rand()%0xFFFFFF00 + 4;
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
// TODO clean up the following, too tired to do it now.
// FIXME there is an error in the file context data.
//       Construct file context data correctly.
//////////////////////////////////

	QByteArray binaryContext(574);
	binaryContext.fill('\0');
	MKDWORD(binaryContext,0,574);  //this is probably the size
	MKDWORD(binaryContext,4,2);    //I don't know.what's this
	MKDWORD(binaryContext,8,fileSize);
	MKDWORD(binaryContext,16,1);    //we don't have preview.

	QByteArray utf16FileName;
	QDataStream stream(utf16FileName, IO_WriteOnly);
	stream.setByteOrder(QDataStream::LittleEndian);
	stream << path.right( path.length() - path.findRev( '/' ) - 1 );

	unsigned int taille = utf16FileName.size();
	if(taille > 574-19-16)
		taille= 574-19-16;
	

	taille-=4; //to skip the string size
	kdDebug() << k_funcinfo << taille << endl;

	for(unsigned int f=0; f<taille; f++)
	{
		//+4 to skip the string length
		binaryContext[20+f]=utf16FileName[f+4];
	}

	binaryContext[570]=binaryContext[571]=binaryContext[572]=binaryContext[573]=0xFF;
	context=QString::fromUtf8(KCodecs::base64Encode( binaryContext ));
//////////////////////////////////

	// Send an INVITE message to the recipient.
	QString content = "EUF-GUID: {5D3E02AB-6190-11D3-BBBB-00C04F795683}\r\n"
			"SessionID: " + QString::number(sessionId) + "\r\n"
			"AppID: 2\r\n"
			"Context: "   + context + "\r\n"
			"\r\n";
	current->sendMessage(INVITE, content);
}

void Dispatcher::sendImage(const QString& fileName, const QString& to)
{
	// TODO have to ask Gof.
}

void Dispatcher::slotReadMessage(const QByteArray& stream)
{
	Message receivedMessage =
		m_messageFormatter.readMessage(stream);

	if(receivedMessage.contentType == "application/x-msnmsgrp2p")
	{
		if((receivedMessage.header.dataSize == 0)/* && ((receivedMessage.header.flag & 0x02) == 0x02)*/)
		{
			TransferContext *current = 0l;
			QMap<Q_UINT32, TransferContext*>::Iterator it = m_sessions.begin();
			for(; it != m_sessions.end(); it++)
			{
				if(receivedMessage.header.ackSessionIdentifier == it.data()->m_identifier){
					current = it.data();
					break;
				}
			}
			
			if(current){
    			// Inform the transfer object of the acknowledge.
				current->acknowledged();
			}
			else
			{
				kdDebug(14140) << k_funcinfo
					<< "no transfer context with identifier, "
					<< receivedMessage.header.ackSessionIdentifier
					<< " found." << endl;
			}
			return;
		}
		
		if(m_messageBuffer.contains(receivedMessage.header.identifier))
		{
			kdDebug(14140) << k_funcinfo
				<< QString("retrieving buffered messsage, %1").arg(receivedMessage.header.identifier)
				<< endl;
				
			// The message was split, try to reconstruct the message
			// with this received piece.
			Message bufferedMessage = m_messageBuffer[receivedMessage.header.identifier];
			// Remove the buffered message.
			m_messageBuffer.remove(receivedMessage.header.identifier);
	
			bufferedMessage.body.resize(bufferedMessage.body.size() + receivedMessage.header.dataSize);
			for(Q_UINT32 i=0; i < receivedMessage.header.dataSize; i++){
				// Add the remaining message data to the buffered message.
				bufferedMessage.body[receivedMessage.header.dataOffset + i] = receivedMessage.body[i];
			}
			bufferedMessage.header.dataSize += receivedMessage.header.dataSize;
			bufferedMessage.header.dataOffset = 0;

			receivedMessage = bufferedMessage;
		}
		
		// Dispatch the received message.
		dispatch(receivedMessage);
	}
}

void Dispatcher::dispatch(const Message& message)

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
			QCString(message.body.data(), message.header.dataSize);
		QRegExp regex("SessionID: ([0-9]*)\r\n");
		if(regex.search(body) > 0)
		{
			Q_UINT32 sessionId = regex.cap(1).toUInt();
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
				regex.search(body);
				QString callId = regex.cap(1);

				TransferContext *current = 0l;
				QMap<Q_UINT32, TransferContext*>::Iterator it = m_sessions.begin();
				for(; it != m_sessions.end(); it++)
				{
					current = it.data();
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
			kdDebug(14140) << k_funcinfo
				<< QString("Buffering messsage, %1").arg(message.header.identifier)
				<< endl;
			m_messageBuffer.insert(message.header.identifier, message);
			return;
		}

		QString body =
			QCString(message.body.data(), message.header.dataSize);
		kdDebug(14140) << k_funcinfo << "received, " << body << endl;

		if(body.startsWith("INVITE"))
		{
			// Retrieve the branch, call id, and session id.
			// These fields will be used later on in the p2p
			// transaction.
			QRegExp regex(";branch=\\{([0-9A-F\\-]*)\\}\r\n");
			regex.search(body);
			QString branch = regex.cap(1);
			regex = QRegExp("Call-ID: \\{([0-9A-F\\-]*)\\}\r\n");
			regex.search(body);
			QString callId = regex.cap(1);
			regex = QRegExp("SessionID: ([0-9]*)\r\n");
			regex.search(body);
			QString sessionId = regex.cap(1);
			// Retrieve the contact that requested the session.
			regex = QRegExp("From: <msnmsgr:([^>]*)>");
			regex.search(body);
			QString from = regex.cap(1);
			// Retrieve the application identifier which
			// is used to determine what type of session
			// is being requested.
			regex = QRegExp("AppID: ([0-9]*)\r\n");
			regex.search(body);
			Q_UINT32 applicationId = regex.cap(1).toUInt();

			if(applicationId == 1)
			{
				// A contact has requested a session to download
				// a display icon (User Display Icon or CustomEmotion).

				regex = QRegExp("Context: ([0-9a-zA-Z+/=]*)");
				regex.search(body);
				QCString msnobj;

				// Decode the msn object from base64 encoding.
				KCodecs::base64Decode(regex.cap(1).utf8() , msnobj);
				kdDebug(14140) << k_funcinfo << "Contact requested, "
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
				if(!source->open(IO_ReadOnly))
				{
					current->error();
					return;
				}

				current->m_file = source;
				// Acknowledge the session request.
				current->acknowledge(message);
				// Send a 200 OK message to the recipient.
				QString content = QString("SessionID: %1\r\n\r\n").arg(sessionId);
				current->sendMessage(OK, content);
			}
			else if(applicationId == 2)
			{
				// A contact has requested a session to
				// send a file.

				kdDebug(14140) << k_funcinfo << "File transfer invitation." << endl;

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
				regex.search(body);
				QByteArray context;

				// Decode the file context from base64 encoding.
				KCodecs::base64Decode(regex.cap(1).utf8(), context);
				QDataStream reader(context, IO_ReadOnly);
				reader.setByteOrder(QDataStream::LittleEndian);
				//Retrieve the file info from the context field.
				// File Size [8..15] Int64
				reader.device()->at(8);
				Q_INT64 fileSize;
				reader >> fileSize;
				// Flag [15..18] Int32
				// 0x00 File transfer with preview data.
				// 0x01 File transfer without preview data.
				// 0x02 Background sharing.
				Q_INT32 flag;
				reader >> flag;
				kdDebug(14140) << flag << endl;
				// FileName UTF16 (Unicode) [19..539]
				QByteArray bytes(520);
				reader.readRawBytes(bytes.data(), bytes.size());
				QTextStream ts(bytes, IO_ReadOnly);
				ts.setEncoding(QTextStream::Unicode);
				QString fileName;
				fileName = ts.readLine().utf8();
				
   				kdDebug(14140) <<
   					QString("%1, %2 bytes.").arg(fileName, QString::number(fileSize))
   					<< endl
   					<< endl;
				
				// Get the contact that is sending the file.
				Kopete::Contact *contact = getContactByAccountId(from);

				if(contact)
				{
					transfer->acknowledge(message);
					
					QObject::disconnect(Kopete::TransferManager::transferManager(), 0l, this, 0l);
					QObject::connect(Kopete::TransferManager::transferManager(), SIGNAL(accepted(Kopete::Transfer*, const QString&)), this, SLOT(slotTransferAccepted(Kopete::Transfer*, const QString&)));
					QObject::connect(Kopete::TransferManager::transferManager(), SIGNAL(refused(const Kopete::FileTransferInfo&)), this, SLOT(slotTransferRefused(const Kopete::FileTransferInfo&)));

					// Show the file transfer accept/decline dialog.
					Kopete::TransferManager::transferManager()->askIncomingTransfer(contact, fileName, fileSize, QString::null, sessionId);
				}
				else
				{
					kdWarning(14140) << fileName << " from " << from
						<< " has failed; could not retrieve contact from contact list."
						<< endl;
					transfer->sendMessage(ERROR);
				}
			}
			else if(applicationId == 4)
			{
				// TODO Webcam support.
			}
		}
		else
		{
			QRegExp regex("Content-Type: ([A-Za-z0-9$!*/\\-]*)");
			regex.search(body);
			QString contentType = regex.cap(1);

			if(contentType == "image/gif")
			{
// 				TODO inkformatgif.
// 				regex = QRegExp("base64:([0-9a-zA-Z+/=]*)");
// 				if(regex.search(body) > 0)
// 				{
// 					QString base64 = regex.cap(1);
// 					kdDebug(14140) << k_funcinfo << "received inkformatgif, " << base64 << endl;
// 					QByteArray image;
// 					// Convert from base64 encoded string to byte array.
// 					KCodecs::base64Decode( base64.utf8() , image );
// 
// 					KTempFile *imageFile = new KTempFile( locateLocal( "tmp", "msntypewrite-" ), ".gif");
// 					imageFile->setAutoDelete(true);
// 					Q_INT32 offset = 0, size = image.size(), chunkLength = 1202;
// 					while(offset < size)
// 					{
// 						if(offset + chunkLength < size)
// 						{
// 							imageFile->file()->writeBlock(image.data() + offset, chunkLength);
// 							offset += chunkLength;
// 						}
// 						else
// 						{
// 							imageFile->file()->writeBlock(image.data(), (offset == 0) ? size : size%offset);
// 							offset += size%offset;
// 						}
// 					}
// 					imageFile->file()->close();
// 
// 					displayIconReceived(imageFile , "typewrite");
// 				}
			}
		}
	}
}

// TODO these slots should be in TransferContext
void Dispatcher::slotTransferAccepted(Kopete::Transfer* transfer, const QString& /*fileName*/)
{
	Q_UINT32 sessionId = transfer->info().internalId().toUInt();
	TransferContext *current = 0l;
	if(m_sessions.contains(sessionId))
	{
		current = m_sessions[sessionId];
		QObject::connect(transfer , SIGNAL(transferCanceled()), current, SLOT(abort()));
		current->m_transfer = transfer;
		
		QString content = QString("SessionID: %1\r\n\r\n").arg(sessionId);
		current->sendMessage(OK, content);
	}
}

void Dispatcher::slotTransferRefused(const Kopete::FileTransferInfo& info)
{
	Q_UINT32 sessionId = info.internalId().toUInt();
	TransferContext *current = 0l;
	if(m_sessions.contains(sessionId))
	{
		current = m_sessions[sessionId];
		QString content = QString("SessionID: %1\r\n\r\n").arg(sessionId);
		// Send the sending client a cancelation message.
		current->sendMessage(DECLINE, content);
	}
}

void Dispatcher::messageAcknowledged(unsigned int correlationId, bool fullReceive)
{
	if(fullReceive)
	{
		TransferContext *current = 0l;
		QMap<Q_UINT32, TransferContext*>::Iterator it = m_sessions.begin();
		for(; it != m_sessions.end(); it++)
		{	
			current = it.data();
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

Q_UINT32 Dispatcher::CallbackChannel::send(const QByteArray& stream)
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
