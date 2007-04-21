/*
    yahoowebcam.cpp - Send webcam images

    Copyright (c) 2005 by André Duffec <andre.duffeck@kdemail.net>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include <kdebug.h>
#include <k3process.h>
#include <ktemporaryfile.h>
#include <qtimer.h>

#include "client.h"
#include "yahoowebcam.h"
#include "yahooaccount.h"
#include "yahoowebcamdialog.h"
#include "avdevice/videodevicepool.h"


YahooWebcam::YahooWebcam( YahooAccount *account ) : QObject( 0 )
{
	setObjectName( QLatin1String("yahoo_webcam") );
	kDebug(YAHOO_GEN_DEBUG) << k_funcinfo << endl;
	theAccount = account;
	theDialog = 0L;
	origImg = new KTemporaryFile();
	origImg->setAutoRemove(false);
	origImg->open();
	convertedImg = new KTemporaryFile();
	convertedImg->setAutoRemove(false);
	convertedImg->open();
	m_img = new QImage();

	m_sendTimer = new QTimer( this );
	connect( m_sendTimer, SIGNAL(timeout()), this, SLOT(sendImage()) );	

	m_updateTimer = new QTimer( this );
	connect( m_updateTimer, SIGNAL(timeout()), this, SLOT(updateImage()) );	

	theDialog = new YahooWebcamDialog( "YahooWebcam" );
	connect( theDialog, SIGNAL(closingWebcamDialog()), this, SLOT(webcamDialogClosing()) );
#ifndef Q_OS_WIN
	m_devicePool = Kopete::AV::VideoDevicePool::self();
	m_devicePool->open();
	m_devicePool->setSize(320, 240);
	m_devicePool->startCapturing();
#endif
	m_updateTimer->start( 250 );
}

YahooWebcam::~YahooWebcam()
{
	QFile::remove( origImg->fileName() );
	QFile::remove( convertedImg->fileName() );
	delete origImg;
	delete convertedImg;
	delete m_img;
}

void YahooWebcam::stopTransmission()
{
	m_sendTimer->stop();
}

void YahooWebcam::startTransmission()
{
	m_sendTimer->start( 1000 );
}

void YahooWebcam::webcamDialogClosing()
{
	m_sendTimer->stop();
	theDialog->delayedDestruct();
	emit webcamClosing();
#ifndef Q_OS_WIN
	m_devicePool->stopCapturing(); 
	m_devicePool->close();
#endif
}

void YahooWebcam::updateImage()
{
#ifndef Q_OS_WIN
	m_devicePool->getFrame();
	m_devicePool->getImage(m_img);
#endif
	theDialog->newImage( QPixmap::fromImage(*m_img) );
}

void YahooWebcam::sendImage()
{
	kDebug(YAHOO_GEN_DEBUG) << k_funcinfo << endl;

#ifndef Q_OS_WIN
	m_devicePool->getFrame();
	m_devicePool->getImage(m_img);
#endif
	
	origImg->close();
	convertedImg->close();
	
	m_img->save( origImg->fileName(), "JPEG");
	
	K3Process p;
	p << "jasper";
	p << "--input" << origImg->fileName() << "--output" << convertedImg->fileName() << "--output-format" << "jpc" << "-O" <<"cblkwidth=64\ncblkheight=64\nnumrlvls=4\nrate=0.0165\nprcheight=128\nprcwidth=2048\nmode=real";
	
	
	p.start( K3Process::Block );
	if( p.exitStatus() != 0 )
	{
		kDebug(YAHOO_GEN_DEBUG) << " jasper exited with status " << p.exitStatus() << endl;
	}
	else
	{
		QFile file( convertedImg->fileName() );
		if( file.open( QIODevice::ReadOnly ) )
		{
			QByteArray ar = file.readAll();
			theAccount->yahooSession()->sendWebcamImage( ar );
		}
		else
			kDebug(YAHOO_GEN_DEBUG) << "Error opening the converted webcam image." << endl;
	}
}

void YahooWebcam::addViewer( const QString &viewer )
{
	m_viewer.push_back( viewer );
	if( theDialog )
		theDialog->setViewer( m_viewer );
}

void YahooWebcam::removeViewer( const QString &viewer )
{
	m_viewer.removeAll( viewer );
	if( theDialog )
		theDialog->setViewer( m_viewer );
}

#include "yahoowebcam.moc"
