/***************************************************************************
                          msnfiletransfersocket.h  -  description
                             -------------------
    begin                : mer jui 31 2002
    copyright            : (C) 2002 by Olivier Goffart
    email                : ogoffart@tiscalinet.be
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MSNFILETRANSFERSOCKET_H
#define MSNFILETRANSFERSOCKET_H

#include <qwidget.h>

#include "msnsocket.h"
#include "msninvitation.h"

class QFile;

class KExtendedSocket;

class KopeteTransfer;
class KopeteFileTransferInfo;
class KopeteProtocol;
class KopeteContact;

/**
 * @author Olivier Goffart
 */
class MSNFileTransferSocket : public MSNSocket , public MSNInvitation
{
	Q_OBJECT

public:
	MSNFileTransferSocket(const QString &myID,KopeteContact* c, bool incoming, QObject* parent = 0L );
	~MSNFileTransferSocket();

	static QString applicationID() { return "5D3E02AB-6190-11d3-BBBB-00C04F795683"; }
	QCString invitationHead();


	void setKopeteTransfer( KopeteTransfer *kt );
	KopeteTransfer* kopeteTransfer() { return m_kopeteTransfer; }
	void setFile( const QString &fn, long unsigned int fileSize = 0L );
	void setAuthCookie( const QString &c ) { m_authcook = c; }
	QString fileName() { return m_fileName;}
	long unsigned int size() { return m_size;}
	void listen( int port );

	virtual void parseInvitation(const QString& invitation);

	virtual QObject* object() { return this; }

public slots:
	void abort();

signals:
	void done( MSNInvitation * );

protected:
	/**
	 * This reimplementation sets up the negotiating with the server and
	 * suppresses the change of the status to online until the handshake
	 * is complete.
	 */
	virtual void doneConnect();

	/**
	 * Handle an MSN command response line.
	 */
	virtual void parseCommand(const QString & cmd, uint id, const QString & data);
	virtual void bytesReceived(const QByteArray & data);

protected slots:
	virtual void slotReadyWrite();

private slots:
	void slotSocketClosed();
	void slotReadBlock(const QByteArray &);
	void slotAcceptConnection();
	void slotTimer();
	void slotSendFile();

	void slotFileTransferRefused( const KopeteFileTransferInfo &info );
	void slotFileTransferAccepted( KopeteTransfer *trans, const QString& fileName );


private:
	QString m_handle;
	KopeteContact *m_contact;

	long unsigned int m_size;
	long unsigned int m_downsize;
	QString m_authcook;
	QString m_fileName;
	KopeteTransfer* m_kopeteTransfer;
	QFile *m_file ;
	KExtendedSocket *m_server;

	bool ready;

};

#endif

// vim: set noet ts=4 sts=4 tw=4:

