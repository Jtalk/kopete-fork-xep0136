/*
	xmlbackend.cpp

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

#include "xmlbackend.h"

XMLBackend::XMLBackend()
	: HistoryBackend()
{
	m_saveTimer = 0L;
	m_saveTimerTime = 0;
}

XMLBackend::~XMLBackend()
{
	if (m_saveTimer && m_saveTimer->isActive())
		saveToDisk();
}

void XMLBackend::appendMessage(const Kopete::Message *msg)
{
	QDate date = msg.timestamp().date();

	QDomDocument doc = getDocument(contact, QDate::currentDate().month() - date.month() - (QDate::currentDate().year() - date.year()) * 12);
	QDomElement docElem = doc.documentElement();

	if (docElem.isNull())
	{
		docElem= doc.createElement( "kopete-history" );
		docElem.setAttribute ( "version" , "0.9" );
		doc.appendChild( docElem );
		QDomElement headElem = doc.createElement( "head" );
		docElem.appendChild( headElem );
		QDomElement dateElem = doc.createElement( "date" );
		dateElem.setAttribute( "year",  QString::number(date.year()) );
		dateElem.setAttribute( "month", QString::number(date.month()) );
		headElem.appendChild( dateElem );
		QDomElement myselfElem = doc.createElement( "contact" );
		myselfElem.setAttribute( "type",  "myself" );
		myselfElem.setAttribute( "contactId", contact->account()->myself()->contactId() );
		headElem.appendChild( myselfElem );
		QDomElement contactElem = doc.createElement( "contact" );
		contactElem.setAttribute( "contactId", contact->contactId() );
		headElem.appendChild( contactElem );
	}

	QDomElement msgElem = doc.createElement( "msg" );
	msgElem.setAttribute( "in",  msg.direction() == Kopete::Message::Outbound ? "0" : "1" );
	msgElem.setAttribute( "from",  msg.from()->contactId() );
	msgElem.setAttribute( "nick",  msg.from()->displayName() ); //do we have to set this?
	msgElem.setAttribute( "time", msg.timestamp().toString("d h:m:s") );

	QDomText msgNode;

	if ( msg.format() != Qt::PlainText )
		msgNode = doc.createTextNode( msg.escapedBody() );
	else
		msgNode = doc.createTextNode( Qt::escape(msg.plainBody()).replace('\n', "<br />") );

	docElem.appendChild( msgElem );
	msgElem.appendChild( msgNode );

	// I'm temporizing the save.
	// On hight-traffic channel, saving can take lots of CPU. (because the file is big)
	// So i wait a time proportional to the time needed to save..

	const QString filename = getFileName( contact, date );
	if (!m_toSaveFileName.isEmpty() && m_toSaveFileName != filename)
	{
		//that mean the contact or the month has changed, save it now.
		save();
	}

	m_toSaveFileName = filename;
	m_toSaveDocument = doc;

	if (!m_saveTimer)
	{
		m_saveTimer = new QTimer(this);

		connect( m_saveTimer, SIGNAL(timeout()),
			this, SLOT(saveSlot()) );
	}

	if (!m_saveTimer->isActive())
	{
		m_saveTimer->setSingleShot( true );
		m_saveTimer->start( m_saveTimerTime );
	}
}

void XMLBackend::save()
{
	if (m_saveTimer)
		m_saveTimer->stop();
	if (m_toSaveFileName.isEmpty() || m_toSaveDocument.isNull())
		return;

	QTime t;
	t.start(); //mesure the time needed to save.

	KSaveFile file( m_toSaveFileName );
	if ( file.open() )
	{
		QTextStream stream( &file );
		//stream.setEncoding( QTextStream::UnicodeUTF8 ); //???? oui ou non?
		m_toSaveDocument.save( stream, 1 );
		file.finalize();

		m_saveTimerTime = qMin( t.elapsed()*1000, 300000 );
			//a time 1000 times supperior to the time needed to save.  but with a upper limit of 5 minutes
		//on a my machine, (2.4Ghz, but old HD) it should take about 10 ms to save the file.
		// So that would mean save every 10 seconds, which seems to be ok.
		// But it may take 500 ms if the file to save becomes too big (1Mb).
		kDebug(14310) << m_toSaveFileName << " saved in " << t.elapsed() << " ms ";

		m_toSaveFileName.clear();
		m_toSaveDocument = QDomDocument();
	}
	else
		kError(14310) << "impossible to save the history file " << m_toSaveFileName << endl;
}

QList<Kopete::Message> XMLBackend::readMessages( QDate date )
{
	QRegExp rxTime( "(\\d+) (\\d+):(\\d+)($|:)(\\d*)" ); //(with a 0.7.x compatibility)
	QList<Kopete::Message> messages;
	QList<Kopete::Contact*> ct = m_metaContact->contacts();

	foreach(Kopete::Contact* contact, ct)
	{
		QDomDocument doc = getDocument( contact,date, true, 0L );
		QDomElement docElem = doc.documentElement();
		QDomNode n = docElem.firstChild();

		while(!n.isNull())
		{
			QDomElement  msgElem2 = n.toElement();
			if ( !msgElem2.isNull() && msgElem2.tagName() == "msg")
			{
				rxTime.indexIn( msgElem2.attribute( "time" ) );
				QDateTime dt( QDate(date.year() , date.month() , rxTime.cap(1).toUInt()), QTime( rxTime.cap(2).toUInt() , rxTime.cap(3).toUInt(), rxTime.cap(5).toUInt()  ) );

				if (dt.date() != date)
				{
					n = n.nextSibling();
					continue;
				}

				Kopete::Message::MessageDirection dir = msgElem2.attribute( "in" ) == "1" ?
						Kopete::Message::Inbound :
						Kopete::Message::Outbound;

				if (!m_hideOutgoing || dir != Kopete::Message::Outbound)
				{
					//parse only if we don't hide it

					QString f = msgElem2.attribute( "from" );
					const Kopete::Contact *from = f.isNull()? 0L : contact->account()->contacts().value( f );

					if (!from)
						from = dir == Kopete::Message::Inbound ? contact : contact->account()->myself();

					Kopete::ContactPtrList to;
					to.append( dir==Kopete::Message::Inbound ? contact->account()->myself() : contact );

					Kopete::Message msg(from, to);

					msg.setHtmlBody( QString::fromLatin1("<span title=\"%1\">%2</span>")
							.arg( dt.toString(Qt::LocalDate), msgElem2.text() ));
					msg.setTimestamp( dt );
					msg.setDirection( dir );

					messages.append(msg);
				}
			}

			n = n.nextSibling();
		} // end while on messages

	}

	//Bubble Sort, can't use qStableSort because we have to compare surrounding items only, mostly
	//it will be only O(n) sort, because we will only have one contact in metacontact for a day.
	const int size = messages.size();
	for (int i = 0; i < size; i++)
	{
		bool swap = false;
		for (int j = 0; j < size - 1; j++)
		{
			if (messageTimestampLessThan( messages.at( j + 1 ), messages.at( j ) )) {
				messages.swap(j, j + 1);
				swap = true;
			}
		}

		if (!swap)
			break;
	}

	return messages;
}
