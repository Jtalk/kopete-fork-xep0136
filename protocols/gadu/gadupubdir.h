//
// Copyright (C) 2003	 Grzegorz Jaskiewicz <gj at pointblue.com.pl>
//
// gadupubdir.h
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//

#ifndef GADUPUBDIR_H
#define GADUPUBDIR_H


#include "gaduaccount.h"
#include "gaduprotocol.h"
#include "gadusearch.h"

#include <kdebug.h>
#include <qhbox.h>
#include <kdialogbase.h>
#include <krestrictedline.h>
#include <kcombobox.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <qpixmap.h>


class GaduProtocol;
class GaduAccount;
class GaduPublicDirectory;

class GaduPublicDir : public KDialogBase
{
Q_OBJECT

public:
	GaduPublicDir( GaduAccount* , QWidget *parent = 0, const char* name = "GaduPublicDir" );
	GaduPublicDir( GaduAccount* , int searchFor, QWidget* parent = 0, const char* name = "GaduPublicDir" );
	QPixmap iconForStatus( uint status );

private slots:
	void slotSearch();
	void slotNewSearch();
	void slotSearchResult( const searchResult& result );
	void inputChanged( const QString& );
	void inputChanged( bool );

private:
	void getData();
	bool validateData();
	void createWidget();
	void initConnections();

	GaduProtocol*		p;
	GaduAccount*		mAccount;
	GaduContact*		mContact;
	GaduPublicDirectory*	mMainWidget;

// form data
	QString	fName;
	QString	fSurname;
	QString	fNick;
	QString	fCity;
	int		fUin;
	int		fGender;
	bool		fOnlyOnline;
	int		fAgeFrom;
	int		fAgeTo;
};
#endif
