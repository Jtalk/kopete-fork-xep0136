/*
    kirctransferhandler.h - DCC Handler

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

#ifndef KIRCTRANSFERHANDLER_H
#define KIRCTRANSFERHANDLER_H

#include <qhostaddress.h>

#include "kirctransfer.h"

class QFile;
class QTextCodec;

class KExtendedSocket;

class KIRCTransferServer;

class KIRCTransferHandler
	: public QObject
{
	Q_OBJECT

//protected:
//	KIRCTransferHandler();

public:
	static KIRCTransferHandler *self()
	{
		return &m_self;
	}


	KIRCTransferServer *server();
	KIRCTransferServer *server( Q_UINT16 port, int backlog  = 1 );

	KIRCTransfer *createClient(
		QHostAddress peer_address, Q_UINT16 peer_port,
		KIRCTransfer::Type type,
		QFile *file = 0L, Q_UINT32 file_size = 0 );

//	void registerServer( DCCServer * );
//	QPtrList<DCCServer> getRegisteredServers();
//	static QPtrList<DCCServer> getAllRegisteredServers();
//	void unregisterServer( DCCServer * );

//	void registerClient( DCCClient * );
//	QPtrList<DCCClient> getRegisteredClients();
//	static QPtrList<DCCClient> getAllRegisteredClients();
//	void unregisterClient( DCCClient * );

signals:
//	void DCCServerCreated( DCCServer *server );
	void transferCreated( KIRCTransfer *transfer );

private:
	static KIRCTransferHandler m_self;

	KIRCTransferServer *m_server;
//	QPtrList<DCCServer> m_DCCServers;
//	QPtrList<DCCClient> m_DCCClients;

//	static QPtrList<DCCServer> sm_DCCServers;
//	static QPtrList<DCCClient> sm_DCCClients;
};

#endif

// vim: set noet ts=4 sts=4 sw=4:
