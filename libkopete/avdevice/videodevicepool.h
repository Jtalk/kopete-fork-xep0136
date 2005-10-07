/*
    videodevicepool.h  -  Kopete Multiple Video Device handler Class

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

#ifndef KOPETE_AVVIDEODEVICE_H
#define KOPETE_AVVIDEODEVICE_H

#include <q3valuevector.h>
#include <iostream>


#include "videoinput.h"
#include <qstring.h>
#include <qimage.h>
#include <q3valuevector.h>
#include <kcombobox.h>
#include "videodevice.h"
#include "kopete_export.h"

namespace Kopete {

namespace AV {

/**
This class allows kopete to check for the existence, open, configure, test, set parameters, grab frames from and close a given video capture card using the Video4Linux API.

@author Cl�udio da Silveira Pinheiro
*/
struct VideoDeviceModel
{
	QString name;
	size_t count;
};

typedef Q3ValueVector<Kopete::AV::VideoDevice> VideoDeviceVector;
typedef Q3ValueVector<VideoDeviceModel> VideoDeviceModelVector;

class VideoDevicePoolPrivate;

class KOPETE_EXPORT VideoDevicePool
{
public:
	static VideoDevicePool* self();
	int open();
	int open(unsigned int device);
	bool isOpen();
	int getFrame();
	int width();
	int minWidth();
	int maxWidth();
	int height();
	int minHeight();
	int maxHeight();
	int setSize( int newwidth, int newheight);
	int close();
	int startCapturing();
	int stopCapturing();
	int readFrame();
	int getImage(QImage *qimage);
	int selectInput(int newinput);
	int scanDevices();
	~VideoDevicePool();
	VideoDeviceVector m_videodevice;
	VideoDeviceModelVector m_model;
	int fillDeviceKComboBox(KComboBox *combobox);
	int fillInputKComboBox(KComboBox *combobox);
	unsigned int currentDevice();
	int currentInput();
	unsigned int inputs();
	bool getAutoColorCorrection();
	bool setAutoColorCorrection(bool colorcorrection);
	void loadConfig(); // Load configuration parameters;
	void saveConfig(); // Save configuretion parameters;

protected:
	int xioctl(int request, void *arg);
	int errnoReturn(const char* s);
	int showDeviceCapabilities(unsigned int device);
	void guessDriver();
	unsigned int m_current_device;
	struct imagebuffer m_buffer; // only used when no devices were found
private:
	VideoDevicePool();
	static VideoDevicePool* s_self;
};

}

}

#endif
