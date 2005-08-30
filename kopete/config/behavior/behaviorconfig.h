/*
    behaviorconfig.h  -  Kopete Look Feel Config

    Kopete    (c) 2002-2005 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef __BEHAVIOR_H
#define __BEHAVIOR_H

#include "kcmodule.h"

class QFrame;
class QTabWidget;

class BehaviorConfig_General;
class BehaviorConfig_Notifications;
class BehaviorConfig_Chat;
class KopeteAwayConfigBaseUI;
class KPluginInfo;

class BehaviorConfig : public KCModule
{
	Q_OBJECT

	public:
		BehaviorConfig(QWidget *parent, const char *  name , const QStringList &args) ;

		virtual void save();
		virtual void load();

	private slots:
		void slotSettingsChanged(bool);
		void slotValueChanged(int);
		void slotUpdatePluginLabel(int);

	private:
		QTabWidget* mBehaviorTabCtl;
		BehaviorConfig_General *mPrfsGeneral;
		BehaviorConfig_Notifications *mPrfsNotifications;
		BehaviorConfig_Chat *mPrfsChat;
		KopeteAwayConfigBaseUI *mAwayConfigUI;
		QValueList<KPluginInfo*> viewPlugins;
};
#endif
// vim: set noet ts=4 sts=4 sw=4:
