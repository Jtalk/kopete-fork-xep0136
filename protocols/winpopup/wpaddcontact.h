/***************************************************************************
                          wpaddcontact.h  -  description
                             -------------------
    begin                : Wed Jan 23 2002
    copyright            : (C) 2002 by Gav Wood
    email                : gav@indigoarchive.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __WPADDCONTACT_H
#define __WPADDCONTACT_H

// Local Includes

// Kopete Includes
#include <addcontactpage.h>

// QT Includes

// KDE Includes

class WPProtocol;
class WPAddContactBase;

class WPAddContact: public AddContactPage
{
	Q_OBJECT

private:
	WPProtocol *theProtocol;
	WPAddContactBase *theDialog;

public:
	WPAddContact(WPProtocol *owner, QWidget *parent = 0, const char *name = 0);
	~WPAddContact();

	virtual bool validateData();
	
public slots:
	virtual void slotFinish();
	void slotSelected(const QString &Group);
	void slotUpdateGroups();
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

