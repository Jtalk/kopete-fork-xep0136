/***************************************************************************
                          Netmeetingpreferences.cpp  -  description
                             -------------------
    copyright            : (C) 2004 by Olivier Goffart
    email                : ogoffart @ kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qlayout.h>
#include <qcheckbox.h>
//Added by qt3to4:
#include <QVBoxLayout>

#include <kcombobox.h>
#include <klineedit.h>
#include <kparts/componentfactory.h>
#include <klocale.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kcombobox.h>
#include <k3listview.h>
#include <kgenericfactory.h>
#include <kcolorbutton.h>
#include <kinputdialog.h>
#include <kurlrequester.h>
#include <kregexpeditorinterface.h>
#include <kdebug.h>

#include "netmeetingplugin.h"
#include "ui_netmeetingprefs_ui.h"
#include "netmeetingpreferences.h"

typedef KGenericFactory<NetmeetingPreferences> NetmeetingPreferencesFactory;
K_EXPORT_COMPONENT_FACTORY( kcm_kopete_netmeeting, NetmeetingPreferencesFactory( "kcm_kopete_netmeeting" )  )

NetmeetingPreferences::NetmeetingPreferences(QWidget *parent, const char* /*name*/, const QStringList &args)
							: KCModule(NetmeetingPreferencesFactory::instance(), parent, args)
{
	QVBoxLayout* l = new QVBoxLayout(this);
	QWidget* w = new QWidget;
	preferencesDialog = new Ui::NetmeetingPrefsUI(this);
	preferencesDialog->setupUi(w);
	l->addWidget(w);

	connect(preferencesDialog->m_app , SIGNAL(textChanged(const QString &)) , this , SLOT(slotChanged()));

	load();
}

NetmeetingPreferences::~NetmeetingPreferences()
{
	delete preferencesDialog;
}

void NetmeetingPreferences::load()
{
	KConfig *config=KGlobal::config();
	config->setGroup("Netmeeting Plugin");
	preferencesDialog->m_app->setCurrentText(config->readEntry("NetmeetingApplication","gnomemeeting -c callto://%1"));
	emit KCModule::changed(false);
}

void NetmeetingPreferences::save()
{
	KConfig *config=KGlobal::config();
	config->setGroup("Netmeeting Plugin");
	config->writeEntry("NetmeetingApplication",preferencesDialog->m_app->currentText());
	emit KCModule::changed(false);
}


void NetmeetingPreferences::slotChanged()
{
	emit KCModule::changed(true);
}

#include "netmeetingpreferences.moc"

// vim: set noet ts=4 sts=4 sw=4:
