 /*
  * jinglecontentdialog.cpp - A dialog which asks the user to accept contents.
  *
  * Copyright (c) 2008 by Detlev Casanova <detlev.casanova@gmail.com>
  *
  * Kopete    (c) by the Kopete developers  <kopete-devel@kde.org>
  *
  * *************************************************************************
  * *                                                                       *
  * * This program is free software; you can redistribute it and/or modify  *
  * * it under the terms of the GNU General Public License as published by  *
  * * the Free Software Foundation; either version 2 of the License, or     *
  * * (at your option) any later version.                                   *
  * *                                                                       *
  * *************************************************************************
  */
#include "jinglecontentdialog.h"
#include <QVBoxLayout>
#include <QLabel>

using namespace XMPP;

static QString typeToString(JingleContent::Type t)
{
	switch (t)
	{
	case JingleContent::Audio :
		return "Audio";
		break;
	case JingleContent::Video :
		return "Video";
		break;
	case JingleContent::FileTransfer :
		return "File Transfer";
		break;
	case JingleContent::Unknown :
		return "Unknown";
		break;
	}
	return "";
}

JingleContentDialog::JingleContentDialog(QWidget *parent)
 : QDialog(parent)
{
	ui.setupUi(this);
}

JingleContentDialog::~JingleContentDialog()
{
	for (int i = 0; i < m_checkBoxes.count(); i++)
	{
		delete m_checkBoxes[i];
	}
}

void JingleContentDialog::setContents(QList<JingleContent*> c)
{
	for (int i = 0; i < c.count(); i++)
	{
		QCheckBox *cb = new QCheckBox(typeToString(c[i]->type()), this);
		cb->setChecked(true);
		if (c[i]->type() == JingleContent::Unknown)
		{
			cb->setChecked(false);
			cb->setEnabled(false);
		}
		m_contentNames << c[i]->name();
		ui.verticalLayout->insertWidget(0, cb);
		m_checkBoxes << cb;
	}
	QLabel *label = new QLabel("Check the contents you wanna accept : ", this);
	ui.verticalLayout->insertWidget(0, label);
}

void JingleContentDialog::setSession(JingleSession *s)
{
	m_session = s;
	setWindowTitle(QString("New Jingle session from ") + s->to().full());
	setContents(s->contents());
}

QStringList JingleContentDialog::checked()
{
	QStringList ret;
	for (int i = 0; i < m_checkBoxes.count(); i++)
	{
		if (m_checkBoxes[i]->checkState() == Qt::Checked)
		{
			qDebug() << "JingleContentDialog::checked() : checked : " << m_contentNames.at(i);
			ret << m_contentNames.at(i);
		}
	}
	return ret;
}

QStringList JingleContentDialog::unChecked()
{
	QStringList ret;
	for (int i = 0; i < m_checkBoxes.count(); i++)
	{
		if (m_checkBoxes[i]->checkState() == Qt::Unchecked)
		{
			qDebug() << "JingleContentDialog::unChecked() : unchecked : " << m_contentNames.at(i);
			ret << m_contentNames.at(i);
		}
	}
	return ret;
}

JingleSession *JingleContentDialog::session()
{
	return m_session;
}

