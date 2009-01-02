/*
    dummyvideodevice.cpp  -  Dummy Video Device

    Copyright (c) 2009 by Alan Jones <skyphyr@gmail.com>

    Kopete    (c) 2002-2009      by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 3 of the License, or (at your option) any later version.      *
    *                                                                       *
    * This program is distributed in the hope that it will be useful,       *
    * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
    * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
    * GNU General Public License for more details.                          *
    *                                                                       *
    * You should have received a copy of the GNU General Public License     *
    * along with this program.  If not, see <http://www.gnu.org/licenses/>. *
    *************************************************************************
*/

#include "dummyvideodevice.h"
#include "videoinput.h"

namespace Kopete {

namespace AV {

DummyVideoDevice::DummyVideoDevice()
	{
	//need to set a bunch of defaults for members
	m_pixelformat = PIXELFORMAT_RGB32;
	m_driver=VIDEODEV_DRIVER_NONE;
	m_input.append(VideoInput());
	minwidth = 160;
	maxwidth = 1280;
	minheight = 120;
	maxheight = 960;
	setSize(320, 240);
	//let's be opened by default seeing this is a fallback 
	opened = true;
	}

int DummyVideoDevice::open()
	{
	//wow - we always succeed in opening our dummy device
	opened = true;
	return EXIT_SUCCESS;
	}
	
bool DummyVideoDevice::isOpen()
	{
	//we'll track it just incase there is situations where it's expected not to be open
	return opened;
	}
	
int DummyVideoDevice::checkDevice()
	{
	//again this is just incase anything ever relies on it failing when closed
	if(isOpen())
		{
		//need to check which, if any, of these need to be enabled for this to function properly
		m_videocapture=false;
		m_videochromakey=false;
		m_videoscale=false;
		m_videooverlay=false;
		m_videoread=false;
		m_videoasyncio=false;
		m_videostream=false;

		m_driver=VIDEODEV_DRIVER_NONE;
		return EXIT_SUCCESS;
		}
	return EXIT_FAILURE;
	}
	
int DummyVideoDevice::initDevice()
	{
	//again this is just incase anything ever relies on it failing when closed
	if(isOpen())
		{
		return EXIT_SUCCESS;
		}
	return EXIT_FAILURE;
	}
	
int DummyVideoDevice::setSize( int newwidth, int newheight)
	{
	if (isOpen())
		{
		if(newwidth  > maxwidth ) newwidth  = maxwidth;
		if(newheight > maxheight) newheight = maxheight;
		if(newwidth  < minwidth ) newwidth  = minwidth;
		if(newheight < minheight) newheight = minheight;

		currentwidth  = newwidth;
		currentheight = newheight;
		
		m_currentbuffer.width = currentwidth;
		m_currentbuffer.height = currentheight;
		m_currentbuffer.pixelformat = m_pixelformat;
		
		//let's fill the buffer up with something
		//TODO: replace this with an .svg that says there is no camera available
		m_currentbuffer.data.resize(currentwidth * currentheight * 4);
		for (int i=0; i<m_currentbuffer.data.size(); i++)
			{
			m_currentbuffer.data[i] = 255;
			}
		
		return EXIT_SUCCESS;
		}
	return EXIT_FAILURE;
	}
	
pixel_format DummyVideoDevice::setPixelFormat(pixel_format newformat)
	{
	pixel_format ret = PIXELFORMAT_NONE;
	
	if (newformat == PIXELFORMAT_RGB32)
		{
		m_pixelformat = newformat;
		ret = m_pixelformat;
		}
	
	return ret;
	}
	
int DummyVideoDevice::startCapturing()
	{
	//again this is just incase anything ever relies on it failing when closed
	if(isOpen())
		{
		return EXIT_SUCCESS;
		}
	return EXIT_FAILURE;
	}
	
int DummyVideoDevice::getFrame()
	{
	//again this is just incase anything ever relies on it failing when closed
	if(isOpen())
		{
		return EXIT_SUCCESS;
		}
	return EXIT_FAILURE;
	}
	
int DummyVideoDevice::getFrame(imagebuffer *imgbuffer)
	{
	if(imgbuffer)
	{
		imgbuffer->height      = m_currentbuffer.height;
		imgbuffer->width       = m_currentbuffer.width;
		imgbuffer->pixelformat = m_currentbuffer.pixelformat;
		imgbuffer->data        = m_currentbuffer.data;
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
	}
	
int DummyVideoDevice::getImage(QImage *qimage)
	{
	// do NOT delete qimage here, as it is received as a parameter
	if (qimage->width() != width() || qimage->height() != height())
		*qimage = QImage(width(), height(), QImage::Format_RGB32);

	uchar *bits=qimage->bits();
	memcpy(bits,&m_currentbuffer.data[0], m_currentbuffer.data.size());
	
	return EXIT_SUCCESS;
	}
	
int DummyVideoDevice::stopCapturing()
	{
	//again this is just incase anything ever relies on it failing when closed
	if(isOpen())
		{
		return EXIT_SUCCESS;
		}
	return EXIT_FAILURE;
	}
	
int DummyVideoDevice::close()
	{
	//we always manage to close it too - amazing :)
	opened = false;
	return EXIT_SUCCESS;
	}

}

}
