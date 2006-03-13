/*
    kopetelistviewsearchline.h - a widget for performing quick searches of Kopete::ListViews

    Copyright (c) 2004      by Richard Smith <kde@metafoo.co.uk>

    Kopete    (c) 2004      by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef KOPETELISTVIEWSEARCHLINE_H
#define KOPETELISTVIEWSEARCHLINE_H

#include <k3listviewsearchline.h>

namespace Kopete {
namespace UI {
namespace ListView {

class ListView;

class SearchLine : public K3ListViewSearchLine
{
	Q_OBJECT
public:
	/**
	 * Constructs a SearchLine with \a listView being the
	 * ListView to be filtered.
	 *
	 * If \a listView is null then the widget will be disabled until a listview
	 * is set with setListView().
	 */
	SearchLine( QWidget *parent, ListView *listView = 0, const char *name = 0 );
	/**
	 * Constructs a SearchLine without any ListView to filter. The
	 * K3ListView object has to be set later with setListView().
	 */
	SearchLine(QWidget *parent, const char *name);
	/**
	 * Destroys the SearchLine.
	 */
	~SearchLine();
	
	void updateSearch( const QString &s );
	
protected:
	virtual void checkItemParentsNotVisible();
	virtual bool checkItemParentsVisible( Q3ListViewItem *it );
	virtual void setItemVisible( Q3ListViewItem *it, bool visible );
	
private:
	QString search;
};

} // end namespace ListView
} // end namespace UI
} // end namespace Kopete

#endif
