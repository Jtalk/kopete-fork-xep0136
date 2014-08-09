/*
    historylogger.cpp

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

#include "historylogger.h"

#include <QtCore/QRegExp>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QtCore/QTimer>
#include <QtCore/QTextStream>
#include <QtCore/QList>
#include <QtCore/QDate>
#include <QtGui/QTextDocument>

#include <kdebug.h>
#include <kstandarddirs.h>
#include <ksavefile.h>

#include "kopeteglobal.h"
#include "kopetecontact.h"
#include "kopeteprotocol.h"
#include "kopeteaccount.h"
#include "kopetemetacontact.h"
#include "kopetechatsession.h"

#include "historyconfig.h"
#include "backends/historybackend.h"
#include "backends/xmlbackend.h"

bool messageTimestampLessThan(const Kopete::Message &m1, const Kopete::Message &m2)
{
	const Kopete::Contact* c1 = (m1.direction() == Kopete::Message::Outbound) ? m1.to().value(0) : m1.from();
	const Kopete::Contact* c2 = (m2.direction() == Kopete::Message::Outbound) ? m2.to().value(0) : m2.from();

	if (c1 == c2) // Messages from the same account, keep order as it was saved.
		return false;

	return m1.timestamp() < m2.timestamp();
}

// -----------------------------------------------------------------------------
HistoryLogger::HistoryLogger( Kopete::MetaContact *m,  QObject *parent )
 : QObject(parent)
{
	m_metaContact = m;
	m_hideOutgoing = false;
	m_cachedMonth=-1;
	m_realMonth = QDate::currentDate().month();
	m_oldSens = Default;
	m_filterCaseSensitive = Qt::CaseSensitive;
	m_filterRegExp = false;

	initBackend();

	//the contact may be destroyed, for example, if the contact changes its metacontact
	connect(m_metaContact , SIGNAL(destroyed(QObject*)) , this , SLOT(slotMCDeleted()));

	setPositionToLast();
}


HistoryLogger::HistoryLogger( Kopete::Contact *c,  QObject *parent )
 : QObject(parent)
{
	m_cachedMonth = -1;
	m_metaContact = c->metaContact();
	m_hideOutgoing = false;
	m_realMonth = QDate::currentDate().month();
	m_oldSens = Default;
	m_filterCaseSensitive = Qt::CaseSensitive;
	m_filterRegExp = false;

	initBackend();

	//the contact may be destroyed, for example, if the contact changes its metacontact
	connect(m_metaContact , SIGNAL(destroyed(QObject*)) , this , SLOT(slotMCDeleted()));

	setPositionToLast();
}


HistoryLogger::~HistoryLogger()
{
	delete m_backend;
}

void HistoryLogger::initBackend()
{
#warning "Proper backend chosing"
	m_backend = new XMLBackend();
}

void HistoryLogger::setPositionToLast()
{
	setCurrentMonth(0);
	m_oldSens = AntiChronological;
	m_oldMonth = 0;
	m_oldElements.clear();
}


void HistoryLogger::setPositionToFirst()
{
	setCurrentMonth( getFirstMonth() );
	m_oldSens = Chronological;
	m_oldMonth = m_currentMonth;
	m_oldElements.clear();
}


void HistoryLogger::setCurrentMonth(int month)
{
	m_currentMonth = month;
	m_currentElements.clear();
}


QDomDocument HistoryLogger::getDocument(const Kopete::Contact *c, unsigned int month , bool canLoad , bool* contain)
{
	if (m_realMonth!=QDate::currentDate().month())
	{ //We changed month, our index is not correct anymore, clean memory.
	  // or we will see what i called "the 31 midnight bug"(TM) :-)  -Olivier
		m_documents.clear();
		m_cachedMonth=-1;
		m_currentMonth++; //Not usre it's ok, but should work;
		m_oldMonth++;     // idem
		m_realMonth = QDate::currentDate().month();
	}

	if (!m_metaContact)
	{ //this may happen if the contact has been moved, and the MC deleted
		if (c && c->metaContact())
			m_metaContact = c->metaContact();
		else
			return QDomDocument();
	}

	if (!m_metaContact->contacts().contains(const_cast<Kopete::Contact*>(c)))
	{
		if (contain)
			*contain = false;
		return QDomDocument();
	}

	QMap<unsigned int , QDomDocument> documents = m_documents[c];
	if (documents.contains(month))
		return documents[month];


	QDomDocument doc =  getDocument(c, QDate::currentDate().addMonths(0-month), canLoad, contain);

	documents.insert(month, doc);
	m_documents[c]=documents;

	return doc;

}

QDomDocument HistoryLogger::getDocument(const Kopete::Contact *contact, const QDate date , bool canLoad , bool* contain)
{
	Kopete::Contact *c = const_cast<Kopete::Contact*>(contact);
	if (!m_metaContact)
	{ //this may happen if the contact has been moved, and the MC deleted
		if (c && c->metaContact())
			m_metaContact = c->metaContact();
		else
			return QDomDocument();
	}

	if (!m_metaContact->contacts().contains(c))
	{
		if (contain)
			*contain = false;
		return QDomDocument();
	}

	if (!canLoad)
	{
		if (contain)
			*contain = false;
		return QDomDocument();
	}

	QString	FileName = getFileName(c, date);

	QDomDocument doc( "Kopete-History" );

	QFile file( FileName );
	if ( !file.open( QIODevice::ReadOnly ) )
	{
		if (contain)
			*contain = false;
		return doc;
	}
	if ( !doc.setContent( &file ) )
	{
		file.close();
		if (contain)
			*contain = false;
		return doc;
	}
	file.close();

	if ( contain )
		*contain = true;

	return doc;
}


void HistoryLogger::appendMessage( const Kopete::Message &msg , const Kopete::Contact *ct )
{
	if (!msg.from())
		return;

	// If no contact are given: If the manager is availiable, use the manager's
	// first contact (the channel on irc, or the other contact for others protocols
	const Kopete::Contact *contact = ct;
	if (!contact && msg.manager())
	{
		QList<Kopete::Contact*> conversationMembers = msg.manager()->members();
		contact = conversationMembers.first();
	}

	if (!contact)  //If the contact is still not initialized, use the message author.
		contact = msg.direction() == Kopete::Message::Outbound ? msg.to().first() : msg.from();


	if (!m_metaContact)
	{
		//this may happen if the contact has been moved, and the MC deleted
		if (contact && contact->metaContact())
			m_metaContact = contact->metaContact();
		else
			return;
	}


	if (!contact || !m_metaContact->contacts().contains( const_cast<Kopete::Contact*>( contact ) ))
	{
		kWarning(14310) << "No contact found in this metacontact to append in the history" << endl;
		return;
	}

	m_backend->appendMessage( msg );
}

QList<Kopete::Message> HistoryLogger::readMessages( QDate date )
{
	return m_backend->readMessages( date );
}

QList<Kopete::Message> HistoryLogger::readMessages(int lines,
	const Kopete::Contact *c, Sens sens, bool reverseOrder, bool colorize)
{
	//QDate dd =  QDate::currentDate().addMonths(0-m_currentMonth);

	QList<Kopete::Message> messages;

	// A regexp useful for this function
	QRegExp rxTime("(\\d+) (\\d+):(\\d+)($|:)(\\d*)"); //(with a 0.7.x compatibility)

	if (!m_metaContact)
	{ //this may happen if the contact has been moved, and the MC deleted
		if (c && c->metaContact())
			m_metaContact = c->metaContact();
		else
			return messages;
	}

	if (c && !m_metaContact->contacts().contains(const_cast<Kopete::Contact*>(c)) )
		return messages;

	if (sens == Default )  //if no sens are selected, just continue in the previous sens
		sens = m_oldSens ;
	if ( m_oldSens != Default && sens != m_oldSens )
	{ //we changed our sens! so retrieve the old position to fly in the other way
		m_currentElements= m_oldElements;
		m_currentMonth = m_oldMonth;
	}
	else
	{
		m_oldElements = m_currentElements;
		m_oldMonth = m_currentMonth;
	}
	m_oldSens = sens;

	//getting the color for messages:
	QColor fgColor = HistoryConfig::history_color();

	//Hello guest!

	//there are two algoritms:
	// - if a contact is given, or the metacontact contain only one contact,  just read the history.
	// - else, merge the history

	//the merging algoritm is the following:
	// we see what contact we have to read first, and we look at the firt date before another contact
	// has a message with a bigger date.

	QDateTime timeLimit;
	const Kopete::Contact *currentContact = c;
	if (!c && m_metaContact->contacts().count()==1)
		currentContact = m_metaContact->contacts().first();
	else if (!c && m_metaContact->contacts().count()== 0)
	{
		return messages;
	}

	while(messages.count() < lines)
	{
		timeLimit = QDateTime();
		QDomElement msgElem; //here is the message element
		QDateTime timestamp; //and the timestamp of this message

		if (!c && m_metaContact->contacts().count()>1)
		{ //we have to merge the differents subcontact history
			QList<Kopete::Contact*> ct = m_metaContact->contacts();

			foreach(Kopete::Contact *contact, ct)
			{ //we loop over each contact. we are searching the contact with the next message with the smallest date,
			  // it will becomes our current contact, and the contact with the mext message with the second smallest
			  // date, this date will bocomes the limit.

				QDomNode n;
				if (m_currentElements.contains(contact))
					n = m_currentElements[contact];
				else  //there is not yet "next message" register, so we will take the first  (for the current month)
				{
					QDomDocument doc = getDocument(contact,m_currentMonth);
					QDomElement docElem = doc.documentElement();
					n= (sens==Chronological)?docElem.firstChild() : docElem.lastChild();

					//i can't drop the root element
					workaround.append(docElem);
				}
				while(!n.isNull())
				{
					QDomElement  msgElem2 = n.toElement();
					if ( !msgElem2.isNull() && msgElem2.tagName()=="msg")
					{
						rxTime.indexIn(msgElem2.attribute("time"));
						QDate d = QDate::currentDate().addMonths(0-m_currentMonth);
						QDateTime dt( QDate(d.year() , d.month() , rxTime.cap(1).toUInt()), QTime( rxTime.cap(2).toUInt() , rxTime.cap(3).toUInt(), rxTime.cap(5).toUInt()  ) );
						if (!timestamp.isValid() || ((sens==Chronological )? dt < timestamp : dt > timestamp) )
						{
							timeLimit = timestamp;
							timestamp = dt;
							msgElem = msgElem2;
							currentContact = contact;

						}
						else if (!timeLimit.isValid() || ((sens==Chronological) ? timeLimit > dt : timeLimit < dt) )
						{
							timeLimit = dt;
						}
						break;
					}
					n=(sens==Chronological)? n.nextSibling() : n.previousSibling();
				}
			}
		}
		else  //we don't have to merge the history. just take the next item in the contact
		{
			if (m_currentElements.contains(currentContact))
				msgElem = m_currentElements[currentContact];
			else
			{
				QDomDocument doc = getDocument(currentContact,m_currentMonth);
				QDomElement docElem = doc.documentElement();
				QDomNode n= (sens==Chronological)?docElem.firstChild() : docElem.lastChild();
				msgElem = QDomElement();
				while(!n.isNull()) //continue until we get a msg
				{
					msgElem = n.toElement();
					if ( !msgElem.isNull() && msgElem.tagName()=="msg")
					{
						m_currentElements[currentContact]=msgElem;
						break;
					}
					n=(sens==Chronological)? n.nextSibling() : n.previousSibling();
				}

				//i can't drop the root element
				workaround.append(docElem);
			}
		}


		if (msgElem.isNull()) //we don't find ANY messages in any contact for this month. so we change the month
		{
			if (sens==Chronological)
			{
				if (m_currentMonth <= 0)
					break; //there are no other messages to show. break even if we don't have nb messages
				setCurrentMonth(m_currentMonth-1);
			}
			else
			{
				if (m_currentMonth >= getFirstMonth(c))
					break; //we don't have any other messages to show
				setCurrentMonth(m_currentMonth+1);
			}
			continue; //begin the loop from the bottom, and find currentContact and timeLimit again
		}

		while(
			(messages.count() < lines) &&
			!msgElem.isNull() &&
			(!timestamp.isValid() || !timeLimit.isValid() ||
				((sens==Chronological) ? timestamp <= timeLimit : timestamp >= timeLimit)
			))
		{
			// break this loop, if we have reached the correct number of messages,
			// if there are no more messages for this contact, or if we reached
			// the timeLimit msgElem is the next message, still not parsed, so
			// we parse it now

			Kopete::Message::MessageDirection dir = (msgElem.attribute("in") == "1") ?
				Kopete::Message::Inbound : Kopete::Message::Outbound;

			if (!m_hideOutgoing || dir != Kopete::Message::Outbound)
			{ //parse only if we don't hide it

				if ( m_filter.isNull() || ( m_filterRegExp? msgElem.text().contains(QRegExp(m_filter,m_filterCaseSensitive)) : msgElem.text().contains(m_filter,m_filterCaseSensitive) ))
				{
					Q_ASSERT(currentContact);
					QString f = msgElem.attribute("from" );
					const Kopete::Contact *from = f.isNull() ? 0L : currentContact->account()->contacts().value(f);

					if ( !from )
						from = (dir == Kopete::Message::Inbound) ? currentContact : currentContact->account()->myself();

					Kopete::ContactPtrList to;
					to.append( dir==Kopete::Message::Inbound ? currentContact->account()->myself() : const_cast<Kopete::Contact*>(currentContact) );

					if (!timestamp.isValid())
					{
						//parse timestamp only if it was not already parsed
						rxTime.indexIn(msgElem.attribute("time"));
						QDate d = QDate::currentDate().addMonths(0-m_currentMonth);
						timestamp = QDateTime( QDate(d.year() , d.month() , rxTime.cap(1).toUInt()), QTime( rxTime.cap(2).toUInt() , rxTime.cap(3).toUInt() , rxTime.cap(5).toUInt() ) );
					}

					Kopete::Message msg(from, to);
					msg.setTimestamp( timestamp );
					msg.setDirection( dir );

					if (colorize)
					{
						msg.setHtmlBody( QString::fromLatin1("<span style=\"color:%1\" title=\"%2\">%3</span>")
							.arg( fgColor.name(), timestamp.toString(Qt::LocalDate), msgElem.text() ));
						msg.setForegroundColor( fgColor );
						msg.addClass( "history" );
					}
					else
					{
						msg.setHtmlBody( QString::fromLatin1("<span title=\"%1\">%2</span>")
							.arg( timestamp.toString(Qt::LocalDate), msgElem.text() ));
					}

					if (reverseOrder)
						messages.prepend(msg);
					else
						messages.append(msg);
				}
			}

			//here is the point of workaround. If i drop the root element, this crashes
			//get the next message
			QDomNode node = ( (sens==Chronological) ? msgElem.nextSibling() :
				msgElem.previousSibling() );

			msgElem = QDomElement(); //n.toElement();
			while (!node.isNull() && msgElem.isNull())
			{
				msgElem = node.toElement();
				if (!msgElem.isNull())
				{
					if (msgElem.tagName() == "msg")
					{
						if (!c && (m_metaContact->contacts().count() > 1))
						{
							// In case of hideoutgoing messages, it is faster to do
							// this, so we don't parse the date if it is not needed
							QRegExp rx("(\\d+) (\\d+):(\\d+):(\\d+)");
							rx.indexIn(msgElem.attribute("time"));

							QDate d = QDate::currentDate().addMonths(0-m_currentMonth);
							timestamp = QDateTime(
								QDate(d.year(), d.month(), rx.cap(1).toUInt()),
								QTime( rx.cap(2).toUInt(), rx.cap(3).toUInt() ) );
						}
						else
							timestamp = QDateTime(); //invalid
					}
					else
						msgElem = QDomElement();
				}

				node = (sens == Chronological) ? node.nextSibling() :
					node.previousSibling();
			}
			m_currentElements[currentContact]=msgElem;  //this is the next message
		}
	}

	if (messages.count() < lines)
		m_currentElements.clear(); //current elements are null this can't be allowed

	return messages;
}

QString HistoryLogger::getFileName(const Kopete::Contact* c, QDate date)
{

	QString name = c->protocol()->pluginId().replace( QRegExp( QString::fromLatin1( "[./~?*]" ) ), QString::fromLatin1( "-" ) ) +
		QString::fromLatin1( "/" ) +
		c->account()->accountId().replace( QRegExp( QString::fromLatin1( "[./~?*]" ) ), QString::fromLatin1( "-" ) ) +
		QString::fromLatin1( "/" ) +
	c->contactId().replace( QRegExp( QString::fromLatin1( "[./~?*]" ) ), QString::fromLatin1( "-" ) ) +
		date.toString(".yyyyMM");

	QString filename = KStandardDirs::locateLocal( "data", QString::fromLatin1( "kopete/logs/" ) + name+ QString::fromLatin1( ".xml" ) ) ;

	//Check if there is a kopete 0.7.x file
	QFileInfo fi(filename);
	if (!fi.exists())
	{
		name = c->protocol()->pluginId().replace( QRegExp( QString::fromLatin1( "[./~?*]" ) ), QString::fromLatin1( "-" ) ) +
			QString::fromLatin1( "/" ) +
			c->contactId().replace( QRegExp( QString::fromLatin1( "[./~?*]" ) ), QString::fromLatin1( "-" ) ) +
			date.toString(".yyyyMM");

		QString filename2 = KStandardDirs::locateLocal( "data", QString::fromLatin1( "kopete/logs/" ) + name+ QString::fromLatin1( ".xml" ) ) ;

		QFileInfo fi2(filename2);
		if (fi2.exists())
			return filename2;
	}

	return filename;

}

unsigned int HistoryLogger::getFirstMonth(const Kopete::Contact *c)
{
	if (!c)
		return getFirstMonth();

	QRegExp rx( "\\.(\\d\\d\\d\\d)(\\d\\d)" );

	// BEGIN check if there are Kopete 0.7.x
	QDir d1(KStandardDirs::locateLocal("data",QString("kopete/logs/")+
		c->protocol()->pluginId().replace( QRegExp(QString::fromLatin1("[./~?*]")),QString::fromLatin1("-"))
		));
	d1.setFilter( QDir::Files | QDir::NoSymLinks );
	d1.setSorting( QDir::Name );

	const QFileInfoList list1 = d1.entryInfoList();

	foreach(const QFileInfo &fi, list1)
	{
		if (fi.fileName().contains(c->contactId().replace( QRegExp( QString::fromLatin1( "[./~?*]" ) ), QString::fromLatin1( "-" ) )))
		{
			rx.indexIn(fi.fileName());
			int result = 12*(QDate::currentDate().year() - rx.cap(1).toUInt()) +QDate::currentDate().month() - rx.cap(2).toUInt();

			if (result < 0)
			{
				kWarning(14310) << "Kopete only found log file from Kopete 0.7.x made in the future. Check your date!";
				break;
			}
			return result;
		}
	}
	// END of kopete 0.7.x check


	QDir d(KStandardDirs::locateLocal("data",QString("kopete/logs/")+
		c->protocol()->pluginId().replace( QRegExp(QString::fromLatin1("[./~?*]")),QString::fromLatin1("-")) +
		QString::fromLatin1( "/" ) +
		c->account()->accountId().replace( QRegExp( QString::fromLatin1( "[./~?*]" ) ), QString::fromLatin1( "-" ) )
		));

	d.setFilter( QDir::Files | QDir::NoSymLinks );
	d.setSorting( QDir::Name );

	const QFileInfoList list = d.entryInfoList();

	foreach(const QFileInfo &fi, list)
	{
		if (fi.fileName().contains(c->contactId().replace( QRegExp( QString::fromLatin1( "[./~?*]" ) ), QString::fromLatin1( "-" ) )))
		{
			rx.indexIn(fi.fileName());
			int result = 12*(QDate::currentDate().year() - rx.cap(1).toUInt()) +QDate::currentDate().month() - rx.cap(2).toUInt();
			if (result < 0)
			{
				kWarning(14310) << "Kopete only found log file made in the future. Check your date!";
				break;
			}
			return result;
		}
	}
	return 0;
}

unsigned int HistoryLogger::getFirstMonth()
{
	if (m_cachedMonth!=-1)
		return m_cachedMonth;

	if (!m_metaContact)
		return 0;

	int m = 0;
	QList<Kopete::Contact*> contacts = m_metaContact->contacts();

	foreach(Kopete::Contact* contact, contacts)
	{
		int m2 = getFirstMonth(contact);
		if (m2>m) m = m2;
	}
	m_cachedMonth = m;
	return m;
}

void HistoryLogger::setHideOutgoing(bool b)
{
	m_hideOutgoing = b;
}

void HistoryLogger::slotMCDeleted()
{
	m_metaContact = 0;
}

void HistoryLogger::setFilter(const QString& filter, bool caseSensitive , bool isRegExp)
{
	m_filter = filter;
	m_filterCaseSensitive = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
	m_filterRegExp = isRegExp;
}

QString HistoryLogger::filter() const
{
	return m_filter;
}

bool HistoryLogger::filterCaseSensitive() const
{
	return (m_filterCaseSensitive == Qt::CaseSensitive);
}

bool HistoryLogger::filterRegExp() const
{
	return m_filterRegExp;
}

QList<int> HistoryLogger::getDaysForMonth(QDate date)
{
	QRegExp rxTime("time=\"(\\d+) \\d+:\\d+(:\\d+)?\""); //(with a 0.7.x compatibility)

	QList<int> dayList;

	QList<Kopete::Contact*> contacts = m_metaContact->contacts();

	int lastDay = 0;
	foreach(Kopete::Contact *contact, contacts)
	{
//		kDebug() << getFileName(*it, date);
		QFile file(getFileName(contact, date));
		if (!file.open(QIODevice::ReadOnly))
		{
			continue;
		}
		QTextStream stream(&file);
		QString fullText = stream.readAll();
		file.close();

		int pos = 0;
		while( (pos = rxTime.indexIn(fullText, pos)) != -1)
		{
			pos += rxTime.matchedLength();
			int day = rxTime.capturedTexts()[1].toInt();

			if ( day !=lastDay && dayList.indexOf(day) == -1) // avoid duplicates
			{
				dayList.append(rxTime.capturedTexts()[1].toInt());
				lastDay = day;
			}
		}
	}
	return dayList;
}

#include "historylogger.moc"
