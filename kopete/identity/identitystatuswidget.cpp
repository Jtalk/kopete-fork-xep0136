/*
    identitystatuswidget.cpp  -  Kopete identity status configuration widget

    Copyright (c) 2007      by Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>

    Kopete    (c) 2003-2007 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/


#include "identitystatuswidget.h"
#include "ui_identitystatusbase.h"

#include <KIcon>
#include <KMenu>
#include <QTimeLine>
#include <QCursor>
#include <QUrl>
#include <QPalette>
#include <kopeteidentity.h>
#include <kopeteaccount.h>
#include <kopeteaccountmanager.h>
#include <kopetecontact.h>
#include <kopeteprotocol.h>
#include <avatardialog.h>
#include <KDebug>

class IdentityStatusWidget::Private
{
public:
	Kopete::Identity *identity;
	Ui::IdentityStatusBase ui;
	QTimeLine *timeline;
	QString photoPath;
	// Used to display changing nickname in red
	QString lastNickName;
};

IdentityStatusWidget::IdentityStatusWidget(Kopete::Identity *identity, QWidget *parent)
: QWidget(parent)
{
	d = new Private();
	d->identity = 0;
	
	// animation for showing/hiding
	d->timeline = new QTimeLine( 150, this );
	d->timeline->setCurveShape( QTimeLine::EaseInOutCurve );
	connect( d->timeline, SIGNAL(valueChanged(qreal)),
			 this, SLOT(slotAnimate(qreal)) );

	d->ui.setupUi(this);
	QWidget::setVisible( false );

	setIdentity(identity);
	slotLoad();

	// user input signals
	connect( d->ui.nickName, SIGNAL(editingFinished()), this, SLOT(slotSave()) );
	connect( d->ui.nickName, SIGNAL(textChanged(QString)), this, SLOT(slotNickNameTextChanged(QString)) );
	connect( d->ui.photo, SIGNAL(linkActivated(const QString&)), 
			 this, SLOT(slotPhotoLinkActivated(const QString &)));
	connect( d->ui.accounts, SIGNAL(linkActivated(const QString&)),
			 this, SLOT(slotAccountLinkActivated(const QString &)));
	connect( Kopete::AccountManager::self(),
			SIGNAL(accountOnlineStatusChanged(Kopete::Account*, const Kopete::OnlineStatus&, const Kopete::OnlineStatus&)),
			this, SLOT(slotUpdateAccountStatus()));
}

IdentityStatusWidget::~IdentityStatusWidget()
{
	delete d->timeline;
	delete d;
}

void IdentityStatusWidget::setIdentity(Kopete::Identity *identity)
{
	if (d->identity)
	{
		disconnect( d->identity, SIGNAL(identityChanged(Kopete::Identity*)), this, SLOT(slotUpdateAccountStatus()));
		slotSave();
	}

	d->identity = identity;
	slotLoad();

	if (d->identity)
	{
		connect( d->identity, SIGNAL(identityChanged(Kopete::Identity*)), this, SLOT(slotUpdateAccountStatus()));
	}
}

Kopete::Identity *IdentityStatusWidget::identity() const
{
	return d->identity;
}

void IdentityStatusWidget::setVisible( bool visible )
{
	// animate the widget disappearing
	d->timeline->setDirection( visible ?  QTimeLine::Forward
										: QTimeLine::Backward );
	d->timeline->start();
}

void IdentityStatusWidget::slotAnimate(qreal amount)
{
	if (amount == 0)
	{
		QWidget::setVisible( false );
		return;
	}
	
	if (amount == 1)
	{
		setFixedHeight( sizeHint().height() );
		d->ui.nickName->setFocus();
		return;
	}

	if (!isVisible())
		QWidget::setVisible( true );

	setFixedHeight( sizeHint().height() * amount );
}

void IdentityStatusWidget::slotLoad()
{
	// clear
	d->ui.photo->setText(QString("<a href=\"identity::getavatar\">%1</a>").arg(i18n("No Photo")));
	d->ui.nickName->clear();
	d->ui.identityStatus->clear();
	d->ui.identityName->clear();
	d->ui.accounts->clear();

	if (!d->identity)
		return;

	Kopete::Global::Properties *props = Kopete::Global::Properties::self();
	
	// photo
	if (d->identity->hasProperty(props->photo().key()))
	{
		d->photoPath = d->identity->property(props->photo()).value().toString();
		d->ui.photo->setText(QString("<a href=\"identity::getavatar\"><img src=\"%1\" width=48 height=48></a>")
								.arg(d->photoPath));
	}

	// nickname
	if (d->identity->hasProperty(props->nickName().key()))
	{
		// Set lastNickName to make red highlighting works when editing the identity nickname
		d->lastNickName = d->identity->property(props->nickName()).value().toString();

		d->ui.nickName->setText( d->lastNickName );
	}

	d->ui.identityName->setText(d->identity->label());

	//acounts
	slotUpdateAccountStatus();
	//TODO: online status
	
}

void IdentityStatusWidget::slotNickNameTextChanged(const QString &text)
{
	if ( d->lastNickName != text )
	{
		QPalette palette;
		palette.setColor(d->ui.nickName->foregroundRole(), Qt::red);
		d->ui.nickName->setPalette(palette);
	}
	else
	{
		// If the nickname is the same, reset the palette
		d->ui.nickName->setPalette(QPalette());
	}

}

void IdentityStatusWidget::slotSave()
{
	if (!d->identity)
		return;

	Kopete::Global::Properties *props = Kopete::Global::Properties::self();

	// photo
	if (!d->identity->hasProperty(props->photo().key()) ||
		d->identity->property(props->photo()).value().toString() != d->photoPath)
	{
		d->identity->setProperty(props->photo(), d->photoPath);
	}

	// nickname
	if (!d->identity->hasProperty(props->nickName().key()) ||
		d->identity->property(props->photo()).value().toString() != d->ui.nickName->text())
	{
		d->identity->setProperty(props->nickName(), d->ui.nickName->text());

		// Set last nickname to the new identity nickname
		// and reset the palette
		d->lastNickName = d->ui.nickName->text();
		d->ui.nickName->setPalette(QPalette());
	}

	//TODO check what more to do

}

void IdentityStatusWidget::slotAccountLinkActivated(const QString &link)
{
	// Account links are in the form:
	// accountmenu:protocolId:accountId
	QStringList args = link.split(":");
	if (args[0] != "accountmenu")
		return;

	Kopete::Account *a = Kopete::AccountManager::self()->findAccount(QUrl::fromPercentEncoding(args[1].toAscii()), 
																	 QUrl::fromPercentEncoding(args[2].toAscii()));
	if (!a)
		return;

	KActionMenu *actionMenu = a->actionMenu();
	actionMenu->menu()->exec(QCursor::pos());
	delete actionMenu;
}

void IdentityStatusWidget::slotPhotoLinkActivated(const QString &link)
{
	if (link == "identity::getavatar")
	{
		d->photoPath = Kopete::UI::AvatarDialog::getAvatar(this, d->photoPath);
		slotSave();
		slotLoad();
	}
}


void IdentityStatusWidget::slotUpdateAccountStatus()
{
  if (!d->identity)
  {
    // no identity or already destroyed, ignore
    return;
  }

  // Always clear text before changing it: otherwise icon changes are not reflected
  d->ui.accounts->clear();

  QString text("<qt>");
  foreach(Kopete::Account *a, d->identity->accounts())
  {
	  Kopete::Contact *self = a->myself();
	  QString onlineStatus = self ? self->onlineStatus().description() : i18n("Offline");
	  text += i18nc( "Account tooltip information: <nobr>ICON <b>PROTOCOL:</b> NAME (<i>STATUS</i>)<br/>",
				    "<nobr><a href=\"accountmenu:%2:%3\"><img src=\"kopete-account-icon:%2:%3\"> %1 (<i>%4</i>)</a><br/>",
		a->accountLabel(), QString(QUrl::toPercentEncoding( a->protocol()->pluginId() )),
		QString(QUrl::toPercentEncoding( a->accountId() )), onlineStatus );
  }
  
  text += QLatin1String("</qt>");
  d->ui.accounts->setText( text );

  // Adding/removing accounts changes needed size
  setFixedHeight( sizeHint().height() );
}

#include "identitystatuswidget.moc"
// vim: set noet ts=4 sts=4 sw=4:
