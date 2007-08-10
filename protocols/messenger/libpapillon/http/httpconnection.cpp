/*
   httpconnection.cpp - HTTP connection over SSL

   Copyright (c) 2007 by Michaël Larouche <larouche@kde.org>

   *************************************************************************
   *                                                                       *
   * This library is free software; you can redistribute it and/or         *
   * modify it under the terms of the GNU Lesser General Public            *
   * License as published by the Free Software Foundation; either          *
   * version 2 of the License, or (at your option) any later version.      *
   *                                                                       *
   *************************************************************************
 */
#include "Papillon/Http/Connection"

// Qt includes
#include <QtCore/QQueue>
#include <QtDebug>

// Papillon includes
#include "Papillon/Network/IpEndpointConnector"
#include "Papillon/Network/NetworkStream"
#include "Papillon/Http/CoreProtocol"
#include "Papillon/Http/Transfer"

namespace Papillon
{

class HttpConnection::Private
{
public:
	Private()
	 : connector(0)
	{}

	~Private()
	{
		delete connector;
	}
	
	IpEndpointConnector *connector;
	HttpCoreProtocol protocol;
	QQueue<HttpTransfer*> transferQueue;
	QString cookie;
};

HttpConnection::HttpConnection(QObject *parent)
 : QObject(parent), d(new Private)
{
	d->connector = new IpEndpointConnector(true, this); // Use SSL/TLS

	connect(d->connector, SIGNAL(connected()), this, SIGNAL(connected()));
	connect(d->connector, SIGNAL(closed()), this, SIGNAL(disconnected()));
	connect(d->connector->networkStream(), SIGNAL(readyRead()), this, SLOT(streamReadyRead()));

	connect(&d->protocol, SIGNAL(outgoingData(QByteArray)), this, SLOT(protocolOutgoingData(QByteArray)));
	connect(&d->protocol, SIGNAL(incomingData()), this, SLOT(protocolIncomingData()));
}

HttpConnection::~HttpConnection()
{
	delete d;
}

void HttpConnection::setCookie(const QString &cookie)
{
	d->cookie = cookie;
}

QString HttpConnection::cookie() const
{
	return d->cookie;
}

void HttpConnection::connectToServer(const QString &server)
{
	d->connector->connectWithAddressInfo(server, 443);
}

void HttpConnection::disconnectFromServer()
{
	d->connector->close();
}

HttpTransfer *HttpConnection::read()
{
	if( d->transferQueue.isEmpty() )
		return 0;
	else
		return d->transferQueue.dequeue();
}

void HttpConnection::write(HttpTransfer *transfer)
{
	qDebug() << Q_FUNC_INFO << "Sending an HttpTransfer on the server.";

	d->protocol.outgoingTransfer(transfer);
}

void HttpConnection::streamReadyRead()
{
	d->protocol.addIncomingData( d->connector->networkStream()->readAll() );
}

void HttpConnection::protocolOutgoingData(const QByteArray &data)
{
	d->connector->networkStream()->write(data);
}

void HttpConnection::protocolIncomingData()
{
	HttpTransfer *incoming = d->protocol.incomingTransfer();
	if( incoming )
	{
		d->transferQueue.enqueue( incoming );
		emit readyRead();
	}
}

}

#include "httpconnection.moc"
