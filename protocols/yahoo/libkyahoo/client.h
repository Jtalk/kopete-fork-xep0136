/*
    Kopete Yahoo Protocol
    
    Copyright (c) 2005 Andre Duffeck <andre.duffeck@kdemail.net>
    Copyright (c) 2004 Duncan Mac-Vicar P. <duncan@kde.org>
    Copyright (c) 2004 Matt Rogers <matt.rogers@kdemail.net>
    Copyright (c) 2004 SuSE Linux AG <http://www.suse.com>
    Copyright (C) 2003  Justin Karneges
    
    Kopete (c) 2002-2004 by the Kopete developers <kopete-devel@kde.org>
 
    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#ifndef LIBYAHOO_CLIENT_H
#define LIBYAHOO_CLIENT_H

#include <qobject.h>
//Added by qt3to4:
#include <QPixmap>
#include <Q3CString>
#include <kurl.h>

#include "rtf2html.h"
#include "transfer.h"
#include "yahootypes.h"


class QString;
class QTimer;
class ClientStream;
class KNetworkConnector;
class Task;
class KTempFile;

class Client : public QObject
{
Q_OBJECT

	public:
	
		/*************
		  EXTERNAL API 
		 *************/
		  
		Client(QObject *parent=0);
		~Client();
		void setUserId( const QString& userName );

		/**
		 * Start a connection to the server using the supplied @ref ClientStream.
		 * This is only a transport layer connection.
		 * Needed for protocol action P1.
		 * @param s initialised client stream to use for the connection.
		 * @param server the server to connect to - but this is also set on the connector used to construct the clientstream??
		 * @param auth indicate whether we're connecting to the authorizer or the bos server
		 */
		void connect( const QString &host, const uint port, const QString &userId, const QString &pass );

		/**
		 * Cancel active login attemps
		 */
		void cancelConnect();

		/**
		 * Logout and disconnect
		 */
		void close();

		/**
		 * Returns the errorcode
		 */
		int error();

		/**
		 * Returns a description of the error
		 */
		QString errorString();

		/**
		 * Specifies the status we connect with.
		 * May be Online or Invisible.
		 */
		void setStatusOnConnect( Yahoo::Status status );

		/**
		 * Accessors needed for login
		 */
		QString host();
		int port();

		/**
		 * Send a Typing notification
		 */
		void sendTyping( const QString &to, bool typing );
		
		/**
		 * Send a Message
		 */
		void sendMessage( const QString &to, const QString &msg );

		/**
		 * Send a Buzz
		 */
		void sendBuzz( const QString &to );

		/**
		 * Change our status
		 */	
		void changeStatus(Yahoo::Status status, const QString &message, Yahoo::StatusType type);

		/**
		 * Set the verification word that is needed for a account verification after
		 * too many wrong login attempts.
		 */
		void setVerificationWord( const QString &word );

		/**
		 * Add a buddy to the contact list
		 */
		void addBuddy( const QString &userId, const QString &group, const QString &message = QString::fromLatin1("Please add me")  );

		/**
		 * Remove a buddy from the contact list
		 */
		void removeBuddy( const QString &userId, const QString &group );

		/**
		 * Move a buddy into another group
		 */
		void moveBuddy( const QString &userId, const QString &oldGroup, const QString &newGroup );

		/**
		 * Change the stealth status of a buddy
		 */
		void stealthContact( const QString &userId, Yahoo::StealthStatus status );

		/**
		 * Request the buddy's picture
		 */
		void requestPicture( const QString &userId );

		/**
		 * Download the buddy's picture
		 */
		void downloadPicture( const QString &userId, KUrl url, int checksum );

		/**
		 * Send our picture
		 */
		void uploadPicture( KUrl url );

		/**
		 * Send checksum of our picture
		 */
		void sendPictureChecksum( int checksum, const QString & );

		/**
		 * Send information about our picture
		 */
		void sendPictureInformation( const QString &userId, const QString &url, int checksum );

		/**
		 * Notify the buddies about our new status
		 */
		void sendPictureStatusUpdate( const QString &userId, int type );

		/**
		 * Send a response to the webcam invite ( Accept / Decline )
		 */
		void requestWebcam( const QString &userId );

		/**
		 * Stop receiving of webcam
		 */
		void closeWebcam( const QString &userId );

		/**
		 * Invite the user to view your Webcam
		 */
		void sendWebcamInvite( const QString &userId );

		/**
		 * transmit a new image to the watchers
		 */
		void sendWebcamImage( const QByteArray &image );

		/**
		 * Stop transmission
		 */
		void closeOutgoingWebcam();

		/**
		 * Allow a buddy to watch the cam
		 */
		void grantWebcamAccess( const QString &userId );

		/**
		 * Invite buddies to a conference
		 */
		void inviteConference( const QString &room, const QStringList &members, const QString &msg );

		/**
		 * Invite buddies to a already existing conference
		 */
		void addInviteConference( const QString &room, const QStringList &members, const QString &msg );

		/**
		 * Join a conference
		 */
		void joinConference( const QString &room, const QStringList &members );

		/**
		 * Decline to join a conference
		 */
		void declineConference( const QString &room, const QStringList &members, const QString &msg );

		/**
		 * Leave the conference
		 */
		void leaveConference( const QString &room, const QStringList &members );

		/**
		 * Send a message to the conference
		 */
		void sendConferenceMessage( const QString &room, const QStringList &members, const QString &msg );

		/**
		 * Send a authorization request response
		 */
		void sendAuthReply( const QString &userId, bool accept, const QString &msg );

		/*************
		  INTERNAL (FOR USE BY TASKS) METHODS 
		 *************/
		/**
		 * Send an outgoing request to the server
		 */
		void send( Transfer *request );
		
		/**
		 * Print a debug statement
		 */
		void debug( const QString &str );
		
		/**
		 * The current user's user ID
		 */
		QString userId();
		
		/**
		 * The current user's password
		 */
		QString password();
		
		/**
		 * Host's IP address
		 */
		Q3CString ipAddress();
		
		/**
		 * current Session ID
		 */
		uint sessionID();
		
		/**
		 * return the pictureFlag describing the status of our buddy icon
		 * 0 = no icon, 2 = icon, 1 = avatar (?)
		 */
		int pictureFlag();

		/**
		 * Get our status
		 */
		Yahoo::Status status();

		/**
		 * Set our status
		 */
		void setStatus( Yahoo::Status );

		/**
		 * Access the root Task for this client, so tasks may be added to it.
		 */
		Task* rootTask();

		/**
		 * Accessors to the cookies
		 */
		QString yCookie();
		QString tCookie();
		QString cCookie();

		/**
		 * Error
		 */
		void notifyError( const QString & );
	signals:
		/** CONNECTION EVENTS */
		/**
		 * Notifies that the login process has succeeded.
		 */
		void loggedIn( int, const QString& );

		/** 
		 * Notifies that the login process has failed 
		 */
		void loginFailed();
		
		/**
		 * Notifies tasks and account so they can react properly
		 */
		void connected();
		/**
		 * Notifies tasks and account so they can react properly
		 */
		void disconnected();
		/**
		 * We were disconnected because we connected elsewhere
		 */
		void connectedElsewhere();
		/**
		 * Notifies about our buddies and groups
		 */
		void gotBuddy( const QString &, const QString &, const QString & );
		/**
		 * Notifies about the status of online buddies
		 */
		void statusChanged( const QString&, int, const QString&, int, int );
		/**
		 * Notifies about the stealth status of buddies
		 */
		void stealthStatusChanged( const QString &, Yahoo::StealthStatus );
		/**
		 * Notifies about mails
		 */
		void mailNotify( const QString&, const QString&, int );
		/**
		 * We got a new message
		 */
		void gotIm( const QString&, const QString&, long, int );
		/**
		 * We got a new system message
		 */
		void systemMessage( const QString& );
		/**
		 * The buddy is typing a message
		 */
		void typingNotify( const QString &, int );
		/**
		 * The buddy has invited us to view his webcam
		 */
		void gotWebcamInvite(const QString &);
		/**
		 * Notifies about a BUZZ notification
		 */
		void gotBuzz( const QString &, long );
		/**
		 * Notifies about a changed picture status
		 */
		void pictureStatusNotify( const QString &, int );
		/**
		 * Notifies about a picture checksum
		 */
		void pictureChecksumNotify( const QString &, int );
		/**
		 * Notifies about a picture
		 */
		void pictureInfoNotify( const QString &, KURL, int );
		/**
		 * The iconLoader has successfully downloaded a picutre
		 */
		void pictureDownloaded( const QString &, KTempFile *, int );
		/**
		 * A Buddy asks for our picture
		 */
		void pictureRequest( const QString & );
		/**
		 * Information about the picture upload
		 */
		void pictureUploaded( const QString & );
		/**
		 * We've received a webcam image from a buddy
		 */
		void webcamImageReceived( const QString &, const QPixmap &);
		/**
		 * The requested Webcam is not available
		 */
		void webcamNotAvailable( const QString & );
		/**
		 * The connection to the webcam was closed
		 */
		void webcamClosed( const QString &, int );
		/**
		 * The webcamtransmission is paused
		 */
		void webcamPaused(const QString&);
		/**
		 * The webcam connection is ready for transmission
		 */
		void webcamReadyForTransmission();
		/**
		 * The webcam should stop sending images
		 */
		void webcamStopTransmission();
		/**
		 * A new buddy watches the cam
		 */
		void webcamViewerJoined( const QString & );
		/**
		 * A buddy no longer watches the cam
		 */
		void webcamViewerLeft( const QString & );
		/**
		 * A buddy wants to watch the cam
		 */
		void webcamViewerRequest( const QString & );
		/**
		 * A buddy invited us to a conference
		 */
		void gotConferenceInvite( const QString &, const QString &, const QString &, const QStringList & );
		/**
		 * A conference message was received
		 */
		void gotConferenceMessage( const QString &, const QString &, const QString & );
		/**
		 * A buddy joined the conference
		 */
		void confUserJoined( const QString &, const QString & );
		/**
		 * A buddy left the conference
		 */
		void confUserLeft( const QString &, const QString & );
		/**
		 * A buddy declined to join the conference
		 */
		void confUserDeclined( const QString &, const QString &, const QString & );
		/**
		 * A buddy accepted our authorization request
		 */
		void authorizationAccepted( const QString & );
		/**
		 * A buddy rejected our authorization request
		 */
		void authorizationRejected( const QString &, const QString & );
		/**
		 * A buddy requests authorization
		 */
		void gotAuthorizationRequest( const QString &, const QString &, const QString & );
	protected slots:
		// INTERNAL, FOR USE BY TASKS' finished() SIGNALS //
		void lt_loginFinished();
		void lt_gotSessionID( uint );
		void cs_connected();
		void slotGotCookies();
		
		/**
		 * Used by tasks to identify a response to a login attempt
		 */
		void slotLoginResponse( int, const QString& );

		/**
		 * Used by the client stream to notify errors to upper layers.
		 */
		void streamError( int error );
		
		/**
		 * The client stream has data ready to read.
		 */
		void streamReadyRead();

		/**
		 * Send a Yahoo Ping packet to the server
		 */
		void sendPing();
	private:
		void distribute( Transfer *transfer );
		
		/**
		 * create static tasks and connect their signals
		 */
		void initTasks();

		/**
		 * remove static tasks and their singal connections
		 */
		void deleteTasks();

		class ClientPrivate;
		ClientPrivate* d;
		KNetworkConnector *m_connector;

		QTimer *m_pingTimer;
};

#endif
