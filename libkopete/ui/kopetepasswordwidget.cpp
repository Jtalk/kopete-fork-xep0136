/*
    kopetepasswordwidget.cpp - widget for modifying a Kopete::Password

    Copyright (c) 2003 by Richard Smith          <kde@metafoo.co.uk>

    Kopete    (c) 2003 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include "kopetepasswordwidget.h"
#include "kopetepassword.h"

#include <kpassworddialog.h>

#include <qcheckbox.h>

class Kopete::UI::PasswordWidget::Private
{
public:
	int maxLength;
};

Kopete::UI::PasswordWidget::PasswordWidget( QWidget *parent, const char *name, Kopete::Password *from )
 : QWidget( parent ), d( new Private )
{
	setObjectName( name );

	setupUi( this );

	load( from );
}

Kopete::UI::PasswordWidget::~PasswordWidget()
{
	delete d;
}

void Kopete::UI::PasswordWidget::load( Kopete::Password *source )
{
	disconnect( mRemembered, SIGNAL( stateChanged( int ) ), this, SLOT( slotRememberChanged() ) );
	disconnect( mPassword, SIGNAL( textChanged( const QString & ) ), this, SIGNAL( changed() ) );
	disconnect( mRemembered, SIGNAL( stateChanged( int ) ), this, SIGNAL( changed() ) );

	if ( source && source->remembered() )
	{
		mRemembered->setTristate();
		mRemembered->setCheckState( Qt::Checked );
		source->requestWithoutPrompt( this, SLOT( receivePassword( const QString & ) ) );
	}
	else
	{
		mRemembered->setTristate( false );
		mRemembered->setCheckState( Qt::Unchecked );
	}

	if ( source )
		d->maxLength = source->maximumLength();
	else
		d->maxLength = 0;

	mPassword->setEnabled( false );

	connect( mRemembered, SIGNAL( stateChanged( int ) ), this, SLOT( slotRememberChanged() ) );
	connect( mPassword, SIGNAL( textChanged( const QString & ) ), this, SIGNAL( changed() ) );
	connect( mRemembered, SIGNAL( stateChanged( int ) ), this, SIGNAL( changed() ) );

	emit changed();
}

void Kopete::UI::PasswordWidget::slotRememberChanged()
{
	mRemembered->setTristate( false );
	mPassword->setEnabled( mRemembered->isChecked() );
}

void Kopete::UI::PasswordWidget::receivePassword( const QString &pwd )
{
	// pwd == null could mean user declined to open wallet
	// don't uncheck the remembered field in this case.
	if ( !pwd.isNull() && mRemembered->checkState() == Qt::PartiallyChecked )
	{
		mRemembered->setChecked( true );
		setPassword( pwd );
	}
}

void Kopete::UI::PasswordWidget::save( Kopete::Password *target )
{
	if ( !target || mRemembered->checkState() == Qt::PartiallyChecked )
		return;

	if ( mRemembered->isChecked() )
		target->set( password() );
	else
		target->set();
}

bool Kopete::UI::PasswordWidget::validate()
{
	if ( !mRemembered->isChecked() ) return true;
	if ( d->maxLength == 0 ) return true;
	return password().length() <= d->maxLength;
}

QString Kopete::UI::PasswordWidget::password() const
{
	return QString::fromLocal8Bit( mPassword->password() );
}

bool Kopete::UI::PasswordWidget::remember() const
{
	return mRemembered->checkState() == Qt::Checked;
}

void Kopete::UI::PasswordWidget::setPassword( const QString &pass )
{
	// switch out of 'waiting for wallet' mode if we're in it
	mRemembered->setTristate( false );

	// fill in the password text
	mPassword->erase();
	mPassword->insert( pass );
	mPassword->setEnabled( remember() );
}

#include "kopetepasswordwidget.moc"

// vim: set noet ts=4 sts=4 sw=4:
