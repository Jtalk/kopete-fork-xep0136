/*
    systemtray.cpp  -  Kopete Tray Dock Icon

    Copyright (c) 2002      by Nick Betcher           <nbetcher@kde.org>
    Copyright (c) 2002-2003 by Martijn Klingens       <klingens@kde.org>
    Copyright (c) 2003      by Olivier Goffart        <ogoffart@tiscalinet.be>

    Kopete    (c) 2002-2003 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "systemtray.h"

//#include <qpixmap.h>
#include <qtimer.h>
#include <qtooltip.h>
#include <qregexp.h>

#include <kwin.h>
#include <kaboutdata.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kiconloader.h>
#include "kopetemessagemanagerfactory.h"
#include "kopeteballoon.h"
#include "kopeteprefs.h"
#include "kopetemetacontact.h"
#include "kopeteaccount.h"

KopeteSystemTray* KopeteSystemTray::s_systemTray = 0L;

KopeteSystemTray* KopeteSystemTray::systemTray( QWidget *parent, const char* name )
{
	if( !s_systemTray )
		s_systemTray = new KopeteSystemTray( parent, name );

	return s_systemTray;
}

KopeteSystemTray::KopeteSystemTray(QWidget* parent, const char* name)
	: KSystemTray(parent,name)
{
//	kdDebug(14010) << "Creating KopeteSystemTray" << endl;
	QToolTip::add( this, kapp->aboutData()->shortDescription() );

	mIsBlinkIcon = false;
	mIsBlinking = false;
	mBlinkTimer = new QTimer(this, "mBlinkTimer");

	//mKopeteIcon = kapp->miniIcon();
#if KDE_IS_VERSION( 3, 1, 90 )
	mKopeteIcon = loadIcon( "kopete" );
#else
	mKopeteIcon = QPixmap( BarIcon( QString::fromLatin1( "kopete" ), 22 ) );
#endif

	connect(mBlinkTimer, SIGNAL(timeout()), this, SLOT(slotBlink()));
	connect(KopeteMessageManagerFactory::factory() , SIGNAL(newEvent(KopeteEvent*)),
		this, SLOT(slotNewEvent(KopeteEvent*)));
	connect(KopetePrefs::prefs(), SIGNAL(saved()), this, SLOT(slotConfigChanged()));

	setPixmap(mKopeteIcon);
	slotConfigChanged();

	m_balloon=0l;
}

KopeteSystemTray::~KopeteSystemTray()
{
//	kdDebug(14010) << "[KopeteSystemTray] ~KopeteSystemTray" << endl;
//	delete mBlinkTimer;
}

void KopeteSystemTray::mousePressEvent( QMouseEvent *me )
{
	if ( me->button() == QEvent::MidButton )
	{
		if ( mIsBlinking )
		{
			mouseDoubleClickEvent( me );
			return;
		}
	}
	KSystemTray::mousePressEvent( me );
}

void KopeteSystemTray::mouseDoubleClickEvent( QMouseEvent *me )
{
	if ( !mIsBlinking )
	{
		KSystemTray::mousePressEvent( me );
	}
	else
	{
		if(!mEventList.isEmpty())
			mEventList.first()->apply();
	}
}

void KopeteSystemTray::contextMenuAboutToShow( KPopupMenu *me )
{
	kdDebug(14010) << k_funcinfo << "Called." << endl;
	emit aboutToShowMenu( me );
}

void KopeteSystemTray::startBlink( const QString &icon )
{
	startBlink( KGlobal::iconLoader()->loadIcon( icon , KIcon::Panel ) );
}

void KopeteSystemTray::startBlink( const QPixmap &icon )
{
	mBlinkIcon = icon;
	if ( mBlinkTimer->isActive() == false )
	{
		mIsBlinkIcon = true;
		mIsBlinking = true;
		mBlinkTimer->start( 1000, false );
	}
	else
	{
		mBlinkTimer->stop();
		mIsBlinkIcon = true;
		mIsBlinking = true;
		mBlinkTimer->start( 1000, false );
	}
}

void KopeteSystemTray::startBlink( const QMovie &icon )
{
	kdDebug( 14010 ) << k_funcinfo << "starting movie." << endl;
	setMovie( icon );
	mIsBlinking = true;
}

void KopeteSystemTray::startBlink()
{
	if( mMovie.isNull() )
		mMovie = KGlobal::iconLoader()->loadMovie( QString::fromLatin1( "newmessage" ), KIcon::Panel );
	startBlink( mMovie );
}

void KopeteSystemTray::stopBlink()
{
	if(movie())
	{
		kdDebug(14010) << k_funcinfo << "stopping movie." << endl;
		setPixmap(mKopeteIcon);
		mIsBlinkIcon = false;
		mIsBlinking=false;
		return;
	}

	if (mBlinkTimer->isActive() == true)
	{
		mBlinkTimer->stop();
		setPixmap(mKopeteIcon);
		mIsBlinkIcon = false;
		mIsBlinking = false;

	}
	else
	{
		setPixmap(mKopeteIcon);
		mIsBlinkIcon = false;
		mIsBlinking = false;
	}
}

void KopeteSystemTray::slotBlink()
{
	if (mIsBlinkIcon == true)
	{
		setPixmap(mKopeteIcon);
		mIsBlinkIcon = false;
	}
	else
	{
		setPixmap(mBlinkIcon);
		mIsBlinkIcon = true;
	}
}

void KopeteSystemTray::slotNewEvent(KopeteEvent *event)
{
	mEventList.append( event );
	connect( event , SIGNAL(done(KopeteEvent*)) , this, SLOT(slotEventDone(KopeteEvent*)));

	//balloon
	addBalloon();

	//flash
	if ( KopetePrefs::prefs()->trayflashNotify() )
		startBlink();
}


void KopeteSystemTray::slotEventDone(KopeteEvent *event)
{
	bool current= event==mEventList.first();
	mEventList.remove(event);

	if(current && m_balloon)
	{
		m_balloon->deleteLater();
		m_balloon=0l;
		if(!mEventList.isEmpty())
			addBalloon();
	}

	if(mEventList.isEmpty())
		stopBlink();
}

void KopeteSystemTray::addBalloon()
{
	kdDebug(14010) << k_funcinfo << m_balloon << ":" << KopetePrefs::prefs()->showTray() << ":" << KopetePrefs::prefs()->balloonNotify()
			<< ":" << !mEventList.isEmpty() << endl;
	if( !m_balloon && KopetePrefs::prefs()->showTray() && KopetePrefs::prefs()->balloonNotify() && !mEventList.isEmpty() )
	{
		KopeteMessage msg = mEventList.first()->message();

		if ( msg.from() )
		{
			kdDebug(14010) << k_funcinfo << "Orig msg text=" << msg.parsedBody() << endl;
			
			QString msgText = squashMessage( msg );
			
			kdDebug(14010) << k_funcinfo << "New msg text=" << msgText << endl;

			QString msgFrom;
			if( msg.from()->metaContact() )
				msgFrom = msg.from()->metaContact()->displayName();
			else
				msgFrom = msg.from()->displayName();

			m_balloon = new KopeteBalloon(
				i18n( "<qt><nobr><b>New Message from %1:</b></nobr><br><nobr>\"%2\"</nobr></qt>" )
#if QT_VERSION < 0x030200
					.arg( msgFrom ).arg( msgText ),
#else
					.arg( msgFrom, msgText ),
#endif
				QString::null );
			connect(m_balloon, SIGNAL(signalBalloonClicked()), mEventList.first() , SLOT(apply()));
			connect(m_balloon, SIGNAL(signalButtonClicked()), mEventList.first() , SLOT(apply()));
			connect(m_balloon, SIGNAL(signalIgnoreButtonClicked()), mEventList.first() , SLOT(ignore()));
			m_balloon->setAnchor( KopeteSystemTray::systemTray()->mapToGlobal(pos()) );
			m_balloon->show();
			KWin::setOnAllDesktops(m_balloon->winId() , true);
		}
	}
}

void KopeteSystemTray::slotConfigChanged()
{
//	kdDebug(14010) << k_funcinfo << "called." << endl;
	if (KopetePrefs::prefs()->showTray())
		show();
	else
		hide(); // for users without kicker or a similar docking app
}

QString KopeteSystemTray::squashMessage( const KopeteMessage& msg )
{
	QString msgText = msg.parsedBody();

	QRegExp rx( "(<a.*>((http://)?(.+))</a>)" );
	rx.setMinimal( true );
	if ( rx.search( msgText ) == -1 )
	{
		// no URLs in text, just pick the first 30 chars of
		// the plain text if necessary
		msgText = msg.plainBody();
		if( msgText.length() > 30 )
			msgText = msgText.left( 30 ) + QString::fromLatin1( " ..." );
	}
	else
	{
		QString plainText = msg.plainBody();
		if ( plainText.length() > 30 )
		{
			QString fullUrl = rx.cap( 2 );
			QString shorterUrl;
			if ( fullUrl.length() > 30 )
			{
				QString urlWithoutProtocol = rx.cap( 4 );
				shorterUrl = urlWithoutProtocol.left( 27 ) 
						+ QString::fromLatin1( "... " );
			}
			else
			{
				shorterUrl = fullUrl.left( 27 ) 
						+ QString::fromLatin1( "... " );
			}
			// remove message text
			msgText = QString::fromLatin1( "... " ) +
					rx.cap( 1 ) +
					QString::fromLatin1( " ..." );
			// find last occurrence of URL (the one inside the <a> tag)
			int revUrlOffset = msgText.findRev( fullUrl );
			msgText.replace( revUrlOffset,
						fullUrl.length(), shorterUrl );
		}
	}
	return msgText;
}
#include "systemtray.moc"

// vim: set noet ts=4 sts=4 sw=4:

