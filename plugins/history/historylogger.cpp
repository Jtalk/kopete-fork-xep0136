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

#include <iterator>

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

// Reverse iterator inherits base iterator but using them the same way does not look possible, so we need this.
static bool ends(const QList<HistoryMessage> &messages, const QList<HistoryMessage>::iterator &position, HistoryLogger::Sens sens)
{
	typedef std::reverse_iterator<QList<HistoryMessage>::iterator> reverse_iterator;

	if (sens == HistoryLogger::Chronological)
		return position == messages.end();
	else
		return reverse_iterator(position) == reverse_iterator(messages.begin());
}

static void inc(QList<HistoryMessage>::iterator &position, HistoryLogger::Sens sens)
{
	typedef std::reverse_iterator<QList<HistoryMessage>::iterator> reverse_iterator;

	if (sens == HistoryLogger::Chronological)
		++position;
	else
		--position;
}

static HistoryMessage& extract(QList<HistoryMessage>::iterator &position, HistoryLogger::Sens sens)
{
	typedef std::reverse_iterator<QList<HistoryMessage>::iterator> reverse_iterator;

	if (sens == HistoryLogger::Chronological)
		return *position;
	else
		return *std::prev(position);
}

// Thanks to not-so-smart C++ 2003 standard library designers we cannot just create
// an std::binary_function pointer and store either std::less or std::greater in it.
// But thanks to smart C99 designers we can create a function pointer storing one
// of those function beneath. Once Kopete is migrated to C++ 2011, we can drop all
// this stuff and replace it with simple std::function<bool(QDateTime, QDateTime)>.
static bool less(const QDateTime &fst, const QDateTime &snd)
{
	return fst < snd;
}

static bool greater(const QDateTime &fst, const QDateTime &snd)
{
	return fst > snd;
}

HistoryLogger::HistoryRange HistoryLogger::findOldestMetaContactMessagesFrom(const QDateTime &startFrom)
{
	/**
	 * TODO: explaination
	 */

	typedef QList<HistoryMessage> Messages;
	typedef bool (*Comparator)(const QDateTime &fst, const QDateTime &snd);

	HistoryRange result;
	QList<Kopete::Contact*> contacts = m_metaContact->contacts();
	Comparator lessCompare = sens == Chronological ? less : greater;
	Comparator greaterCompare = sens == Chronological ? greater : less;

	QDateTime startTime = startFrom;
	QDateTime endTime;

	foreach(Kopete::Contact *contact, contacts)
	{
		MessagesPair pair = readContactMessages( contact );

		Messages &contactMessages = pair.first;
		Messages::iterator &contactMessagesPosition = pair.second;

		if (ends(contactMessages, contactMessagesPosition, sens))
			continue;

		HistoryMessage &msg = extract(contactMessagesPosition, sens);

		QDateTime msgTime = msg.time;

		// We use isNull here instead of isValid because msgTime must always be valid thus
		// the real matter of this check is to detect whether startTime and endTime were initialized.
		// QDateTime default constructor initializes a null-object, so isNull would be enough
		// making checking much faster at the same time.
		if (!startTime.isNull() || lessCompare( msgTime, startTime ))
		{
			result.currentContact = contact;
			result.startTime = msgTime;
			continue;

		}

		if (!endTime.isNull() || greaterCompare( msgTime, endTime ))
		{
			result.nextContact = contact;
			result.nextTime = msgTime;
			continue;
		}
	}

	return result;
}

HistoryLogger::MessagesPair HistoryLogger::readContactMessages(Kopete::Contact *contact)
{
	if (m_currentElements.contains( contact ))
		return m_currentElements[ contact ];

	MessagesPair result;
	result.first = m_backend->readMessages( contact, m_currentMonth );
	result.second = result.first.end();

	if (result.first.isEmpty())
		return result;

	if (sens == Chronological)
		result.second = messages.begin();

	m_currentElements[ contact ] = result;

	return result;
}

QList<Kopete::Message> HistoryLogger::readMessages( QDate date )
{
	return m_backend->readMessages( date );
}

QList<Kopete::Message> HistoryLogger::readMessages(int lines,
	const Kopete::Contact *contact, Sens sens, bool reverseOrder, bool colorize)
{
	QList<Kopete::Message> messages;

	// TODO: Handy comment about this expression's purpose.
	QRegExp rxTime("(\\d+) (\\d+):(\\d+)($|:)(\\d*)"); //(with a 0.7.x compatibility)

	if (!m_metaContact)
	{
		//this may happen if the contact has been moved, and the MC deleted
		if (contact && contact->metaContact())
			m_metaContact = contact->metaContact();
		else
			return messages;
	}

	if (contact && !m_metaContact->contacts().contains( const_cast<Kopete::Contact*>(contact) ))
		return messages;

	if (sens == Default )  //if no sens selected, just continue with the previous one
		sens = m_oldSens;

	if ( m_oldSens != Default && sens != m_oldSens )
	{
		// we changed our sens! so retrieve the old position to fly in the other way
		m_currentElements = m_oldElements;
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

	// the merging algoritm is the following:
	//

	QDateTime timeLimit;
	const Kopete::Contact *currentContact = contact;
	if (!contact && m_metaContact->contacts().count() == 1)
		currentContact = m_metaContact->contacts().first();
	else if (!contact && m_metaContact->contacts().count() == 0)
		return messages;

	while(messages.count() < lines)
	{
		timeLimit = QDateTime();
		QDomElement msgElem; //here is the message element
		QDateTime timestamp; //and the timestamp of this message

		if (!contact && m_metaContact->contacts().count() > 1)
		{
			QList<Kopete::Message> metaContactMessages = readMetaContactMessages();
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
				if (m_currentMonth >= getFirstMonth(contact))
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
						if (!contact && (m_metaContact->contacts().count() > 1))
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
