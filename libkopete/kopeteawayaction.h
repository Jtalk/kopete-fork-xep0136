/*
    kopetehistorydialog.h - Kopete Away Action

    Copyright (c) 2003     Jason Keirstead   <jason@keirstead.org>

    Kopete    (c) 2002 by the Kopete developers  <kopete-devel@kde.org>

   *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#ifndef KOPETEAWAYACTION_H
#define KOPETEAWAYACTION_H

#include <kdeversion.h>

#if KDE_IS_VERSION( 3, 1, 90 )
	#include <kactionclasses.h>
#endif

#include <kaction.h>

/**
 * @class KopeteAwayAction kopeteawayaction.h
 *
 * KopeteAwayAction is a KAction that lets you select an away message
 * from the list of predefined away messages, or enter custom one.
 *
 * @author Jason Keirstead   <jason@keirstead.org>
 */
 
class KopeteAwayAction : public KSelectAction
{
	Q_OBJECT
	public:
		KopeteAwayAction(const QString &text, const QIconSet &pix, const KShortcut &cut, const QObject *receiver, const char *slot, QObject *parent, const char *name = 0);
	
	signals:
		/**
		* @brief Emits when the user selects an away message
		*/
		void awayMessageSelected( const QString & );

	private slots:
		void slotAwayChanged();
		void slotSelectAway( int index );

	private:
		int reasonCount;
};

#endif
