/*
    videodevice.cpp  -  Kopete Video Device Low-level Support

    Copyright (c) 2005 by Cl�udio da Silveira Pinheiro   <taupter@gmail.com>

    Kopete    (c) 2002-2003      by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include <assert.h>
#include <iostream>
#include <ostream>
#include <sstream>
#include <cstdlib>
#include <cerrno>
#include <cstring>

#include <kdebug.h>
#include <klocale.h>
#include <qdir.h>

#include "videodevice.h"
#include "videodevicepool.h"

#define CLEAR(x) memset (&(x), 0, sizeof (x))

namespace Kopete {

namespace AV {

VideoDevicePool *VideoDevicePool::s_self = NULL;

VideoDevicePool* VideoDevicePool::self()
{
//	kdDebug() << "libkopete (avdevice): self() called" << endl;
	if (s_self == NULL)
	{
		s_self = new VideoDevicePool;
	}
//	kdDebug() << "libkopete (avdevice): self() exited successfuly" << endl;
	return s_self;
}

VideoDevicePool::VideoDevicePool()
{
}


VideoDevicePool::~VideoDevicePool()
{
}




/*!
    \fn VideoDevicePool::open()
 */
int VideoDevicePool::open()
{
    /// @todo implement me

	return m_videodevice[currentDevice()].open();
}

/*!
    \fn VideoDevicePool::open(int device)
 */
int VideoDevicePool::open(unsigned int device)
{
    /// @todo implement me
	kdDebug() << "libkopete (avdevice): open(" << device << ") called." << endl;
	if(device >= m_videodevice.size())
	{
		kdDebug() << "libkopete (avdevice): open(" << device <<"): Device does not exist." << endl;
		return EXIT_FAILURE;
	}
	close();
	kdDebug() << "libkopete (avdevice): open(" << device << ") Setting m_current_Device to " << device << endl;
	m_current_device = device;
	kdDebug() << "libkopete (avdevice): open(" << device << ") Calling open()." << endl;
	open();
//	m_videodevice[currentDevice()].initDevice();
	kdDebug() << "libkopete (avdevice): open(" << device << ") exited successfuly." << endl;
	return EXIT_SUCCESS;
}

bool VideoDevicePool::isOpen()
{
	return m_videodevice[currentDevice()].isOpen();
}

/*!
    \fn VideoDevicePool::processImage(const void *p)
 */
int VideoDevicePool::processImage(const void * p)
{
	return m_videodevice[currentDevice()].processImage(p);
}

/*!
    \fn VideoDevicePool::showDeviceCapabilities(int device)
 */
int VideoDevicePool::showDeviceCapabilities(unsigned int device)
{
	return m_videodevice[device].showDeviceCapabilities();
}

int VideoDevicePool::width()
{
	return m_videodevice[currentDevice()].width();
}

int VideoDevicePool::minWidth()
{
	return m_videodevice[currentDevice()].minWidth();
}

int VideoDevicePool::maxWidth()
{
	return m_videodevice[currentDevice()].maxWidth();
}

int VideoDevicePool::height()
{
	return m_videodevice[currentDevice()].height();
}

int VideoDevicePool::minHeight()
{
	return m_videodevice[currentDevice()].minHeight();
}

int VideoDevicePool::maxHeight()
{
	return m_videodevice[currentDevice()].maxHeight();
}

int VideoDevicePool::setSize( int newwidth, int newheight)
{
	if(m_videodevice.size())
		return m_videodevice[currentDevice()].setSize(newwidth, newheight);
	else
	{
		kdDebug() << "libkopete (avdevice): VideoDevicePool::setSize() fallback for no device." << endl;
		m_buffer.width=newwidth;
		m_buffer.height=newheight;
		m_buffer.pixelformat=	PIXELFORMAT_RGB24;
		m_buffer.data.resize(m_buffer.width*m_buffer.height*3);
		m_buffer.size=m_buffer.data.size();
		kdDebug() << "libkopete (avdevice): VideoDevicePool::setSize() buffer size: "<< m_buffer.size << endl;
	}
	return EXIT_SUCCESS;
}

/*!
    \fn VideoDevicePool::close()
 */
int VideoDevicePool::close()
{
    /// @todo implement me
	if(currentDevice() < m_videodevice.size())
		return m_videodevice[currentDevice()].close();
	kdDebug() << "libkopete (avdevice): VideoDevicePool::close() Current device out of range." << endl;
	return EXIT_FAILURE;
}

/*!
    \fn VideoDevicePool::startCapturing()
 */
int VideoDevicePool::startCapturing()
{
	if(m_videodevice.size())
		return m_videodevice[currentDevice()].startCapturing();
}


/*!
    \fn VideoDevicePool::stopCapturing()
 */
int VideoDevicePool::stopCapturing()
{
	if(m_videodevice.size())
		return m_videodevice[currentDevice()].stopCapturing();
}


/*!
    \fn VideoDevicePool::getFrame()
 */
int VideoDevicePool::getFrame()
{
	kdDebug() << "libkopete (avdevice): VideoDevicePool::getFrame() called." << endl;
	if(m_videodevice.size())
		return m_videodevice[currentDevice()].getFrame();
	else
	{
		kdDebug() << "libkopete (avdevice): VideoDevicePool::getFrame() fallback for no device." << endl;
		for(int loop=0; loop < m_buffer.size; loop+=3)
		{
			m_buffer.data[loop]   = 255;
			m_buffer.data[loop+1] = 0;
			m_buffer.data[loop+2] = 0;
		}
	}
	kdDebug() << "libkopete (avdevice): VideoDevicePool::getFrame() exited successfuly." << endl;
	return EXIT_SUCCESS;
}

/*!
    \fn VideoDevicePool::getQImage(QImage *qimage)
 */
int VideoDevicePool::getImage(QImage *qimage)
{
	kdDebug() << "libkopete (avdevice): VideoDevicePool::getImage() called." << endl;
	if(m_videodevice.size())
		return m_videodevice[currentDevice()].getImage(qimage);
	else
	{
		kdDebug() << "libkopete (avdevice): VideoDevicePool::getImage() fallback for no device." << endl;
		qimage->create(m_buffer.width, m_buffer.height,32, QImage::IgnoreEndian);
		uchar *bits=qimage->bits();
		switch(m_buffer.pixelformat)
		{
			case PIXELFORMAT_NONE	: break;
			case PIXELFORMAT_GREY	: break;
			case PIXELFORMAT_RGB332	: break;
			case PIXELFORMAT_RGB555	: break;
			case PIXELFORMAT_RGB555X: break;
			case PIXELFORMAT_RGB565	: break;
			case PIXELFORMAT_RGB565X: break;
			case PIXELFORMAT_RGB24	:
				{
					kdDebug() << "libkopete (avdevice): VideoDevicePool::getImage() fallback for no device - RGB24." << endl;
					int step=0;
					for(unsigned int loop=0;loop < qimage->numBytes();loop+=4)
					{
						bits[loop]   = m_buffer.data[step];
						bits[loop+1] = m_buffer.data[step+1];
						bits[loop+2] = m_buffer.data[step+2];
						bits[loop+3] = 255;
						step+=3;
					}
				}
				break;
			case PIXELFORMAT_BGR24	: break;
				{
					int step=0;
					for(unsigned int loop=0;loop < qimage->numBytes();loop+=4)
					{
						bits[loop]   = m_buffer.data[step+2];
						bits[loop+1] = m_buffer.data[step+1];
						bits[loop+2] = m_buffer.data[step];
						bits[loop+3] = 255;
						step+=3;
					}
				}
				break;
			case PIXELFORMAT_RGB32	: memcpy(bits,&m_buffer.data[0], m_buffer.data.size());
				break;
			case PIXELFORMAT_BGR32	: break;
		}
	}
	kdDebug() << "libkopete (avdevice): VideoDevicePool::getImage() exited successfuly." << endl;
	return EXIT_SUCCESS;
}

/*!
    \fn Kopete::AV::VideoDevicePool::selectInput(int input)
 */
int VideoDevicePool::selectInput(int newinput)
{
	kdDebug() << "libkopete (avdevice): VideoDevicePool::selectInput(" << newinput << ") called." << endl;
	if(m_videodevice.size())
		return m_videodevice[currentDevice()].selectInput(newinput);
	else
		return 0;
}

/*!
    \fn Kopete::AV::VideoDevicePool::fillInputKComboBox(KComboBox *combobox)
 */
int VideoDevicePool::fillDeviceKComboBox(KComboBox *combobox)
{
    /// @todo implement me
	kdDebug() << "libkopete (avdevice): fillInputKComboBox: Called." << endl;
	combobox->clear();
	if(m_videodevice.size())
	{
		for (unsigned int loop=0; loop < m_videodevice.size(); loop++)
		{
			combobox->insertItem(m_videodevice[loop].name);
			kdDebug() << "libkopete (avdevice): DeviceKCombobox: Added device " << loop << ": " << m_videodevice[loop].name << endl;
		}
		combobox->setCurrentItem(currentDevice());
	}
	return EXIT_SUCCESS;
}

/*!
    \fn Kopete::AV::VideoDevicePool::fillInputKComboBox(KComboBox *combobox)
 */
int VideoDevicePool::fillInputKComboBox(KComboBox *combobox)
{
    /// @todo implement me
	kdDebug() << "libkopete (avdevice): fillInputKComboBox: Called." << endl;
	combobox->clear();
	if(m_videodevice.size())
	{
		if(m_videodevice[currentDevice()].inputs()>0)
			for (unsigned int loop=0; loop < m_videodevice[currentDevice()].inputs(); loop++)
			{
				combobox->insertItem(m_videodevice[currentDevice()].input[loop].name);
				kdDebug() << "libkopete (avdevice): InputKCombobox: Added input " << loop << ": " << m_videodevice[currentDevice()].input[loop].name << " (tuner: " << m_videodevice[currentDevice()].input[loop].hastuner << ")" << endl;
			}
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

/*!
    \fn Kopete::AV::VideoDevicePool::scanDevices()
 */
int VideoDevicePool::scanDevices()
{
    /// @todo implement me

	kdDebug() << "libkopete (avdevice): scanDevices() called" << endl;
#ifdef __linux__
	QDir videodevice_dir;
	const QString videodevice_dir_path=QString::fromLocal8Bit("/dev/v4l/");
	const QString videodevice_dir_filter=QString::fromLocal8Bit("video*");
	VideoDevice videodevice;

	videodevice_dir.setPath(videodevice_dir_path);
	videodevice_dir.setNameFilter(videodevice_dir_filter);
        videodevice_dir.setFilter( QDir::System | QDir::NoSymLinks | QDir::Readable | QDir::Writable );
        videodevice_dir.setSorting( QDir::Name );

	const QFileInfoList list = videodevice_dir.entryInfoList();

	QFileInfoList::ConstIterator fileinfoiterator = list.constBegin();

	m_videodevice.clear();
	kdDebug() << "libkopete (avdevice): scanDevices() called" << endl;
	while ( fileinfoiterator != list.constEnd() )
	{
		videodevice.setFileName(fileinfoiterator->absFilePath());
		kdDebug() << "libkopete (avdevice): Found device " << videodevice.full_filename << endl;
		videodevice.open(); // It should be opened with O_NONBLOCK (it's a FIFO) but I dunno how to do it using QFile
		if(videodevice.isOpen())
		{
			kdDebug() << "libkopete (avdevice): File " << videodevice.full_filename << " was opened successfuly" << endl;
			videodevice.close();
			m_videodevice.push_back(videodevice);
		}
		++fileinfoiterator;
	}
#endif
	kdDebug() << "libkopete (avdevice): scanDevices() exited successfuly" << endl;
	return EXIT_SUCCESS;
}

/*!
    \fn Kopete::AV::VideoDevicePool::currentDevice()
 */
unsigned int VideoDevicePool::currentDevice()
{
    /// @todo implement me
	return m_current_device;
}

/*!
    \fn Kopete::AV::VideoDevicePool::currentInput()
 */
int VideoDevicePool::currentInput()
{
    /// @todo implement me
	return m_videodevice[currentDevice()].currentInput();
}

/*!
    \fn Kopete::AV::VideoDevicePool::currentInput()
 */
unsigned int VideoDevicePool::inputs()
{
    /// @todo implement me
	return m_videodevice[currentDevice()].inputs();
}



}

}
