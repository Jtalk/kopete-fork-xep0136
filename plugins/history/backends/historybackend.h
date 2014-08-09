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

#ifndef HISTORYBACKEND_H
#define HISTORYBACKEND_H

class HistoryBackend
{
public:
	HistoryBackend();
	virtual ~HistoryBackend();

	virtual void appendMessage( const Kopete::Message *msg ) = 0;
	virtual QList<Kopete::Message> readMessages( QDate date ) = 0;

protected:
	virtual void save() = 0;

protected slots:
	void saveSlot();
};

#endif // HISTORYBACKEND_H