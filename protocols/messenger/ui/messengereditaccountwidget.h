/*
    messengereditaccountwidget.h - Messenger Account Widget

    Copyright (c) 2007      by Zhang Panyong		<pyzhang@gmail.com>
    Kopete    (c) 2002-2003 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/
#ifndef MESSENGEREDITACCOUNTWIDEGET_H
#define MESSENGEREDITACCOUNTWIDEGET_H

#include <qwidget.h>

#include "editaccountwidget.h"

namespace Kopete { class Account; }

class MessengerProtocol;
class MessengerEditAccountWidgetPrivate;

/*Messenger Edit Account Widget Class*/
class MessengerEditAccountWidget : public QWidget, public KopeteEditAccountWidget
{
	Q_OBJECT

public:
	explicit MessengerEditAccountWidget( Kopete::Account *account );
	~MessengerEditAccountWidget();
	virtual bool validateData();
	virtual Kopete::Account * apply();

private slots:
	void slotAllow();
	void slotBlock();
	void slotShowReverseList();
	void slotSelectImage();
	void slotOpenRegister();

private:
	MessengerEditAccountWidgetPrivate *d;
	MessengerAccount	*m_account;
};

#endif

// vim: set noet ts=4 sts=4 sw=4:
