/*  *************************************************************************
    *   copyright: (C) 2003 Richard L�rk�ng <nouseforaname@home.se>         *
    *   copyright: (C) 2003 Gav Wood <gav@kde.org>                          *
    *************************************************************************
*/

/*  *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef SMSSENDPROVIDER_H
#define SMSSENDPROVIDER_H

#include <qstring.h>
#include <qstringlist.h>
#include <qptrlist.h>
#include <qlabel.h>
#include <qvaluelist.h>

#include <klineedit.h>

#include "kopetemessage.h"

#include "smsaccount.h"

class KProcess;
class KopeteAccount;
class SMSContact;

class SMSSendProvider : public QObject
{
	Q_OBJECT
public:
	SMSSendProvider(const QString& providerName, const QString& prefixValue, KopeteAccount* account, QObject* parent = 0, const char* name = 0);
	~SMSSendProvider();

	void setAccount(KopeteAccount *account);

	int count();
	const QString& name(int i);
	const QString& value(int i);
	const QString& description(int i);
	const bool isHidden(int i);

	void save(QPtrList<KLineEdit>& args);
	void send(const KopeteMessage& msg);

	int maxSize();
private slots:
	void slotReceivedOutput(KProcess*, char  *buffer, int  buflen);
	void slotSendFinished(KProcess*);
private:
	QStringList names;
	QStringList descriptions;
	QStringList values;
	QValueList<bool> isHiddens;

	int messagePos;
	int telPos;
	int m_maxSize;

	QString provider;
	QString prefix;
	QCString output;

	KopeteAccount* m_account;

	KopeteMessage m_msg;

	bool canSend;
signals:
	void messageSent(const KopeteMessage& msg);
	void messageNotSent(const KopeteMessage& msg, const QString &error);
} ;

#endif //SMSSENDPROVIDER_H
