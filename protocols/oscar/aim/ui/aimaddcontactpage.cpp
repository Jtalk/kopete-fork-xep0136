/***************************************************************************
                            description
                             -------------------
    begin                :
    copyright            : (C) 2002 by nbetcher
    email                : nbetcher@usinternet.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "aimaddcontactui.h"
#include "aimaddcontactpage.h"

#include "kopeteaccount.h"

#include <qlayout.h>
#include <qlineedit.h>
#include <klocale.h>
#include <kmessagebox.h>

AIMAddContactPage::AIMAddContactPage(bool connected, QWidget *parent,
					 const char *name )
	: AddContactPage(parent,name)
{
	(new QVBoxLayout(this))->setAutoAdd(true);

	if(connected)
	{
		m_gui = new aimAddContactUI(this);
		canadd = true;
	}
	else
	{
		noaddMsg1 = new QLabel(i18n("You need to be connected to be able to add contacts."), this);
		noaddMsg2 = new QLabel(i18n("Connect to the AIM network and try again."), this);
		canadd = false;
	}
}


AIMAddContactPage::~AIMAddContactPage()
{
}

bool AIMAddContactPage::validateData()
{
	QString sn = m_gui->addSN->text();
	if ( sn.isEmpty() )
	{
		KMessageBox::sorry ( this,
			i18n("<qt>You must enter a valid screenname.</qt>"),
			i18n("You Must Enter a Screenname") );
		return false;
	}
	return true;
}

bool AIMAddContactPage::apply(KopeteAccount *account,
	KopeteMetaContact *metaContact)
{
	if(validateData())
	{ // If everything is ok
		return account->addContact( m_gui->addSN->text(), m_gui->addSN->text(),
			metaContact);
	}
	return false;
}
// vim: set noet ts=4 sts=4 sw=4:

#include "aimaddcontactpage.moc"
