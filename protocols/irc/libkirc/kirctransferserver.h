/*
    kirctransfer.h - DCC Handler

    Copyright (c) 2003      by Michel Hermier <michel.hermier@wanadoo.fr>

    Kopete    (c) 2003      by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef KIRCTRANSFERSERVER_H
#define KIRCTRANSFERSERVER_H

#include <qobject.h>

class KExtendedSocket;

class QFile;
class QTextCodec;

class KIRCTransferServer
	: public QObject
{
	Q_OBJECT

public:
	KIRCTransferServer( QObject *parent = 0, const char *name = 0 );
	KIRCTransferServer( Q_UINT16 port, int backlog = 1, QObject *parent = 0, const char *name = 0 );

//signals:
//	void incomingNewConnection();

protected:
	bool initServer();
	bool initServer( Q_UINT16 port, int backlog = 1 );

protected slots:
	void readyAccept();

private:
	KExtendedSocket *	m_socket;
	Q_UINT16		m_port;
	int			m_backlog;
};

#endif

// vim: set noet ts=4 sts=4 sw=4:

