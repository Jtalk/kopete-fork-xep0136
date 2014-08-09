/*
	historybackend.h

	Copyright (c) 2014 by Roman Nazarenko			  <me@jtalk.me>

	Kopete    (c) 2003-2014 by the Kopete developers  <kopete-devel@kde.org>

	*************************************************************************
	*                                                                       *
	* This program is free software; you can redistribute it and/or modify  *
	* it under the terms of the GNU General Public License as published by  *
	* the Free Software Foundation; either version 2 of the License, or     *
	* (at your option) any later version.                                   *
	*                                                                       *
	*************************************************************************
*/

#include "historybackend.h"

HistoryBackend::HistoryBackend()
{
}

HistoryBackend::~HistoryBackend()
{
}

void HistoryBackend::saveSlot()
{
	save();
}
