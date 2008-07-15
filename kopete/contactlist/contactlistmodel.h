/*
    Kopete Contactlist Model

    Copyright (c) 2007      by Aleix Pol              <aleixpol@gmail.com>

    Kopete    (c) 2002-2007 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef KOPETE_UI_CONTACTLISTMODEL_H
#define KOPETE_UI_CONTACTLISTMODEL_H

#include <QAbstractItemModel>

namespace Kopete {

class Group;
class Contactlist;
class MetaContact;
	
namespace UI {

/**
@author Aleix Pol <aleixpol@gmail.com>
*/
class ContactListModel : public QAbstractItemModel
{
Q_OBJECT
	enum KopeteRoles { MetaContact=Qt::UserRole+1, };
	public:
		ContactListModel(QObject* parent = 0);
		~ContactListModel();
		
		virtual int columnCount ( const QModelIndex & parent = QModelIndex() ) const { return 1; }
		virtual QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
		virtual QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
		
		virtual QModelIndex parent ( const QModelIndex & index ) const;
		virtual int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
		virtual bool hasChildren ( const QModelIndex & parent = QModelIndex() ) const;
		
		virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	public Q_SLOTS:
		void addMetaContact( Kopete::MetaContact* );
		void removeMetaContact( Kopete::MetaContact* );
		
		void addGroup( Kopete::Group* );
		void removeGroup( Kopete::Group* );
	
	private:
		int childCount(const QModelIndex& parent) const;
		int countConnected(Kopete::Group* g) const;
		
		QList<Kopete::Group*> m_groups;
		QMap<Kopete::Group*, QList<Kopete::MetaContact*> > m_contacts;
};

}

}

#endif
