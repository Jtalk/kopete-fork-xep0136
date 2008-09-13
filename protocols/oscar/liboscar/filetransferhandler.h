/*
    filetransferhandler.h  -  File Transfer Handler

    Copyright (c) 2008 by Roman Jarosz <kedgedev@centrum.cz>
    Kopete    (c) 2008 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/
#ifndef FILETRANSFERHANDLER_H
#define FILETRANSFERHANDLER_H

#include <QtCore/QObject>
#include "oscartypes.h"

#include "liboscar_export.h"

class FileTransferTask;

class LIBOSCAR_EXPORT FileTransferHandler : public QObject
{
	Q_OBJECT
public:
	FileTransferHandler( FileTransferTask* fileTransferTask );

	void send();

	QString internalId() const;
	QString contact() const;
	QString fileName() const;
	Oscar::DWORD totalSize() const;
	QString description() const;

public Q_SLOTS:
	void cancel();
	void accept( const QString &path );

Q_SIGNALS:
	void transferCancelled();
	void transferError( int errorCode, const QString &error );
	void transferFinished();
	void transferProcessed( unsigned int totalSent );

	void transferNextFile( const QString& fileName, unsigned int fileSize );
	void transferFileProcessed(unsigned int bytesSent, unsigned int fileSize );

private:
	FileTransferTask* mFileTransferTask;
};

#endif
