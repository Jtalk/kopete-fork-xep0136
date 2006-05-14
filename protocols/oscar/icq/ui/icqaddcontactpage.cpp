 /*
    icqaddcontactpage.cpp  -  ICQ Protocol Plugin

    Copyright (c) 2002 by Stefan Gehn <metz AT gehn.net>
    Kopete    (c) 2002-2003 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "icqaddcontactpage.h"

#include <ctype.h>

#include <qlayout.h>
#include <qpushbutton.h>
#include <q3combobox.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <qtabwidget.h>
#include <qlabel.h>
//Added by qt3to4:
#include <QShowEvent>
#include <QVBoxLayout>

#include <kdebug.h>
#include <kiconloader.h>
#include <k3listview.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <kmessagebox.h>

#include "ui_icqadd.h"
#include "icqaccount.h"
#include "icqprotocol.h"
#include "icqsearchdialog.h"


ICQAddContactPage::ICQAddContactPage(ICQAccount *owner, QWidget *parent)
	: AddContactPage(parent)
{
	kDebug(14153) << k_funcinfo << "called" << endl;
	mAccount = owner;
	m_searchDialog = 0L;

	addUI = new Ui::icqAddUI();
	addUI->setupUi(this);
	connect( addUI->searchButton, SIGNAL( clicked() ), this, SLOT( showSearchDialog() ) );
}

ICQAddContactPage::~ICQAddContactPage()
{
	delete addUI;
}

void ICQAddContactPage::setUINFromSearch( const QString& uin )
{
	addUI->uinEdit->setText( uin );
}

void ICQAddContactPage::showEvent(QShowEvent *e)
{
//	slotSelectionChanged();
	AddContactPage::showEvent(e);
}

bool ICQAddContactPage::apply(Kopete::Account* , Kopete::MetaContact *parentContact  )
{
	kDebug(14153) << k_funcinfo << "called; adding contact..." << endl;

	QString contactId = addUI->uinEdit->text();
	kDebug(14153) << k_funcinfo << "uin=" << contactId << endl;
	return mAccount->addContact(contactId, parentContact, Kopete::Account::ChangeKABC );

}

bool ICQAddContactPage::validateData()
{
	if(!mAccount->isConnected())
	{
		// Account currently offline
		addUI->searchButton->setEnabled( false );
		addUI->uinEdit->setEnabled( false );
		KMessageBox::sorry( this, i18n("You must be online to add a contact."), i18n("ICQ Plugin") );
		return false;
	}

	Q_ULONG uin = addUI->uinEdit->text().toULong();
	if ( uin < 1000 )
	{
		// Invalid (or missing) UIN
		KMessageBox::sorry( this, i18n("You must enter a valid UIN."), i18n("ICQ Plugin") );
		return false;
	}
	else
	{
		// UIN is valid
		return true;
	}
}

void ICQAddContactPage::showSearchDialog()
{
	if ( m_searchDialog )
		m_searchDialog->raise();
	else
	{
		m_searchDialog = new ICQSearchDialog( mAccount, this, "icqSearchDialog" );
		m_searchDialog->show();
		connect( m_searchDialog, SIGNAL( closeClicked() ), this, SLOT( searchDialogDestroyed() ) );
	}
}

void ICQAddContactPage::searchDialogDestroyed()
{
	QObject::disconnect( this, 0, m_searchDialog, 0 );
	m_searchDialog->deleteLater();
	m_searchDialog = NULL;
}


#include "icqaddcontactpage.moc"
// vim: set noet ts=4 sts=4 sw=4:
// kate: indent-mode csands; space-indent off; replace-tabs off;
