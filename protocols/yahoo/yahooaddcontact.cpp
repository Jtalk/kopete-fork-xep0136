/*
    yahooaddcontact.cpp - UI Page for Adding a Yahoo Contact

    Copyright (c) 2003 by Gav Wood               <gav@kde.org>
    Copyright (c) 2003 by Matt Rogers            <mattrogers@sbcglobal.net>
    Based on code by Duncan Mac-Vicar Prett      <duncan@kde.org>
    Kopete    (c) 2003 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

// QT Includes
#include <qlayout.h>

// KDE Includes
#include <kdebug.h>
#include <klineedit.h>

// Kopete Includes
#include <addcontactpage.h>
#include <kopeteaccount.h>

// Local Includes
#include "yahooadd.h"
#include "yahooaddcontact.h"
#include "yahooaccount.h"

// Yahoo Add Contact page
YahooAddContact::YahooAddContact(YahooProtocol *owner, QWidget *parent, const char *name): AddContactPage(parent, name)
{
	kdDebug(14180) << "YahooAddContact::YahooAddContact(<owner>, <parent>, " << name << ")" << endl;

	(new QVBoxLayout(this))->setAutoAdd(true);
	theDialog = new YahooAddContactBase(this);
	theDialog->show();
	theProtocol = owner;
}

// Destructor
YahooAddContact::~YahooAddContact()
{
	kdDebug(14180) << k_funcinfo << endl;
}

bool YahooAddContact::validateData()
{
	kdDebug(14180) << k_funcinfo << endl;

	return theDialog->contactID->text() != "";
}

bool YahooAddContact::apply(KopeteAccount *theAccount, KopeteMetaContact *theMetaContact)
{
	kdDebug(14180) << k_funcinfo << endl;

	QString displayName = theDialog->displayName->text() == "" ? theDialog->contactID->text() : theDialog->displayName->text();
	YahooAccount* myAccount = static_cast<YahooAccount*>(theAccount);
	myAccount->addContact(theDialog->contactID->text(), displayName, theMetaContact, KopeteAccount::ChangeKABC );
	return true;
}

#include "yahooaddcontact.moc"
/*
 * Local variables:
 * c-indentation-style: k&r
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 */
// vim: set noet ts=4 sts=4 sw=4:

