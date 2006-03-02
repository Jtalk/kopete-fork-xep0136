/*
    tooltipeditdialog.cpp  -  Kopete Tooltip Editor

    Copyright (c) 2004 by Stefan Gehn <metz AT gehn.net>
    Kopete    (c) 2004 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "tooltipeditdialog.h"
#include "tooltipeditwidget.h"

#include "kopetecontactproperty.h"
#include "kopeteglobal.h"
#include "kopeteappearancesettings.h"

#include <qapplication.h>
#include <qtoolbutton.h>
#include <qstringlist.h>

#include <kiconloader.h>
#include <klistview.h>
#include <klocale.h>

class TooltipItem : public KListViewItem
{
	public:
		TooltipItem(KListView *parent, const QString& label, const QString& propertyName)
			: KListViewItem(parent, label),
				mPropName(propertyName)
		{
		}

		TooltipItem(KListView *parent, Q3ListViewItem *item, const QString& label, const QString& propertyName)
			: KListViewItem(parent, item, label),
				mPropName(propertyName)
		{
		}

		QString propertyName() const { return mPropName; }
	private:
		QString mPropName;
};



TooltipEditDialog::TooltipEditDialog(QWidget *parent, const char*/* name*/)
	: KDialog(parent, i18n("Tooltip Editor"), Ok|Cancel)
{
	setDefaultButton(KDialog::Ok);
	enableButtonSeparator(true);

	mMainWidget = new TooltipEditWidget(this, "TooltipEditDialog::mMainWidget");
	setMainWidget(mMainWidget);
	mMainWidget->lstUsedItems->header()->hide();
	mMainWidget->lstUnusedItems->header()->hide();
	mMainWidget->lstUsedItems->setSorting( -1 );
	mMainWidget->lstUnusedItems->setSorting( 0 );

	const Kopete::ContactPropertyTmpl::Map propmap(
		Kopete::Global::Properties::self()->templateMap());
	QStringList usedKeys = Kopete::AppearanceSettings::self()->toolTipContents();

	connect(mMainWidget->lstUnusedItems, SIGNAL(doubleClicked ( Q3ListViewItem *, const QPoint &, int )), this, SLOT(slotAddButton()));
	connect(mMainWidget->lstUsedItems, SIGNAL(doubleClicked ( Q3ListViewItem *, const QPoint &, int )), this, SLOT(slotRemoveButton()));

	// first fill the "used" list
	foreach(QString usedProp, usedKeys)
	{
		if(propmap.contains(usedProp) && !propmap[usedProp].isPrivate())
		{
			new TooltipItem(mMainWidget->lstUsedItems, propmap[usedProp].label(), usedProp);
		}
	}

	// then iterate over all known properties and insert the remaining ones
	// into the "unused" list
	Kopete::ContactPropertyTmpl::Map::ConstIterator it;
	for(it = propmap.begin(); it != propmap.end(); ++it)
	{
		if((usedKeys.contains(it.key())==0) && (!it.data().isPrivate()))
			new TooltipItem(mMainWidget->lstUnusedItems, it.data().label(), it.key());
	}

	connect(mMainWidget->lstUnusedItems, SIGNAL(selectionChanged(Q3ListViewItem *)),
		this, SLOT(slotUnusedSelected(Q3ListViewItem *)));
	connect(mMainWidget->lstUsedItems, SIGNAL(selectionChanged(Q3ListViewItem *)),
		this, SLOT(slotUsedSelected(Q3ListViewItem *)));

	QIcon iconSet;
	iconSet = SmallIconSet("up");
	mMainWidget->tbUp->setIconSet(iconSet);
	mMainWidget->tbUp->setEnabled(false);
	mMainWidget->tbUp->setAutoRepeat(true);
	connect(mMainWidget->tbUp, SIGNAL(clicked()), SLOT(slotUpButton()));

	iconSet = SmallIconSet("down");
	mMainWidget->tbDown->setIconSet(iconSet);
	mMainWidget->tbDown->setEnabled(false);
	mMainWidget->tbDown->setAutoRepeat(true);
	connect(mMainWidget->tbDown, SIGNAL(clicked()), SLOT(slotDownButton()));

	iconSet = QApplication::isRightToLeft() ? SmallIconSet("back") : SmallIconSet("forward");
	mMainWidget->tbAdd->setIconSet(iconSet);
	mMainWidget->tbAdd->setEnabled(false);
	connect(mMainWidget->tbAdd, SIGNAL(clicked()), SLOT(slotAddButton()));

	iconSet = QApplication::isRightToLeft() ? SmallIconSet("forward") : SmallIconSet("back");
	mMainWidget->tbRemove->setIconSet(iconSet);
	mMainWidget->tbRemove->setEnabled(false);
	connect(mMainWidget->tbRemove, SIGNAL(clicked()), SLOT(slotRemoveButton()));

	connect(this, SIGNAL(okClicked()), this, SLOT(slotOkClicked()));

	resize(QSize(450, 450));
}

void TooltipEditDialog::slotOkClicked()
{
	QStringList oldList = Kopete::AppearanceSettings::self()->toolTipContents();
	QStringList newList;
	Q3ListViewItemIterator it(mMainWidget->lstUsedItems);
	QString keyname;

	while(it.current())
	{
		keyname = static_cast<TooltipItem *>(it.current())->propertyName();
		newList += keyname;
		kDebug(14000) << k_funcinfo <<
			"Adding key '" << keyname << "' to tooltip list" << endl;
		++it;
	}

	if(oldList != newList)
	{
		Kopete::AppearanceSettings::self()->setToolTipContents(newList);
		emit changed(true);
		kDebug(14000) << k_funcinfo << "tooltip fields changed, emitting changed()" << endl;
	}
}


void TooltipEditDialog::slotUnusedSelected(Q3ListViewItem *item)
{
	//mMainWidget->tbRemove->setEnabled(false);
	mMainWidget->tbAdd->setEnabled(item!=0);
}

void TooltipEditDialog::slotUsedSelected(Q3ListViewItem *item)
{
	mMainWidget->tbRemove->setEnabled(item!=0);
	//mMainWidget->tbAdd->setEnabled(false);
	if (item)
	{
		mMainWidget->tbUp->setEnabled(item->itemAbove() != 0);
		mMainWidget->tbDown->setEnabled(item->itemBelow() != 0);
	}
	else
	{
		mMainWidget->tbUp->setEnabled(false);
		mMainWidget->tbDown->setEnabled(false);
	}
}

void TooltipEditDialog::slotUpButton()
{
	Q3ListViewItem *item = mMainWidget->lstUsedItems->currentItem();
	Q3ListViewItem *prev = item->itemAbove();
	if(prev == 0) // we are first item already
		return;

	prev->moveItem(item);
	slotUsedSelected(item);
}

void TooltipEditDialog::slotDownButton()
{
	Q3ListViewItem *item = mMainWidget->lstUsedItems->currentItem();
	Q3ListViewItem *next = item->itemBelow();
	if(next == 0) // we are last item already
		return;

	item->moveItem(next);
	slotUsedSelected(item);
}

void TooltipEditDialog::slotAddButton()
{
	TooltipItem *item = static_cast<TooltipItem *>(mMainWidget->lstUnusedItems->currentItem());
	if(!item)
		return;
	//kDebug(14000) << k_funcinfo << endl;

	// build a new one in the "used" list
	new TooltipItem(mMainWidget->lstUsedItems, item->text(0), item->propertyName());

	// remove the old one from "unused" list
	mMainWidget->lstUnusedItems->takeItem(item);
	delete item;
}

void TooltipEditDialog::slotRemoveButton()
{
	TooltipItem *item = static_cast<TooltipItem *>(mMainWidget->lstUsedItems->currentItem());
	if(!item)
		return;
	//kDebug(14000) << k_funcinfo << endl;

	// build a new one in the "unused" list
	new TooltipItem(mMainWidget->lstUnusedItems, item->text(0), item->propertyName());

	// remove the old one from "used" list
	mMainWidget->lstUsedItems->takeItem(item);
	delete item;
}

#include "tooltipeditdialog.moc"
