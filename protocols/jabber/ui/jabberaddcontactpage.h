
/***************************************************************************
                          jabberaddcontactpage.h  -  Add contact widget
                             -------------------
    begin                : Thu Aug 08 2002
    copyright            : (C) 2003 by Till Gerken <till@tantalo.net>
                           (C) 2003 by Daniel Stone <dstone@kde.org>
    email                : kopete-devel@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef JABBERADDCONTACTPAGE_H
#define JABBERADDCONTACTPAGE_H

#include <qwidget.h>
#include <addcontactpage.h>
#include <qlabel.h>
#include "jabberaccount.h"
#include "dlgaddcontact.h"

/**
  *@author Daniel Stone
  */
class dlgAddContact;
class JabberAccount;

class JabberAddContactPage:public AddContactPage
{
  Q_OBJECT public:
	  JabberAddContactPage (KopeteAccount * owner, QWidget * parent = 0, const char *name = 0);
	 ~JabberAddContactPage ();
	virtual bool validateData ();
	virtual bool apply (KopeteAccount *, KopeteMetaContact *);
	dlgAddContact *jabData;
	QLabel *noaddMsg1;
	QLabel *noaddMsg2;
	bool canadd;
};


#endif

/*
 * Local variables:
 * c-indentation-style: k&r
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 */
// vim: set noet ts=4 sts=4 sw=4:
