/*
    kirc_ctcp.h - IRC Client

    Copyright (c) 2003      by Michel Hermier <michel.hermier@wanadoo.fr>
    Copyright (c) 2002      by Nick Betcher <nbetcher@kde.org>
    Copyright (c) 2003      by Jason Keirstead <jason@keirstead.org>

    Kopete    (c) 2002-2003 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include <qfileinfo.h>

#include "kirctransferhandler.h"

#include "kirc.h"

// Normal order for a ctcp commands:
// CtcpRequest_*
// CtcpQuery_*
// CtcpReply_* (if any)

/* Generic ctcp commnd for the /ctcp trigger */
void KIRC::CtcpRequestCommand(const QString &contact, const QString &command)
{
	if(m_status == Connected)
	{
		writeCtcpQueryMessage(contact, QString::null, command);
//		emit ctcpCommandMessage( contact, command );
	}
}

void KIRC::CtcpRequest_action(const QString &contact, const QString &message)
{
	if(m_status == Connected)
	{
		writeCtcpQueryMessage(contact, QString::null, "ACTION", QStringList(message));

		if( isChannel(contact) )
			emit incomingPrivAction(m_Nickname, contact, message);
		else
			emit incomingAction(m_Nickname, contact, message);
	}
}

bool KIRC::CtcpQuery_action(const KIRCMessage &msg)
{
	QString target = msg.args()[0];
	if (target[0] == '#' || target[0] == '!' || target[0] == '&')
		emit incomingAction(msg.nickFromPrefix(), target, msg.ctcpMessage().ctcpRaw());
	else
		emit incomingPrivAction(msg.nickFromPrefix(), target, msg.ctcpMessage().ctcpRaw());
	return true;
}

/*
NO REPLY EXIST FOR THE CTCP ACTION COMMAND !
bool KIRC::CtcpReply_action(const KIRCMessage &msg)
{
}
*/

void KIRC::CtcpRequest_dcc(const QString &nickname, const QString &filename, uint port, KIRCTransfer::Type type)
{
	if(	m_status != Connected ||
		m_sock->localAddress() == 0 ||
		m_sock->localAddress()->nodeName() == QString::null)
		return;

	switch( type )
	{
	case KIRCTransfer::Chat:
		writeCtcpQueryMessage(nickname, QString::null,
			QString("DCC"),
			QStringList(QString::fromLatin1("CHAT")) << QString::fromLatin1("chat") <<
			m_sock->localAddress()->nodeName() << QString::number(port));
		break;
	case KIRCTransfer::FileOutgoing:
		QFileInfo file(filename);
		QString noWhiteSpace = file.fileName();
		if (noWhiteSpace.contains(' ') > 0)
			noWhiteSpace.replace(QRegExp("\\s+"), "_");

		writeCtcpQueryMessage(nickname, QString::null,
			QString("DCC"),
			QStringList( QString::fromLatin1( "SEND" ) ) << noWhiteSpace <<
			    m_sock->localAddress()->nodeName() << QString::number( port ) << QString::number( file.size() ) );
		break;
	}
}

bool KIRC::CtcpQuery_dcc(const KIRCMessage &msg)
{
	const KIRCMessage &ctcpMsg = msg.ctcpMessage();
	QString dccCommand = ctcpMsg.args()[0].upper();

	if (dccCommand == QString::fromLatin1("CHAT"))
	{
		if(ctcpMsg.args().size()!=4) return false;

		/* DCC CHAT type longip port
		 *
		 *  type   = Either Chat or Talk, but almost always Chat these days
		 *  longip = 32-bit Internet address of originator's machine
		 *  port   = Port on which the originator is waitng for a DCC chat
		 */
		bool okayHost, okayPort;
		// should ctctArgs[1] be tested?
		QHostAddress address(ctcpMsg.args()[2].toUInt(&okayHost));
		unsigned int port = ctcpMsg.args()[3].toUInt(&okayPort);
		if (okayHost && okayPort)
		{
			kdDebug(14120) << "Starting DCC chat window." << endl;
			KIRCTransfer *chatObject = KIRCTransferHandler::self()->createClient(address, port, KIRCTransfer::Chat );
			emit incomingDccChatRequest(address, port, msg.nickFromPrefix(), chatObject);
			return true;
		}
	}
	else if (dccCommand == QString::fromLatin1("SEND"))
	{
		if(ctcpMsg.args().size()!=5) return false;

		/* DCC SEND (filename) (longip) (port) (filesize)
		 *
		 *  filename = Name of file being sent
		 *  longip   = 32-bit Internet address of originator's machine
		 *  port     = Port on which the originator is waiitng for a DCC chat
		 *  filesize = Size of file being sent
		 */
		bool okayHost, okayPort, okaySize;
		QFileInfo realfile(msg.args()[1]);
		QHostAddress address(ctcpMsg.args()[2].toUInt(&okayHost));
		unsigned int port = ctcpMsg.args()[3].toUInt(&okayPort);
		unsigned int size = ctcpMsg.args()[4].toUInt(&okaySize);
		if (okayHost && okayPort && okaySize)
		{
			kdDebug(14120) << "Starting DCC send file transfert." << endl;
			KIRCTransfer *chatObject = KIRCTransferHandler::self()->createClient(address, port, KIRCTransfer::FileIncoming, (QFile *)0L, size );
			emit incomingDccSendRequest(address, port, msg.nickFromPrefix(), realfile.fileName(), size, chatObject);
			return true;
		}
	}
//	else
//		emit unknown dcc command signal
	return false;
}

/*
NO REPLY EXIST FOR THE CTCP DCC COMMAND !
bool KIRC::CtcpReply_dcc(const KIRCMessage &msg)
{
}
*/

//	FIXME: the API can now answer to help commands.
bool KIRC::CtcpQuery_clientInfo(const KIRCMessage &msg)
{
	QString response = customCtcpMap[ QString::fromLatin1("clientinfo") ];
	if( !response.isNull() )
	{
		writeCtcpReplyMessage(	msg.nickFromPrefix(), QString::null,
					msg.ctcpMessage().command(), QStringList(), response);
	}
	else
	{
		QString info = QString::fromLatin1("The following commands are supported, but "
			"without sub-command help: VERSION, CLIENTINFO, USERINFO, TIME, SOURCE, PING,"
			"ACTION.");

		writeCtcpReplyMessage(	msg.nickFromPrefix(), QString::null,
					msg.ctcpMessage().command(), QStringList(), info);
	}
	return true;
}

bool KIRC::CtcpQuery_finger( const KIRCMessage & /* msg */ )
{
	// To be implemented
	return true;
}

void KIRC::CtcpRequest_pingPong(const QString &target)
{
	kdDebug(14120) << k_funcinfo << endl;

	timeval time;
	if (gettimeofday(&time, 0) == 0)
	{
		QString timeReply;

		if( isChannel(target) )
			timeReply = QString::fromLatin1("%1.%2").arg(time.tv_sec).arg(time.tv_usec);
		else
		 	timeReply = QString::number( time.tv_sec );

		writeCtcpQueryMessage(	target, QString::null, "PING", timeReply);
	}
}

bool KIRC::CtcpQuery_pingPong(const KIRCMessage &msg)
{
	writeCtcpReplyMessage(	msg.nickFromPrefix(), QString::null,
				msg.ctcpMessage().command(), msg.ctcpMessage().args()[0]);
	return true;
}

bool KIRC::CtcpReply_pingPong( const KIRCMessage &msg )
{
	timeval time;
	if (gettimeofday(&time, 0) == 0)
	{
		// FIXME: the time code is wrong for usec
		QString timeReply = QString::fromLatin1("%1.%2").arg(time.tv_sec).arg(time.tv_usec);
		double newTime = timeReply.toDouble();
		double oldTime = msg.suffix().section(' ',0, 0).toDouble();
		double difference = newTime - oldTime;
		QString diffString;

		if (difference < 1)
		{
			diffString = QString::number(difference);
			diffString.remove((diffString.find('.') -1), 2);
			diffString.truncate(3);
			diffString.append("milliseconds");
		}
		else
		{
			diffString = QString::number(difference);
			QString seconds = diffString.section('.', 0, 0);
			QString millSec = diffString.section('.', 1, 1);
			millSec.remove(millSec.find('.'), 1);
			millSec.truncate(3);
			diffString = QString::fromLatin1("%1 seconds, %2 milliseconds").arg(seconds).arg(millSec);
		}

		emit incomingCtcpReply(QString::fromLatin1("PING"), msg.nickFromPrefix(), diffString);

		return true;
	}

	return false;
}

bool KIRC::CtcpQuery_source(const KIRCMessage &msg)
{
	writeCtcpReplyMessage(	msg.nickFromPrefix(), QString::null,
				msg.ctcpMessage().command(), QStringList(m_SourceString));
	return true;
}

bool KIRC::CtcpQuery_time(const KIRCMessage &msg)
{
	writeCtcpReplyMessage(	msg.nickFromPrefix(), QString::null,
				msg.ctcpMessage().command(), QStringList(QDateTime::currentDateTime().toString()), QString::null,
				false);
	return true;
}

bool KIRC::CtcpQuery_userInfo(const KIRCMessage &msg)
{
	QString response = customCtcpMap[ QString::fromLatin1("userinfo") ];
	if( !response.isNull() )
	{
		writeCtcpReplyMessage(msg.nickFromPrefix(), QString::null,
			msg.ctcpMessage().command(), QStringList(), response);
	}
	else
	{
		writeCtcpReplyMessage( msg.nickFromPrefix(), QString::null,
				msg.ctcpMessage().command(), QStringList(), m_UserString );
	}

	return true;
}

void KIRC::CtcpRequest_version(const QString &target)
{
	writeCtcpQueryMessage(target, QString::null, "VERSION");
}

bool KIRC::CtcpQuery_version(const KIRCMessage &msg)
{
	QString response = customCtcpMap[ QString::fromLatin1("version") ];
	kdDebug(14120) << "Version check: " << response << endl;

	if( !response.isNull() )
	{
		writeCtcpReplyMessage(msg.nickFromPrefix(), QString::null,
			msg.ctcpMessage().command(), QStringList(), response);
	}
	else
	{
		writeCtcpReplyMessage(msg.nickFromPrefix(), QString::null,
			msg.ctcpMessage().command(), QStringList(), m_VersionString);
	}

	return true;
}

bool KIRC::CtcpReply_version(const KIRCMessage &msg)
{
	emit incomingCtcpReply(msg.ctcpMessage().command(), msg.nickFromPrefix(), msg.ctcpMessage().ctcpRaw());
	return true;
}
