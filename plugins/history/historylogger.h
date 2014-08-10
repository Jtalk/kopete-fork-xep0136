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

#ifndef HISTORYLOGGER_H
#define HISTORYLOGGER_H

#include <QtCore/QDateTime>
#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QMap>

class QDate;
class QTimer;

namespace Kopete { class Message; }
namespace Kopete { class Contact; }
namespace Kopete { class MetaContact; }

class HistoryBackend;

/**
 * One hinstance of this class is opened for every Kopete::ChatSession,
 * or for the history dialog
 *
 * @author Olivier Goffart <ogoffart@kde.org>
 */
class HistoryLogger : public QObject
{
Q_OBJECT
public:

	/**
	 * - Chronological: messages are read from the first to the last, in the time order
	 * - AntiChronological: messages are read from the last to the first, in the time reversed order
	 */
	enum Sens { Default , Chronological , AntiChronological };

	struct HistoryRange
	{
		Kopete::Contact *currentContact;
		QDateTime startTime;

		Kopete::Contact *nextContact;
		QDateTime nextTime;

		HistoryRange(): currentContact(0L), nextContact(0L)
		{}
	};

	/**
	 * Constructor, takes the contact, and the color of messages
	 */
	explicit HistoryLogger( Kopete::MetaContact *m , QObject *parent = 0 );
	explicit HistoryLogger( Kopete::Contact *c , QObject *parent = 0 );


	~HistoryLogger();

	/**
	 * return or setif yes or no outgoing message are hidden (and not parsed)
	 */
	bool hideOutgoing() const { return m_hideOutgoing; }
	void setHideOutgoing(bool);

	/**
	 * set a searching  filter
	 * @param filter is the string to search
	 * @param caseSensitive say if the case is important
	 * @param isRegExp say if the filter is a QRegExp, or a simle string
	 */
	 void setFilter(const QString& filter, bool caseSensitive=false , bool isRegExp=false);
	 QString filter() const;
	 bool filterCaseSensitive() const ;
	 bool filterRegExp() const;



	//----------------------------------

	/**
	 * log a message
	 * @param c add a presision to the contact to use, if null, autodetect.
	 */
	void appendMessage( const Kopete::Message &msg , const Kopete::Contact *c=0L );

	/**
	 * @brief readMessagesFrom returns messages for only one @param contact from
	 * metacontact for @param date specified
	 */
	QList<HistoryMessage> readMessagesFrom( const Kopete::Contact *contact, QDate date);

	/**
	 * @brief readMessages returns messages for all contacts in metacontact for
	 * @param date specified.
	 */
	QList<HistoryMessage> readMessages(QDate date);

	/**
	 * @return The list of the days for which there is a log for m_metaContact for month of
	 * @param date (don't care of the day)
	 */
	QList<int> getDaysForMonth(QDate date);

private:
	typedef QPair<QList<HistoryMessage>, QList<HistoryMessage>::iterator> MessagesPair;

	HistoryBackend *m_backend;

	bool m_hideOutgoing;
	Qt::CaseSensitivity m_filterCaseSensitive;
	bool m_filterRegExp;
	QString m_filter;

	/**
	 * @brief initBackend choses proper history backend for this logger based on preferences
	 * and availability lists and initializes m_backend this value chosen.
	 */
	void initBackend();

	/*
	 * contais all QDomDocument, for a KC, for a specified Month
	 */
	QMap<const Kopete::Contact*,QMap<unsigned int , QDomDocument> > m_documents;

	/**
	 * Contains the current message.
	 * in fact, this is the next, still not shown
	 */
	QMap<const Kopete::Contact*, QDomElement>  m_currentElements;

	/**
	 * Get the document, open it if @param canLoad is true, contain is set to false if the document
	 * is not already contained
	 */
	QDomDocument getDocument(const Kopete::Contact *c, unsigned int month , bool canLoad=true , bool* contain=0L);

	QDomDocument getDocument(const Kopete::Contact *c, const QDate date, bool canLoad=true, bool* contain=0L);

	/**
	 * look over files to get the last month for this contact
	 */
	unsigned int getFirstMonth(const Kopete::Contact *c);
	unsigned int getFirstMonth();


	/*
	 * the current month
	 */
	unsigned int m_currentMonth;

	/*
	 * the cached getFirstMonth
	 */
	int m_cachedMonth;


	/*
	 * the metacontact we are using
	 */
	Kopete::MetaContact *m_metaContact;

	/*
	 * keep the old position in memory, so if we change the sens, we can begin here
	 */
	QMap<const Kopete::Contact*, QDomElement>  m_oldElements;
	unsigned int m_oldMonth;
	Sens m_oldSens;

	/**
	 * workaround for the 31 midnight bug.
	 * it contains the number of the current month.
	 */
	int m_realMonth;

	/*
	 * FIXME:
	 * WORKAROUND
	 * due to a bug in QT, i have to keep the document element in the memory to
	 * prevent crashes
	 */
	QList<QDomElement> workaround;

	HistoryRange findOldestMetaContactMessagesFrom( const QDateTime &startFrom );
	HistoryMessages readContactMessages( Kopete::Contact *contact, QDate date = QDate(), int lines = 0 );

	bool nextMonth( Sens sens );

private slots:
	/**
	 * the metacontact has been deleted
	 */
	void slotMCDeleted();
	
	/**
	 * save the current month's document on the disk. 
	 * connected to the m_saveTimer signal
	 */
	void saveToDisk();
};

#endif
