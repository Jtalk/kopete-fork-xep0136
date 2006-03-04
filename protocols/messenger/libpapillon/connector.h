/*
   connector.h - Papillon Socket connector abstract class.

   Copyright (c) 2006 by Michaël Larouche <michael.larouche@kdemail.net>

   Based on code Copyright (c) 2004 Matt Rogers <mattr@kde.org>
   Based on code copyright (c) 2004 SuSE Linux AG <http://www.suse.com>
   Based on Iris, Copyright (C) 2003 Justin Karneges

   *************************************************************************
   *                                                                       *
   * This library is free software; you can redistribute it and/or         *
   * modify it under the terms of the GNU Lesser General Public            *
   * License as published by the Free Software Foundation; either          *
   * version 2 of the License, or (at your option) any later version.      *
   *                                                                       *
   *************************************************************************
*/
#ifndef PAPILLON_CONNECTOR_H
#define PAPILLON_CONNECTOR_H


#include <QObject>
#include <QtNetwork/QHostAddress>

#include <papillon_macros.h>

namespace Papillon
{

class ByteStream;

class PAPILLON_EXPORT Connector : public QObject
{
	Q_OBJECT
public:
	Connector(QObject *parent=0);
	virtual ~Connector();

	virtual void connectToServer(const QString &server, quint16 port)=0;
	virtual ByteStream *stream() const=0;
	virtual void done()=0;

	bool havePeerAddress() const;
	QHostAddress peerAddress() const;
	quint16 peerPort() const;

signals:
	void connected();
	void error();

protected:
	void setPeerAddressNone();
	void setPeerAddress(const QHostAddress &addr, quint16 port);

private:
	class Private;
	Private *d;
};

}
#endif
