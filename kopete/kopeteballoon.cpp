/*
    kopeteballoon.cpp  -  Nice Balloon

    Copyright (c) 2002      by Duncan Mac-Vicar Prett <duncan@kde.org>

    Kopete    (c) 2002-2003      by the Kopete developers  <kopete-devel@kde.org>

    Portions of this code based on Kim Applet code
    Copyright (c) 2000-2002 by Malte Starostik        <malte@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include <qpointarray.h>
#include <qpushbutton.h>
#include <qtooltip.h>
#include <qlayout.h>

#include <kdeversion.h>
#if KDE_IS_VERSION( 3, 1, 90 )
#include <kglobalsettings.h>
#endif

#include <kapplication.h>
#include <kdialog.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kactivelabel.h>

#include "kopeteballoon.h"
#include "systemtray.h"

KopeteBalloon::KopeteBalloon(const QString &text, const QString &pix)
: QWidget(0L, "KopeteBalloon", WStyle_StaysOnTop | WStyle_Customize |
	WStyle_NoBorder | WStyle_Tool | WX11BypassWM)
{
	setCaption("");

	QVBoxLayout *BalloonLayout = new QVBoxLayout(this, 22,
		KDialog::spacingHint(), "BalloonLayout");

	// BEGIN Layout1
	QHBoxLayout *Layout1 = new QHBoxLayout(BalloonLayout,
		KDialog::spacingHint(), "Layout1");
	//QLabel *mCaption = new QLabel(text, this, "mCaption");
	KActiveLabel *mCaption = new KActiveLabel(text, this, "mCaption");
	mCaption->setPalette(QToolTip::palette());
	mCaption->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );

	if (!pix.isEmpty())
	{
		QLabel *mImage = new QLabel(this, "mImage");
		mImage->setScaledContents(FALSE);
		mImage->setPixmap(locate("data", pix));

		Layout1->addWidget(mImage);
	}
	Layout1->addWidget(mCaption);
	// END Layout1


	// BEGIN Layout2
	QHBoxLayout *Layout2 = new QHBoxLayout(BalloonLayout,
		KDialog::spacingHint(), "Layout2");
	QPushButton *mViewButton = new QPushButton(i18n("to view", "View"), this,
		"mViewButton");
	QPushButton *mIgnoreButton = new QPushButton(i18n("Ignore"), this,
		"mIgnoreButton");

	Layout2->addStretch();
	Layout2->addWidget(mViewButton);
	Layout2->addWidget(mIgnoreButton);
	Layout2->addStretch();
	// END Layout2

	setPalette(QToolTip::palette());
	setAutoMask(TRUE);

	connect(mViewButton, SIGNAL(clicked()),
		this, SIGNAL(signalButtonClicked()));
	connect(mViewButton, SIGNAL(clicked()),
		this, SLOT(deleteLater()));
	connect(mIgnoreButton, SIGNAL(clicked()),
		this, SIGNAL(signalIgnoreButtonClicked()));
	connect(mIgnoreButton, SIGNAL(clicked()),
		this, SLOT(deleteLater()));
	connect(mCaption, SIGNAL(linkClicked(const QString &)),
		this, SIGNAL(signalIgnoreButtonClicked()));
	connect(mCaption, SIGNAL(linkClicked(const QString &)),
		this, SLOT(deleteLater()));
}

void KopeteBalloon::setAnchor(const QPoint &anchor)
{
	mAnchor = anchor;
	updateMask();
}

void KopeteBalloon::updateMask()
{
	QRegion mask(10, 10, width() - 20, height() - 20);

	QPoint corners[8] = {
		QPoint(width() - 50, 10),
		QPoint(10, 10),
		QPoint(10, height() - 50),
		QPoint(width() - 50, height() - 50),
		QPoint(width() - 10, 10),
		QPoint(10, 10),
		QPoint(10, height() - 10),
		QPoint(width() - 10, height() - 10)
	};

	for (int i = 0; i < 4; ++i)
	{
		QPointArray corner;
		corner.makeArc(corners[i].x(), corners[i].y(), 40, 40,
			i * 16 * 90, 16 * 90);
		corner.resize(corner.size() + 1);
		corner.setPoint(corner.size() - 1, corners[i + 4]);
		mask -= corner;
	}

	// get screen-geometry for screen our anchor is on
	// (geometry can differ from screen to screen!
	#if KDE_IS_VERSION( 3, 1, 90 )
		QRect deskRect = KGlobalSettings::desktopGeometry(mAnchor);
	#else
		QDesktopWidget* tmp = QApplication::desktop();
		QRect deskRect = tmp->screenGeometry(tmp->screenNumber(mAnchor));
	#endif

	bool bottom = (mAnchor.y() + height()) > ((deskRect.y() + deskRect.height()) - 48);
	bool right = (mAnchor.x() + width()) > ((deskRect.x() + deskRect.width()) - 48);

	QPointArray arrow(4);
	arrow.setPoint(0, QPoint(right ? width() : 0, bottom ? height() : 0));
	arrow.setPoint(1, QPoint(right ? width() - 10 : 10,
		bottom ? height() - 30 : 30));
	arrow.setPoint(2, QPoint(right ? width() - 30 : 30,
		bottom ? height() - 10 : 10));
	arrow.setPoint(3, arrow[0]);
	mask += arrow;
	setMask(mask);

	move(right ? mAnchor.x() - width() : ( mAnchor.x() < 0 ? 0 : mAnchor.x() ),
	     bottom ? mAnchor.y() - height() : ( mAnchor.y() < 0 ? 0 : mAnchor.y() ) );
}

#include "kopeteballoon.moc"
// vim: set noet ts=4 sts=4 sw=4:
