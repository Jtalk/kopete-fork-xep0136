/*
	xmlbackend.h

	Copyright (c) 2003-2004 by Olivier Goffart        <ogoffart@kde.org>

	Kopete    (c) 2003-2004 by the Kopete developers  <kopete-devel@kde.org>

	*************************************************************************
	*                                                                       *
	* This program is free software; you can redistribute it and/or modify  *
	* it under the terms of the GNU General Public License as published by  *
	* the Free Software Foundation; either version 2 of the License, or     *
	* (at your option) any later version.                                   *
	*                                                                       *
	*************************************************************************
*/

#ifndef XMLBACKEND_H
#define XMLBACKEND_H

#include <QtCore/QDate>
#include <QtCore/QList>
#include <QtXml/QDomDocument>

#include "kopetemessage.h"

#include "historybackend.h"

class QTimer;

class XMLBackend : public HistoryBackend
{
public:
	XMLBackend();
	virtual ~XMLBackend();

	void appendMessage( const Kopete::Message *msg );
	QList<Kopete::Message> readMessages( QDate date );

	QList<int> getDaysForMonth( QDate date, const QList<const Kopete::Contact*> &contacts );

private:
	static QString getFileName( const Kopete::Contact *contact, QDate date );

	QTimer *m_saveTimer;
	QDomDocument m_toSaveDocument;
	QString m_toSaveFileName;
	unsigned int m_saveTimerTime; //time in ms between each save

	void save();
};

#endif // XMLBACKEND_H
