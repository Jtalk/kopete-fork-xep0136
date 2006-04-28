/*
    Copyright (c) 2002-2005 by Olivier Goffart       <ogoffart@ kde.org>
    Kopete    (c) 2002-2005 by The Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/


#include <qlayout.h>
#include <qlineedit.h>
//Added by qt3to4:
#include <QVBoxLayout>

#include <klocale.h>
#include <kmessagebox.h>

#include "ui_msnadd.h"
#include "msnaddcontactpage.h"
#include "msnprotocol.h"
#include "kopeteaccount.h"
#include "kopeteuiglobal.h"

MSNAddContactPage::MSNAddContactPage(bool connected, QWidget *parent)
				  : AddContactPage(parent)
{
	(new QVBoxLayout(this))->setAutoAdd(true);
/*	if ( connected )
	{*/
			QWidget* w = new QWidget( this );
			msndata = new Ui::msnAddUI();
			msndata->setupUi( w );
			/*
			msndata->cmbGroup->insertStringList(owner->getGroups());
			msndata->cmbGroup->setCurrentItem(0);
			*/
			canadd = true;

/*	}
	else
	{
			noaddMsg1 = new QLabel( i18n( "You need to be connected to be able to add contacts." ), this );
			noaddMsg2 = new QLabel( i18n( "Please connect to the MSN network and try again." ), this );
			canadd = false;
}*/

}
MSNAddContactPage::~MSNAddContactPage()
{
	delete msndata;
}

bool MSNAddContactPage::apply( Kopete::Account* i, Kopete::MetaContact*m )
{
	if ( validateData() )
	{
		QString userid = msndata->addID->text();
		return i->addContact( userid , m, Kopete::Account::ChangeKABC );
	}
	return false;
}


bool MSNAddContactPage::validateData()
{
	if(!canadd)
		return false;

	QString userid = msndata->addID->text();

	if(MSNProtocol::validContactId(userid))
		return true;

	KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Sorry,
			i18n( "<qt>You must enter a valid email address.</qt>" ), i18n( "MSN Plugin" )  );

	return false;

}

#include "msnaddcontactpage.moc"

// vim: set noet ts=4 sts=4 sw=4:

