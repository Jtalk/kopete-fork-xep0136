/*
    msnsocket.h - Base class for the sockets used in MSN

    Copyright (c) 2002 by Martijn Klingens       <klingens@kde.org>
    Kopete    (c) 2002 by the Kopete developers  <kopete-devel@kde.org>

    Portions of this code are taken from KMerlin,
              (c) 2001 by Olaf Lueg              <olueg@olsd.de>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef MSNSOCKET_H
#define MSNSOCKET_H

#include <qobject.h>
#include <qmap.h>

class KExtendedSocket;

/**
 * @author Martijn Klingens <klingens@kde.org>
 *
 * MSNSocket encapsulates the common functionality shared by the Dispatch
 * Server, the Notification Server and the Switchboard Server. It is
 * inherited by the various specialized classes.
 */
class MSNSocket : public QObject
{
	Q_OBJECT

public:
	MSNSocket();
	~MSNSocket();

	/**
	 * Asynchronously read a block of data of the specified size. When the
	 * data is available, the blockRead() signal will be emitted with the
	 * data as parameter.
	 *
	 * NOTE: As the block queue takes precedence over the line-based
	 *       command-processing this method can effectively block all
	 *       communications when passed a wrong length!
	 */
	void readBlock( uint len );

	/**
	 * OnlineStatus encapsulates the 4 states a connection can be in,
	 * Connecting, Connected, Disconnecting, Disconnected. Connecting
	 * and Disconnecting are in the default implementation not used,
	 * because the socket connect is an atomic operation and not yet
	 * performed asynchronously.
	 * In derived classes, like the Notification Server, this state is
	 * actively used, because merely having a socket connection established
	 * by no means indicates we're actually online - the rest of the
	 * handshake likely has to follow first.
	 */
	enum OnlineStatus { Connecting, Connected, Disconnecting, Disconnected };
	enum LookupStatus { Processing, Success, Failed };

	void connect( const QString &server, uint port );
	virtual void disconnect();

	OnlineStatus onlineStatus() { return m_onlineStatus; }

	/**
	 * Send an MSN command to the socket
	 *
	 * For debugging it's convenient to have this method public, but using
	 * it outside this class is deprecated for any other use!
	 *
	 * The size of the body (if any) is automatically added to the argument
	 * list and shouldn't be explicitly specified! This size is in bytes
	 * instead of characters to reflect what actually goes over the wire.
	 */
	void sendCommand( const QString &cmd, const QString &args = QString::null,
		bool addId = true, const QString &body = QString::null );

signals:
	/**
	 * A block read is ready.
	 * After this the normal line-based reads go on again
	 */
	void blockRead( const QString &block );
  void blockRead( const QByteArray &block );

	/**
	 * The online status has changed
	 */
	void onlineStatusChanged( MSNSocket::OnlineStatus status );

	/**
	 * The connection failed
	 */
	void connectionFailed();

	/**
	 * The connection was closed
	 */
	void socketClosed( int );

protected:
	/**
	 * Convenience method: escape spaces with '%20' for use in the protocol.
	 * Doesn't escape any other sequence.
	 */
	QString escape( const QString &str );

	/**
	 * And the other way round...
	 */
	QString unescape( const QString &str );

	/**
	 * Set the online status. Emits onlineStatusChanged.
	 */
	void setOnlineStatus( OnlineStatus status );

	/**
	 * This method is called directly before the socket will actually connect.
	 * Override in derived classes to setup whatever is needed before connect.
	 */
	virtual void aboutToConnect();

	/**
	 * Directly after the connect, this method is called. The default
	 * implementation sets the OnlineStatus to Connected, be sure to override
	 * this if a handshake is required.
	 */
	virtual void doneConnect();

	/**
	 * Directly after the disconnect, this method is called before the actual
	 * cleanup takes place. The socket is close here. Cleanup internal
	 * variables here.
	 */
	virtual void doneDisconnect();

	/**
	 * Handle an MSN error condition.
	 * The default implementation displays a generic error message and
	 * closes the connection. Override to allow more graceful handling and
	 * possibly recovery.
	 */
	virtual void handleError( uint code, uint id );

	/**
	 * Handle an MSN command response line.
	 * This method is pure virtual and *must* be overridden in derived
	 * classes.
	 */
	virtual void parseCommand( const QString &cmd, uint id,
		const QString &data ) = 0;

  /** Used in MSNFileTransferSocket */
  virtual void bytesReceived(const QByteArray &);

    
	const QString &server() { return m_server; }
	uint port() { return m_port; }

 	/**
	 * The last confirmed ID by the server
	 */
	uint m_lastId;

  

private slots:
	void slotDataReceived();
	/**
	 * If the socket emits a connectionFailed() then this slot is called
	 * to handle the error.
	 */
	void slotSocketError( int error );

	/*
	 * Calls connectDone() when connection is successfully established.
	 */
	void slotConnectionSuccess();

	/**
	 * Sets m_lookupProgress to 'Finished' if count > 0 or 'Failed' if count = 0.
	 */
	void slotLookupFinished( int count );

	/**
	 * Check if new lines of data are available and process the first line
	 */
	void slotReadLine();
	
	void slotSocketClosed( int state );

private:
	/**
	 * Check if we're waiting for a block of raw data. Emits blockRead()
	 * when the data is available.
	 * Returns true when still waiting and false when there is no pending
	 * read, or when the read is succesfully handled.
	 */
	bool pollReadBlock();

	/**
	 * The id of the message sent to the MSN server. This ID will increment
	 * for each subsequent message sent.
	 */
	uint m_id;


	/**
	 * Queue of pending commands (should be mostly empty, but is needed to
	 * send more than one command to the server)
	 */
	QMap<uint, QCString> m_sendQueue;

	/**
	 * Parse a single line of data.
	 * Will call either parseCommand or handleError depending on the type of
	 * data received.
	 */
	void parseLine( const QString &str );

	KExtendedSocket *m_socket;
	OnlineStatus m_onlineStatus;

	QString m_server;
	uint m_port;

	/**
	 * Contains the status of the Lookup process.
	 */
	LookupStatus m_lookupStatus;

	/**
	 * The size of the requested block for block-based reads
	 */
	uint m_waitBlockSize;

  class Buffer : public QByteArray
  {
      public:
        Buffer(unsigned int sz=0);
        ~Buffer();
        void add(char *str,unsigned int size);
        QByteArray take(unsigned int size);
      
  };
  Buffer m_buffer;
};

#endif



/*
 * Local variables:
 * c-indentation-style: k&r
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 */
// vim: set noet ts=4 sts=4 sw=4:

