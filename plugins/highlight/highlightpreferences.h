/***************************************************************************
                          highlightpreferences.h  -  description
                             -------------------
    begin                : mar 14 2003
    copyright            : (C) 2003 by Olivier Goffart
    email                : ogoffart @ kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef HighlightPREFERENCES_H
#define HighlightPREFERENCES_H

#include <kcmodule.h>
#include <qstring.h>

namespace Ui { class HighlightPrefsUI; }
class Filter;

/**
  *@author Olivier Goffart
  */

class HighlightPreferences : public KCModule  {
   Q_OBJECT
public:

	HighlightPreferences(QWidget *parent = 0, const QStringList &args = QStringList());
	~HighlightPreferences();

	virtual void save();
	virtual void load();

private:
	Ui::HighlightPrefsUI preferencesDialog;
	HighlightConfig *m_config;
	
	Filter *selectedItem();

	bool donttouch;

private slots:
	void slotCurrentFilterChanged();
	void slotAddFilter();
	void slotRemoveFilter();
	void slotRenameFilter();
	void slotSomethingHasChanged();
	void slotEditRegExp();
	void slotConfigureNotifications();
};

#endif
