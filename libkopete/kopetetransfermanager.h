/***************************************************************************
                          kopetetransfermanager.h  -  description
                             -------------------
    begin                : Sat Aug 3 2002
    copyright            : (C) 2002 by nbetcher, lilachaze
    email                : nbetcher@kde.org, kopete@metafoo.co.uk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KOPETEFILETRANSFER_H
#define KOPETEFILETRANSFER_H

#include <qobject.h>
#include <qstring.h>
#include <qmap.h>

#include <klistview.h>
#include <kio/job.h>

#include "kopetefiletransferui.h"

class KopeteTransfer;
class KopeteContact;

/**
 * @author Nick Betcher. <nbetcher@kde.org>
 */
class KopeteFileTransferInfo
{
public:
	enum KopeteTransferDirection { Incoming, Outgoing };

	KopeteFileTransferInfo( KopeteContact *, const QString&, const unsigned long size, const QString &,KopeteFileTransferInfo::KopeteTransferDirection di, const unsigned int id, QString internalId=QString::null);
	~KopeteFileTransferInfo(){}
	unsigned int transferId() const { return mId; }
	const KopeteContact* contact() const { return mContact; }
	QString file() const { return mFile; }
	QString recipient() const { return mRecipient; }
	unsigned long size() const { return mSize; }
	QString internalId() const { return m_intId; }
	KopeteTransferDirection direction() const { return mDirection; }

private:
	unsigned long mSize;
	QString mRecipient;
	unsigned int mId;
	KopeteContact *mContact;
	QString mFile;
	QString m_intId;
	KopeteTransferDirection mDirection;
};

/**
 * Creates and manages kopete file transfers
 */
class KopeteTransferManager : public QObject
{
	Q_OBJECT

public:
	/**
	 * Retrieve the transfer manager instance
	 */
	static KopeteTransferManager* transferManager();
	virtual ~KopeteTransferManager(){};

	KopeteTransfer *addTransfer( KopeteContact *contact, const QString& file, const unsigned long size, const QString &recipient , KopeteFileTransferInfo::KopeteTransferDirection di);
	int askIncomingTransfer( KopeteContact *contact, const QString& file, const unsigned long size, const QString& description=QString::null, QString internalId=QString::null);
	void removeTransfer( unsigned int id );

	/**
	 * Possibly ask the user which file to send when they click Send File. Sends a signal indicating KURL to
	 * send when the local user accepts the transfer.
	 * @param file If valid, the user will not be prompted for a URL, and this one will be used instead.
	 *  If it refers to a remote file and mustBeLocal is true, the file will be transferred to the local
	 *  filesystem.
	 * @param localFile file name to display if file is a valid URL
	 * @param fileSize file size to send if file is a valid URL
	 * @param mustBeLocal If the protocol can only send files on the local filesystem, this flag
	 *  allows you to ensure the filename will be local.
	 * @param sendTo The object to send the signal to
	 * @param slot The slot to send the signal to. Signature: sendFile(const KURL &file)
	 */
	void sendFile( const KURL &file, const QString &localFile, unsigned long fileSize,
		bool mustBeLocal, QObject *sendTo, const char *slot );

signals:
	void done( KopeteTransfer* );
	void canceled( KopeteTransfer* );

	void accepted(KopeteTransfer*, const QString &fileName);
	void refused(const KopeteFileTransferInfo& );

	void sendFile(const KURL &file, const QString &localFile, unsigned int fileSize);

private slots:
	void slotAccepted(const KopeteFileTransferInfo&, const QString&);
	void slotComplete(KopeteTransfer*);

private:
	KopeteTransferManager( QObject *parent );
	static KopeteTransferManager *s_transferManager;

	int nextID;
	QMap<unsigned int, KopeteTransfer *> mTransfersMap;
};

/**
 * A KIO job for a kopete file transfer.
 * @author Richard Smith <kopete@metafoo.co.uk>
 */
class KopeteTransfer : public KIO::Job
{
	Q_OBJECT

public:
	enum KopeteTransferError
	{
		CanceledLocal,
		CanceledRemote,
		Timeout,
		Refused,
		Other
	};
	KopeteTransfer( const KopeteFileTransferInfo &, const QString &localFile, bool showProgressInfo = true);
	KopeteTransfer( const KopeteFileTransferInfo &, const KopeteContact *toUser, bool showProgressInfo = true);
	~KopeteTransfer();
	const KopeteFileTransferInfo &info() const { return mInfo; }

	/**
	 * Deprecated! Use @ref slotError instead.
	 */
	void setError(KopeteTransferError error);

	/**
	 * Retrieve a URL indicating where the file is being copied from.
	 * For display purposes only! There's no guarantee that this URL
	 * refers to a real file being transferred.
	 */
	KURL sourceURL();
	/**
	 * Retrieve a URL indicating where the file is being copied to.
	 * See @ref sourceURL
	 */
	KURL destinationURL();

public slots:
	/**
	 * Deprecated!! Use @ref slotProcessed and slotFinished instead.
	 * FIXME: find all uses, change to slotProcessed, and remove this function.
	 */
	void slotPercentCompleted(unsigned int);
	/**
	 * Set the file size processed so far
	 */
	void slotProcessed(unsigned int);
	/**
	 * Indicate that the transfer is complete
	 */
	void slotComplete();

	/**
	 * Inform the job that an error has occurred while transferring the file.
	 * @param error A member of the @ref KIO::Error enumeration indicating what error occurred.
	 * @param errorText A string to aid understanding of the error, often the offending URL.
	 */
	void slotError( int error, const QString &errorText );

signals:
	/**
	 * Deprecated; use result() and check error() for ERR_USER_CANCELLED
	 */
	void transferCanceled();

private:
	void init( const KURL &, bool );

	static KURL displayURL( const KopeteContact *contact, const QString &file );

	KopeteFileTransferInfo mInfo;
	KURL mTarget;
	int mPercent;

private slots:
	void slotResultEmitted();
};

#endif

// vim: set noet ts=4 sts=4 sw=4:

