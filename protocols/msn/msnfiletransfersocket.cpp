/***************************************************************************
                          msnfiletransfersocket.cpp  -  description
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

#include "msnfiletransfersocket.h"

#include <math.h>

//qt
#include <qfile.h>
#include <qregexp.h>
#include <qtimer.h>

// kde
#include <kdebug.h>
#include <kextendedsocket.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <ksockaddr.h>

#include "kopetecontact.h"
#include "kopetetransfermanager.h"
#include "msnmessagemanager.h"
#include "msnswitchboardsocket.h"

MSNFileTransferSocket::MSNFileTransferSocket(const QString &handle, KopeteContact *c,bool incoming, QObject* parent)
	: MSNSocket(parent) , MSNInvitation(incoming, MSNFileTransferSocket::applicationID() , i18n("File Transfer"))
{
	m_handle=handle;
	m_kopeteTransfer=0l;
	m_file=0L;
	m_server=0L;
	m_contact=c;
	ready=true;

	QObject::connect( this, SIGNAL( socketClosed( int ) ), this, SLOT( slotSocketClosed( ) ) );
	QObject::connect( this, SIGNAL( blockRead( const QByteArray & ) ), this, SLOT(slotReadBlock( const QByteArray & ) ) );
}

MSNFileTransferSocket::~MSNFileTransferSocket()
{
	delete m_file;
	delete m_server;
	kdDebug(14140) << "MSNFileTransferSocket::~MSNFileTransferSocket" <<endl;
}

void MSNFileTransferSocket::parseCommand(const QString & cmd, uint id, const QString & data)
{
	if( cmd == "VER" )
	{
		if(data.section( ' ', 0, 0 ) != "MSNFTP")
		{
			kdDebug(14140) << "MSNFileTransferSocket::parseCommand (VER): bad version: disconnect" <<endl;
			disconnect();
		}
		else
		{
			if( m_incoming )
				sendCommand( "USR", m_handle + " " + m_authcook, false );
			else
				sendCommand( "VER", "MSNFTP" , false );
		}
	}
	else if( cmd == "FIL" )
	{
		m_size=id; //data.toUInt(); //BUG: the size is take as id bye MSNSocket because it is a number

		m_downsize=0;
		m_file=new QFile(m_fileName);

		if( m_file->open( IO_WriteOnly ))
			sendCommand( "TFR" ,NULL,false);
		else
		{
			kdDebug(14140) << "MSNFileTransferSocket::parseCommand: ERROR: unable to open file - disconnect " <<endl;
			disconnect();
		}
	}
	else if( cmd == "BYE" )
	{
		kdDebug(14140) << "MSNFileTransferSocket::parseCommand : end of transfer " <<endl;
		disconnect();
	}
	else if( cmd == "USR" )
	{
		if(data.section( ' ', 1, 1 )!= m_authcook)
		{
			kdDebug(14140) << "MSNFileTransferSocket::parseCommand (USR): bad auth" <<endl;
			disconnect();
		}
		else
			sendCommand("FIL" , QString::number(size()) , false);
	}
	else if( cmd == "TFR" )
	{
		m_downsize=0;
		ready=true;
		QTimer::singleShot( 0, this, SLOT(slotSendFile()) );
	}
	else if( cmd == "CCL" )
	{
		disconnect();
	}
	else
		kdDebug(14140) << "MSNFileTransferSocket::parseCommand : unknown command " <<cmd <<endl;

//	kdDebug(14140) << "MSNFileTransferSocket::parseCommand : done " <<cmd <<endl;
}

void MSNFileTransferSocket::doneConnect()
{
	if(m_incoming)
		sendCommand( "VER", "MSNFTP", false );
	MSNSocket::doneConnect();
}

void MSNFileTransferSocket::bytesReceived(const QByteArray & head)
{
	if(head[0]!='\0')
	{
		kdDebug(14140) << "MSNFileTransferSocket::bytesReceived: transfer aborted" <<endl;
		disconnect();
	}
	unsigned int sz=(int)((unsigned char)head.data()[2])*256+(int)((unsigned char)head.data()[1]);
//	kdDebug(14140) << "MSNFileTransferSocket::bytesReceived: " << sz <<endl;
	readBlock(sz);
}

void MSNFileTransferSocket::slotSocketClosed()
{
	kdDebug(14140) << "MSNFileTransferSocket::slotSocketClose "<<  endl;
	if(m_file)
		m_file->close();
	delete m_file;
	m_file=0L;
	delete m_server;
	m_server=0L;
	if(m_kopeteTransfer && (m_downsize!=m_size  || m_downsize==0 ) )
		m_kopeteTransfer->setError(KopeteTransfer::Other);
	emit done(this);
}

void MSNFileTransferSocket::slotReadBlock(const QByteArray &block)
{
	m_file->writeBlock( block.data(), block.size() );      // write to file
	m_downsize+=block.size();
	int percent=0;
	if(m_size!=0)   percent=100*m_downsize/m_size;

	if(m_kopeteTransfer) m_kopeteTransfer->slotPercentCompleted(percent);
	kdDebug(14140) << "MSNFileTransferSocket  -  " <<percent <<"% done"<<endl;

	if(m_downsize==m_size)
		sendCommand( "BYE" ,"16777989",false);
}

void MSNFileTransferSocket::setKopeteTransfer(KopeteTransfer *kt)
{
	m_kopeteTransfer=kt;
	if(kt)
		QObject::connect(kt , SIGNAL(transferCanceled()), this, SLOT(abort()));
}

void MSNFileTransferSocket::listen(int port)
{
	m_server = new KExtendedSocket();

	QObject::connect( m_server, SIGNAL(readyAccept()), this,  SLOT(slotAcceptConnection()));
	m_server->setPort(port);
	m_server->setSocketFlags(  KExtendedSocket::noResolve
                            | KExtendedSocket::passiveSocket
                            | KExtendedSocket::anySocket);
	int listenResult = m_server->listen(1);

	kdDebug(14140) << "MSNFileTransferSocket::listen: result: "<<  listenResult <<endl;
	m_server->setBlockingMode(true);
	QTimer::singleShot( 60000, this, SLOT(slotTimer()) );
	kdDebug(14140) << "MSNFileTransferSocket::listen done" <<endl;
}

void MSNFileTransferSocket::slotAcceptConnection()
{
	kdDebug(14140) << "MSNFileTransferSocket::slotAcceptConnection" <<endl;
	if(!accept(m_server))
	{
		if( m_kopeteTransfer)
			m_kopeteTransfer->setError(KopeteTransfer::Other);
		emit done(this);
	}
}

void MSNFileTransferSocket::slotTimer()
{
	if(onlineStatus() != Disconnected)
		return;
	kdDebug(14140) << "MSNFileTransferSocket::slotTimer: timeout "<<  endl;
	if( m_kopeteTransfer)
	{
		m_kopeteTransfer->setError(KopeteTransfer::Timeout);
	}

	MSNMessageManager* manager=dynamic_cast<MSNMessageManager*>(m_contact->manager());
	if(manager && manager->service())
		manager->service()->sendCommand( "MSG" , "N", true, rejectMessage("TIMEOUT") );

	emit done(this);
}

void MSNFileTransferSocket::abort()
{
	if(m_incoming)
		sendCommand( "CCL" , NULL ,false);
	disconnect();
//	emit done(this);
}

void MSNFileTransferSocket::setFile( const QString &fn, long unsigned int fileSize )
{
	m_fileName=fn;
	if(!m_incoming)
	{
		if(m_file)
		{
			kdDebug(14140) << "MSNFileTransferSocket::setFileName: WARNING m_file already exists" << endl;
			delete m_file;
		}
		m_file = new QFile( fn );
		if(!m_file->open(IO_ReadOnly))
		{
			//FIXME: abort transfer here
			kdDebug(14140) << "MSNFileTransferSocket::setFileName: WARNING unable to open the file" << endl;
		}

		//If the fileSize is 0 it was not given, we are to get it from the file
		if(fileSize == 0L)
			m_size = m_file->size();
		else
			m_size = fileSize;
	}
}


void MSNFileTransferSocket::slotSendFile()
{
//	kdDebug(14140) <<"MSNFileTransferSocket::slotSendFile()" <<endl;
	if( m_downsize >= m_size)
		return;

	if(ready)
	{
		char data[2046];
		int bytesRead = m_file->readBlock( data, 2045 );
			
		QByteArray block(bytesRead+3);
//		char i1= (char)fmod( bytesRead, 256 ) ;
//		char i2= (char)floor( bytesRead / 256 ) ;
//		kdDebug(14140) << "MSNFileTransferSocket::slotSendFile: " << (int)i1 <<" + 256* "<< (int)i2 <<" = " << bytesRead <<endl;
		block[0]='\0';
		block[1]= (char)fmod( bytesRead, 256 );
		block[2]= (char)floor( bytesRead / 256 );

		for (  int f = 0; f < bytesRead; f++ )
		{
			block[f+3] = data[f];
		}

		sendBytes(block);

		int percent=0;
		m_downsize+=bytesRead;
		if(m_size!=0)   percent=100*m_downsize/m_size;
		if(m_kopeteTransfer)
			 m_kopeteTransfer->slotPercentCompleted(percent);
		kdDebug(14140) << "MSNFileTransferSocket::slotSendFile:  " <<percent <<"% done"<<endl;
	}
	ready=false;

	QTimer::singleShot( 10, this, SLOT(slotSendFile()) );
}

void MSNFileTransferSocket::slotReadyWrite()
{
	ready=true;
	MSNSocket::slotReadyWrite();
}

QCString MSNFileTransferSocket::invitationHead()
{
	QTimer::singleShot( 10 * 60000, this, SLOT(slotTimer()) );  //the user has 10 mins to accept or refuse or initiate the transfer

	return QString( MSNInvitation::invitationHead()+
					"Application-File: "+ m_fileName.right( m_fileName.length() - m_fileName.findRev( QRegExp("/") ) - 1 ) +"\r\n"
					"Application-FileSize: "+ QString::number(size()) +"\r\n\r\n").utf8();
}

void MSNFileTransferSocket::parseInvitation(const QString& msg)
{
	QRegExp rx("Invitation-Command: ([A-Z]*)");
	rx.search(msg);
	QString command=rx.cap(1);
	if( msg.contains("Invitation-Command: INVITE") )
	{
		rx=QRegExp("Application-File: ([^\\r\\n]*)");
		rx.search(msg);
		QString filename = rx.cap(1);
		rx=QRegExp("Application-FileSize: ([0-9]*)");
		rx.search(msg);
		unsigned long int filesize= rx.cap(1).toUInt();

		MSNInvitation::parseInvitation(msg); //for the cookie

		KopeteTransferManager::transferManager()->askIncomingTransfer( m_contact , filename, filesize, QString::null, QString::number( cookie() ) );

		QObject::connect( KopeteTransferManager::transferManager(), SIGNAL( accepted( KopeteTransfer *, const QString& ) ),this, SLOT( slotFileTransferAccepted( KopeteTransfer *, const QString& ) ) );
		QObject::connect( KopeteTransferManager::transferManager(), SIGNAL( refused( const KopeteFileTransferInfo & ) ), this, SLOT( slotFileTransferRefused( const KopeteFileTransferInfo & ) ) );

	}
	else if( msg.contains("Invitation-Command: ACCEPT") )
	{
		if(incoming())
		{
			rx=QRegExp("IP-Address: ([0-9\\.]*)");
			rx.search(msg);
			QString ip_adress = rx.cap(1);
			rx=QRegExp("AuthCookie: ([0-9]*)");
			rx.search(msg);
			QString authcook = rx.cap(1);
			rx=QRegExp("Port: ([0-9]*)");
			rx.search(msg);
			QString port = rx.cap(1);

			setAuthCookie(authcook);
			connect(ip_adress, port.toUInt());
		}
		else
		{
			unsigned long int auth = (rand()%(999999))+1;
			setAuthCookie(QString::number(auth));

			setKopeteTransfer(KopeteTransferManager::transferManager()->addTransfer(m_contact, fileName(), size(),  m_contact->displayName(), KopeteFileTransferInfo::Outgoing));

			MSNMessageManager* manager=dynamic_cast<MSNMessageManager*>(m_contact->manager());
			if(manager && manager->service())
			{
				QCString message=QString(
					"MIME-Version: 1.0\r\n"
					"Content-Type: text/x-msmsgsinvite; charset=UTF-8\r\n"
					"\r\n"
					"Invitation-Command: ACCEPT\r\n"
					"Invitation-Cookie: " + QString::number(cookie()) + "\r\n"
					"IP-Address: " + manager->service()->getLocalIP() + "\r\n"
					"Port: 6891\r\n"
					"AuthCookie: "+QString::number(auth)+"\r\n"
					"Launch-Application: FALSE\r\n"
					"Request-Data: IP-Address:\r\n\r\n").utf8();

				manager->service()->sendCommand( "MSG" , "N", true, message );
			}

			listen(6891);
		}
	}
	else //CANCEL
	{
		MSNInvitation::parseInvitation(msg);
		if( m_kopeteTransfer)
			m_kopeteTransfer->setError(KopeteTransfer::CanceledRemote);
		emit done(this);

	}
}

void MSNFileTransferSocket::slotFileTransferAccepted(KopeteTransfer *trans, const QString& fileName)
{
 	if(trans->info().internalId().toULong() != cookie())
		return;

	if(!trans->info().contact())
		return;

	setKopeteTransfer(trans);

	MSNMessageManager* manager=dynamic_cast<MSNMessageManager*>(m_contact->manager());

	if(manager && manager->service())
	{
		setFile(fileName);

		QCString message=QString(
			"MIME-Version: 1.0\r\n"
			"Content-Type: text/x-msmsgsinvite; charset=UTF-8\r\n"
			"\r\n"
			"Invitation-Command: ACCEPT\r\n"
			"Invitation-Cookie: " + QString::number(cookie()) + "\r\n"
			"Launch-Application: FALSE\r\n"
			"Request-Data: IP-Address:\r\n"  ).utf8();
		manager->service()->sendCommand( "MSG" , "N", true, message );

		QTimer::singleShot( 3 * 60000, this, SLOT(slotTimer()) ); //if after 3 minutes the transfer has not begin, delete this
	}
	else
	{
		if( m_kopeteTransfer)
			m_kopeteTransfer->setError(KopeteTransfer::Other);
		emit done(this);

	}
}

void MSNFileTransferSocket::slotFileTransferRefused(const KopeteFileTransferInfo &info)
{
	if(info.internalId().toULong() != cookie())
		return;

	if(!info.contact())
		return;

	MSNMessageManager* manager=dynamic_cast<MSNMessageManager*>(m_contact->manager());

	if(manager && manager->service())
	{
		manager->service()->sendCommand( "MSG" , "N", true, rejectMessage() );
	}

	emit done(this);
}



#include "msnfiletransfersocket.moc"

