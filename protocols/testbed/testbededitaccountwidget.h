/*
    testbededitaccountwidget.h - Kopete Testbed Protocol

    Copyright (c) 2003      by Will Stephenson		 <will@stevello.free-online.co.uk>
    Kopete    (c) 2002-2003 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU General Public                   *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#ifndef TESTBEDEDITACCOUNTWIDGET_H
#define TESTBEDEDITACCOUNTWIDGET_H

#include <qwidget.h>
#include <editaccountwidget.h>

class QVBoxLayout;
class KopeteAccount;
class TestbedAccountPreferences;

/**
 * A widget for editing this protocol's accounts
 * @author Will Stephenson
*/
class TestbedEditAccountWidget : public QWidget, public KopeteEditAccountWidget
{
Q_OBJECT
public:
    TestbedEditAccountWidget( QWidget* parent, KopeteAccount* account);

    ~TestbedEditAccountWidget();

	/**
	 * Make an account out of the entered data
	 */
	virtual KopeteAccount* apply();
	/**
	 * Is the data correct?
	 */
	virtual bool validateData();
protected:
	KopeteAccount *m_account;
	QVBoxLayout *m_layout;
	TestbedAccountPreferences *m_preferencesDialog;
};

#endif
