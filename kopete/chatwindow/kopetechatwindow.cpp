/*
    kopetechatwindow.cpp - Chat Window

    Copyright (c) 2002      by Olivier Goffart       <ogoffart@tiscalinet.be>
    Copyright (C) 2002      by James Grant
    Copyright (c) 2002      by Stefan Gehn           <metz AT gehn.net>
    Copyright (c) 2002-2003 by Martijn Klingens      <klingens@kde.org>

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

#include <qtimer.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qfileinfo.h>

#include <kcursor.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kconfig.h>
#include <kpopupmenu.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <kwin.h>
#include <ktempfile.h>
#include <kkeydialog.h>
#include <kedittoolbar.h>
#include <kstatusbar.h>
#include <kpushbutton.h>
#include <ktabwidget.h>
#include <kstandarddirs.h>
#include <kdialog.h>
#include <kstringhandler.h>

#include "chatview.h"
#include "kopetechatwindow.h"
#include "kopeteemoticonaction.h"
#include "kopetegroup.h"
#include "kopetemessagemanager.h"
#include "kopetemetacontact.h"
#include "kopetepluginmanager.h"
#include "kopeteprefs.h"
#include "kopeteprotocol.h"
#include "kopetestdaction.h"
#include "kopeteviewmanager.h"

#if QT_VERSION >= 0x030200
	#include <qtoolbutton.h>
#endif

#if KDE_IS_VERSION( 3, 1, 90 )
	#include <kactionclasses.h>
#else
	#include "loadmovie.h"
#endif

static int MESSAGE_STATUS_ID = 1;

typedef QMap<KopeteAccount*,KopeteChatWindow*> AccountMap;
typedef QMap<KopeteGroup*,KopeteChatWindow*> GroupMap;
typedef QMap<KopeteMetaContact*,KopeteChatWindow*> MetaContactMap;
typedef QPtrList<KopeteChatWindow> WindowList;

namespace
{
	AccountMap accountMap;
	GroupMap groupMap;
	MetaContactMap mcMap;
	WindowList windows;
}

KopeteChatWindow *KopeteChatWindow::window( KopeteMessageManager *manager )
{
	bool windowCreated = false;
	KopeteChatWindow *myWindow;

	//Take the first and the first? What else?
	KopeteGroup *g = 0L;
	KopeteContactPtrList members = manager->members();
	KopeteMetaContact *m = members.first()->metaContact();

	//Don't do group by group for temporary contacts  //Why not? -Olivier
	if(m && !m->isTemporary() )
	{
		KopeteGroupList gList = m->groups();
		g = gList.first();
	}

	switch( KopetePrefs::prefs()->chatWindowPolicy() )
	{
		case GROUP_BY_ACCOUNT: //Open chats in the same protocol in the same window
			if( accountMap.contains( manager->account() ) )
				myWindow = accountMap[ manager->account() ];
			else
				windowCreated = true;
			break;

		case GROUP_BY_GROUP: //Open chats in the same group in the same window
			if( g && groupMap.contains( g ) )
				myWindow = groupMap[ g ];
			else
				windowCreated = true;
			break;

		case GROUP_BY_METACONTACT: //Open chats in the same metacontact in the same window
			if( mcMap.contains( m ) )
				myWindow = mcMap[ m ];
			else
				windowCreated = true;
			break;

		case GROUP_ALL: //Open all chats in the same window
			if( windows.isEmpty() )
				windowCreated = true;
			else
			{
				//Here we are finding the window with the most tabs and
				//putting it there. Need this for the cases where config changes
				//midstream

				int viewCount = -1;
				for ( KopeteChatWindow *thisWindow = windows.first(); thisWindow; thisWindow = windows.next() )
				{
					if( thisWindow->chatViewCount() > viewCount )
					{
						myWindow = thisWindow;
						viewCount = thisWindow->chatViewCount();
					}
				}
			}
			break;

		case NEW_WINDOW: //Open every chat in a new window
		default:
			windowCreated = true;
			break;
	}

	if( windowCreated )
	{
		myWindow = new KopeteChatWindow();

		if( !accountMap.contains( manager->account() ) )
			accountMap.insert( manager->account(), myWindow );

		if( !mcMap.contains( m ) )
			mcMap.insert( m, myWindow );

		if( g && !groupMap.contains( g ) )
			groupMap.insert( g, myWindow );
	}

//	kdDebug( 14010 ) << k_funcinfo << "Open Windows: " << windows.count() << endl;

	return myWindow;
}

KopeteChatWindow::KopeteChatWindow(QWidget *parent, const char* name) : KParts::MainWindow(parent, name)
{
	m_activeView = 0L;
	m_popupView = 0L;
	backgroundFile = 0L;
	updateBg = true;
	initActions();
	m_tabBar = 0L;

	vBox = new QVBox( this );
	vBox->setLineWidth( 0 );
	vBox->setSpacing( 0 );
	vBox->setFrameStyle( QFrame::NoFrame );
	setCentralWidget( vBox );

	mainArea = new QFrame( vBox );
	mainArea->setLineWidth( 0 );
	mainArea->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );
	mainLayout = new QVBoxLayout( mainArea );

	if( KopetePrefs::prefs()->chatWShowSend() )
	{
		//Send Button
		m_button_send = new KPushButton( i18n("Send"), statusBar() );
		m_button_send->setSizePolicy( QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum ) );
		m_button_send->setEnabled( false );
		m_button_send->setFont( statusBar()->font() );
		m_button_send->setFixedHeight( statusBar()->sizeHint().height() );
		connect( m_button_send, SIGNAL( pressed() ), this, SLOT( slotSendMessage() ) );
		statusBar()->addWidget( m_button_send, 0, true );
	}
	else
		m_button_send = 0L;

	statusBar()->insertItem( i18n("Ready."), MESSAGE_STATUS_ID, 1, false );
	statusBar()->setItemAlignment( MESSAGE_STATUS_ID, AlignLeft | AlignVCenter );

	readOptions();
	setWFlags( Qt::WDestructiveClose );

	windows.append( this );

//	kdDebug( 14010 ) << k_funcinfo << "Open Windows: " << windows.count() << endl;
}

KopeteChatWindow::~KopeteChatWindow()
{
	//	kdDebug( 14010 ) << k_funcinfo << endl;

	emit( closing( this ) );

	for( AccountMap::Iterator it = accountMap.begin(); it != accountMap.end(); )
	{
		AccountMap::Iterator mayDeleteIt = it;
		++it;
		if( mayDeleteIt.data() == this )
			accountMap.remove( mayDeleteIt.key() );
	}

	for( GroupMap::Iterator it = groupMap.begin(); it != groupMap.end(); )
	{
		GroupMap::Iterator mayDeleteIt = it;
		++it;
		if( mayDeleteIt.data() == this )
			groupMap.remove( mayDeleteIt.key() );
	}

	for( MetaContactMap::Iterator it = mcMap.begin(); it != mcMap.end(); )
	{
		MetaContactMap::Iterator mayDeleteIt = it;
		++it;
		if( mayDeleteIt.data() == this )
			mcMap.remove( mayDeleteIt.key() );
	}

	windows.remove( this );

//	kdDebug( 14010 ) << "Open Windows: " << windows.count() << endl;

	saveOptions();

	if( backgroundFile )
	{
		backgroundFile->close();
		backgroundFile->unlink();
		delete backgroundFile;
	}

	delete anim;
}

bool KopeteChatWindow::eventFilter( QObject *o, QEvent *e )
{
	if ( o->inherits( "KTextEdit" ) )
		KCursor::autoHideEventFilter( o, e );

	if( e->type() == QEvent::KeyPress )
	{
		QKeyEvent *event = static_cast<QKeyEvent*>( e );
		KKey key( event );

		// NOTE:
		// shortcut.contains( key ) doesn't work. It was the old way we used to do it, but it is incorrect
		// because if you have a multi-key shortcut then pressing any of the keys in
		// that shortcut individually causes the shortcut to be activated.

		if( chatSend->isEnabled() )
		{
			for( uint i = 0; i < chatSend->shortcut().count(); i++ )
			{
				if( key == chatSend->shortcut().seq(i).key(0) )
				{
					slotSendMessage();
					return true;
				}
			}
		}

		for( uint i = 0; i < nickComplete->shortcut().count(); i++ )
		{
			if( key == nickComplete->shortcut().seq(i).key(0) )
			{
				slotNickComplete();
				return true;
			}
		}

		if( historyDown->isEnabled() )
		{
			for( uint i = 0; i < historyDown->shortcut().count(); i++ )
			{
				if( key == historyDown->shortcut().seq(i).key(0) )
				{
					slotHistoryDown();
					return true;
				}
			}
		}

		if( historyUp->isEnabled() )
		{
			for( uint i = 0; i < historyUp->shortcut().count(); i++ )
			{
				if( key == historyUp->shortcut().seq(i).key(0) )
				{
					slotHistoryUp();
					return true;
				}
			}
		}

		if( m_activeView )
		{
			if( event->key() == Qt::Key_Prior )
			{
				m_activeView->pageUp();
				return true;
			}
			else if( event->key() == Qt::Key_Next )
			{
				m_activeView->pageDown();
				return true;
			}
		}
	}

	return false;
}

void KopeteChatWindow::slotNickComplete()
{
	if( m_activeView )
		m_activeView->nickComplete();
}

void KopeteChatWindow::slotTabContextMenu( QWidget *tab, const QPoint &pos )
{
	m_popupView = static_cast<ChatView*>( tab );

	KPopupMenu *p = new KPopupMenu;
	p->insertTitle( KStringHandler::rsqueeze( m_popupView->caption() ) );

	actionContactMenu->plug(p);
	p->insertSeparator();
	actionTabPlacementMenu->plug( p );
	tabDetach->plug( p );
	actionDetachMenu->plug( p );
	tabClose->plug( p );
	p->exec( pos );
	delete p;
	m_popupView = 0;
}

ChatView *KopeteChatWindow::activeView()
{
	return m_activeView;
}

void KopeteChatWindow::initActions(void)
{
	KActionCollection *coll = actionCollection();

#if KDE_IS_VERSION( 3, 1, 90 )
	createStandardStatusBarAction();
#else
	mStatusbarAction = KStdAction::showStatusbar(this,
			SLOT(slotToggleStatusBar()), actionCollection());
#endif

	chatSend = new KAction( i18n( "&Send Message" ), QString::fromLatin1( "mail_send" ), 0,
		this, SLOT( slotSendMessage() ), coll, "chat_send" );
	//Default to "send" shortcut as used by KMail and KNode
	chatSend->setShortcut( QKeySequence(CTRL + Key_Return) );
	chatSend->setEnabled( false );

	KStdAction::save ( this, SLOT(slotChatSave()), coll );
	KStdAction::print ( this, SLOT(slotChatPrint()), coll );
	KStdAction::quit ( this, SLOT(close()), coll );

	tabClose = KStdAction::close ( this, SLOT(slotChatClosed()), coll, "tabs_close" );

	tabLeft = KStdAction::back( this, SLOT( slotPreviousTab() ), coll, "tabs_left" );
	tabLeft->setText( i18n( "&Previous Chat" ) );
	tabLeft->setEnabled( false );

	tabRight = KStdAction::forward( this, SLOT( slotNextTab() ), coll, "tabs_right" );
	tabRight->setText( i18n( "&Next Chat" ) );
	tabRight->setEnabled( false );

	nickComplete = new KAction( i18n( "Nic&k Completion" ), QString::null, 0, this, SLOT( slotNickComplete() ), coll , "nick_compete");
	nickComplete->setShortcut( QKeySequence( Key_Tab ) );

	tabDetach = new KAction( i18n( "&Detach Chat" ), QString::fromLatin1( "tab_breakoff" ), 0,
		this, SLOT( slotDetachChat() ), coll, "tabs_detach" );
	tabDetach->setEnabled( false );

	actionDetachMenu = new KActionMenu( i18n( "&Move Tab to Window" ), QString::fromLatin1( "tab_breakoff" ), coll, "tabs_detachmove" );
	actionDetachMenu->setDelayed( false );

	connect ( actionDetachMenu->popupMenu(), SIGNAL(aboutToShow()), this, SLOT(slotPrepareDetachMenu()) );
	connect ( actionDetachMenu->popupMenu(), SIGNAL(activated(int)), this, SLOT(slotDetachChat(int)) );

	actionTabPlacementMenu = new KActionMenu( i18n( "&Tab Placement" ), coll, "tabs_placement" );
	connect ( actionTabPlacementMenu->popupMenu(), SIGNAL(aboutToShow()), this, SLOT(slotPreparePlacementMenu()) );
	connect ( actionTabPlacementMenu->popupMenu(), SIGNAL(activated(int)), this, SLOT(slotPlaceTabs(int)) );

	tabDetach->setShortcut( QKeySequence(CTRL + SHIFT + Key_B) );

	KStdAction::cut( this, SLOT(slotCut()), coll);
	KStdAction::copy( this, SLOT(slotCopy()), coll);
	KStdAction::paste( this, SLOT(slotPaste()), coll);

	new KAction( i18n( "Set Default &Font..." ), QString::fromLatin1( "charset" ), 0, this, SLOT( slotSetFont() ), coll, "format_font" );
	new KAction( i18n( "Set Default Text &Color..." ), QString::fromLatin1( "pencil" ), 0, this, SLOT( slotSetFgColor() ), coll, "format_fgcolor" );
	new KAction( i18n( "Set &Background Color..." ), QString::fromLatin1( "fill" ), 0, this, SLOT( slotSetBgColor() ), coll, "format_bgcolor" );

	historyUp = new KAction( i18n( "Previous History" ), QString::null, 0,
		this, SLOT( slotHistoryUp() ), coll, "history_up" );
	historyUp->setShortcut( QKeySequence(CTRL + Key_Up) );

	historyDown = new KAction( i18n( "Next History" ), QString::null, 0,
		this, SLOT( slotHistoryDown() ), coll, "history_down" );
	historyDown->setShortcut( QKeySequence(CTRL + Key_Down) );

	KStdAction::showMenubar( this, SLOT(slotViewMenuBar()), coll );

	membersLeft = new KToggleAction( i18n( "Place to Left of Chat Area" ), QString::null, 0,
		this, SLOT( slotViewMembersLeft() ), coll, "options_membersleft" );
	membersRight = new KToggleAction( i18n( "Place to Right of Chat Area" ), QString::null, 0,
		this, SLOT( slotViewMembersRight() ), coll, "options_membersright" );
	toggleMembers = new KToggleAction( i18n( "Show" ), QString::null, 0,
		this, SLOT( slotToggleViewMembers() ), coll, "options_togglemembers" );
	//toggleMembers->setChecked( true );

	actionSmileyMenu = new KopeteEmoticonAction( coll, "format_smiley" );
	actionSmileyMenu->setDelayed( false );
	connect(actionSmileyMenu, SIGNAL(activated(const QString &)), this, SLOT(slotSmileyActivated(const QString &)));

	//actionActionMenu = new KActionMenu(i18n("&Actions"), coll, "actions_menu" );
	//connect ( actionActionMenu->popupMenu(), SIGNAL(aboutToShow()), this, SLOT(slotPrepareActionMenu()) );
//	connect ( actionActionMenu->popupMenu(), SIGNAL(activated(int)), this, SLOT(slotActionActivated(int)) );

	actionContactMenu = new KActionMenu(i18n("Co&ntacts"), coll, "contacts_menu" );
	actionContactMenu->setDelayed( false );
	connect ( actionContactMenu->popupMenu(), SIGNAL(aboutToShow()), this, SLOT(slotPrepareContactMenu()) );

	// add configure key bindings menu item
	KStdAction::keyBindings(this, SLOT(slotConfKeys()), coll);
	KStdAction::configureToolbars(this, SLOT(slotConfToolbar()), coll);
	KopeteStdAction::preferences( coll );

	//The Sending movie
	normalIcon = QPixmap( BarIcon( QString::fromLatin1( "kopete" ) ) );
#if KDE_IS_VERSION(3, 1, 90)
	animIcon = KGlobal::iconLoader()->loadMovie( QString::fromLatin1( "newmessage" ), KIcon::Toolbar);
#else
	animIcon = KopeteCompat::loadMovie( QString::fromLatin1( "newmessage" ), KIcon::Toolbar);
#endif

	// Pause the animation because otherwise it's running even when we're not
	// showing it. This eats resources, and also triggers a pixmap leak in
	// QMovie in at least Qt 3.1, Qt 3.2 and the current Qt 3.3 beta
	animIcon.pause();

	// we can't set the tool bar as parent, if we do, it will be deleted when we configure toolbars
	anim = new QLabel( QString::null, 0L ,"kde toolbar widget" );
	anim->setMargin(5);
	anim->setPixmap( normalIcon );

	new KWidgetAction( anim , i18n("Toolbar Animation") , 0, 0 , 0 , coll , "toolbar_animation");

	//toolBar()->insertWidget( 99, anim->width(), anim );
	//toolBar()->alignItemRight( 99 );
	setStandardToolBarMenuEnabled( true );

	setXMLFile( QString::fromLatin1( "kopetechatwindow.rc" ) );
	createGUI( 0L );

#if !KDE_IS_VERSION( 3, 1, 90 )
	mStatusbarAction->setChecked(!statusBar()->isHidden());
#endif
}

const QString KopeteChatWindow::fileContents( const QString &path ) const
{
 	QString contents;
	QFile file( path );
	if ( file.open( IO_ReadOnly ) )
	{
		QTextStream stream( &file );
		contents = stream.read();
		file.close();
	}

	return contents;
}

void KopeteChatWindow::slotStopAnimation( ChatView* view )
{
	if( view == m_activeView )
		anim->setPixmap( normalIcon );
}

void KopeteChatWindow::setSendEnabled( bool enabled )
{
	chatSend->setEnabled( enabled );
	if(m_button_send)
		m_button_send->setEnabled( enabled );
}

void KopeteChatWindow::updateMembersActions()
{
	if( m_activeView )
	{
		const KDockWidget::DockPosition pos = m_activeView->membersListPosition();
		bool visibleMembers = m_activeView->visibleMembersList();
		membersLeft->setChecked( pos == KDockWidget::DockLeft  );
		membersLeft->setEnabled( visibleMembers );
		membersRight->setChecked( pos == KDockWidget::DockRight );
		membersRight->setEnabled( visibleMembers );
		toggleMembers->setChecked( visibleMembers );
	}
}

void KopeteChatWindow::slotViewMembersLeft()
{
	m_activeView->placeMembersList( KDockWidget::DockLeft );
	updateMembersActions();
}

void KopeteChatWindow::slotViewMembersRight()
{
	m_activeView->placeMembersList( KDockWidget::DockRight );
	updateMembersActions();
}

void KopeteChatWindow::slotToggleViewMembers()
{
	m_activeView->toggleMembersVisibility();
	updateMembersActions();
}

void KopeteChatWindow::slotHistoryUp()
{
	if( m_activeView )
		m_activeView->historyUp();
}

void KopeteChatWindow::slotHistoryDown()
{
	if( m_activeView )
		m_activeView->historyDown();
}

void KopeteChatWindow::slotCut()
{
	m_activeView->cut();
}

void KopeteChatWindow::slotCopy()
{
	m_activeView->copy();
}

void KopeteChatWindow::slotPaste()
{
	m_activeView->paste();
}


void KopeteChatWindow::slotSetFont()
{
	m_activeView->setFont();
}

void KopeteChatWindow::slotSetFgColor()
{
	m_activeView->setFgColor();
}

void KopeteChatWindow::slotSetBgColor()
{
	m_activeView->setBgColor();
}

/*
KopeteView *KopeteChatWindow::addChatView( KopeteMessageManager *manager )
{
	kdDebug(14010) << k_funcinfo << "Adding a new chat view" << endl;

	//Save options so we can duplicate its appearance
	if( m_activeView )
		m_activeView->saveOptions();

	QPixmap pluginIcon = SmallIcon( manager->protocol()->pluginIcon() );

	ChatView *newView = new ChatView ( manager, this, pluginIcon, mainArea );

	return newView;
}
*/

void KopeteChatWindow::setStatus(const QString &text)
{
	statusBar()->changeItem(text, MESSAGE_STATUS_ID);
}

void KopeteChatWindow::createTabBar()
{
	if( !m_tabBar )
	{
		m_tabBar = new KTabWidget( mainArea );
		m_tabBar->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );
		m_tabBar->setHoverCloseButton(true);
		m_tabBar->setTabReorderingEnabled(true);
		connect( m_tabBar, SIGNAL( closeRequest( QWidget* )), this, SLOT( slotCloseChat( QWidget* ) ) );

		#if QT_VERSION >= 0x030200
			QToolButton* m_rightWidget = new QToolButton( m_tabBar );
			connect( m_rightWidget, SIGNAL( clicked() ), this, SLOT( slotChatClosed() ) );
			m_rightWidget->setIconSet( SmallIcon( "tab_remove" ) );
			m_rightWidget->adjustSize();
			QToolTip::add( m_rightWidget, i18n("Close the current tab"));
			m_tabBar->setCornerWidget( m_rightWidget, QWidget::TopRight );
		#endif

		mainLayout->addWidget( m_tabBar );
		m_tabBar->show();
		connect ( m_tabBar, SIGNAL(currentChanged(QWidget *)), this, SLOT(setActiveView(QWidget *)) );
		connect ( m_tabBar, SIGNAL(contextMenu(QWidget *, const QPoint & )), this, SLOT(slotTabContextMenu( QWidget *, const QPoint & )) );

		for( ChatView *view = chatViewList.first(); view; view = chatViewList.next() )
			addTab( view );

		if( m_activeView )
			m_tabBar->showPage( m_activeView );

		KGlobal::config()->setGroup( QString::fromLatin1("ChatWindowSettings") );
		int tabPosition = KGlobal::config()->readNumEntry( QString::fromLatin1("Tab Placement") , 0 );
		slotPlaceTabs( tabPosition );
	}
}

void KopeteChatWindow::slotCloseChat( QWidget *chatView )
{
	static_cast<ChatView*>( chatView )->closeView();
}

void KopeteChatWindow::addTab( ChatView *view )
{
	QPtrList<KopeteContact> chatMembers=view->msgManager()->members();
	KopeteContact *c=0L;
	for ( KopeteContact *contact = chatMembers.first(); contact; contact = chatMembers.next() )
	{
		if(!c || c->onlineStatus() < contact->onlineStatus())
			c=contact;
	}
	QPixmap pluginIcon = c ? view->msgManager()->contactOnlineStatus( c ).iconFor( c) : SmallIcon( view->msgManager()->protocol()->pluginIcon() );

	view->reparent( m_tabBar, 0, QPoint(), true );
	m_tabBar->addTab( view, pluginIcon, QString::null );
	view->setTabBar( m_tabBar );
	if( view == m_activeView )
		view->show();
	else
		view->hide();
	view->setCaption( view->caption(), false );
}

void KopeteChatWindow::setPrimaryChatView( ChatView *view )
{
	view->reparent( mainArea, 0, QPoint(), true );
	view->setTabBar( 0L );
	view->show();
	mainLayout->addWidget( view );
	setActiveView( view );
}

void KopeteChatWindow::deleteTabBar()
{
	if( m_tabBar )
	{
		disconnect ( m_tabBar, SIGNAL(currentChanged(QWidget *)), this, SLOT(setActiveView(QWidget *)) );
		disconnect ( m_tabBar, SIGNAL(contextMenu(QWidget *, const QPoint & )), this, SLOT(slotTabContextMenu( QWidget *, const QPoint & )) );

		if( !chatViewList.isEmpty() )
			setPrimaryChatView( chatViewList.first() );

		m_tabBar->deleteLater();
		m_tabBar = 0L;
	}
}

void KopeteChatWindow::attachChatView( ChatView* newView )
{
	chatViewList.append( newView );

	switch( chatViewList.count() )
	{
	case 1:
		setPrimaryChatView( newView );
		break;
	case 2:
		createTabBar();
		break;
	default:
		addTab( newView );
		break;
	}

	newView->setMainWindow( this );

	newView->editWidget()->installEventFilter( this );
	KCursor::setAutoHideCursor( newView->editWidget(), true, true );
	connect( newView, SIGNAL(captionChanged( bool)), this, SLOT(slotSetCaption(bool)) );
	connect( newView, SIGNAL(messageSuccess( ChatView* )), this, SLOT(slotStopAnimation( ChatView* )) );
	connect( newView, SIGNAL(updateStatusIcon( const ChatView* )), this, SLOT(slotUpdateCaptionIcons( const ChatView* )) );

	checkDetachEnable();
}

void KopeteChatWindow::checkDetachEnable()
{
	bool haveTabs = (chatViewList.count() > 1);
	tabDetach->setEnabled( haveTabs );
	tabLeft->setEnabled( haveTabs );
	tabRight->setEnabled( haveTabs );
	actionTabPlacementMenu->setEnabled( haveTabs );

	bool otherWindows = (windows.count() > 1);
	actionDetachMenu->setEnabled( otherWindows );
}

void KopeteChatWindow::detachChatView( ChatView *view )
{
	if( !chatViewList.removeRef( view ) )
		return;

	disconnect( view, SIGNAL(captionChanged( bool)), this, SLOT(slotSetCaption(bool)) );
	disconnect( view, SIGNAL(updateStatusIcon( const ChatView *)), this, SLOT(slotUpdateCaptionIcons( const ChatView * )) );
	view->editWidget()->removeEventFilter( this );

	if( m_tabBar )
	{
		int curPage = m_tabBar->currentPageIndex();
		QWidget *page = m_tabBar->page( curPage );

		// if the current view is to be detached, switch to a different one
		if( page == view )
		{
			if( curPage > 0 )
				m_tabBar->setCurrentPage( curPage - 1 );
			else
				m_tabBar->setCurrentPage( curPage + 1 );
		}

		view->setTabBar( 0L );
		m_tabBar->removePage( view );

		if( m_tabBar->currentPage() )
			setActiveView( static_cast<ChatView*>(m_tabBar->currentPage()) );
	}

	if( chatViewList.isEmpty() )
		close();
	else if( chatViewList.count() == 1)
		deleteTabBar();

	checkDetachEnable();
}

void KopeteChatWindow::slotDetachChat( int newWindowIndex )
{
	KopeteChatWindow *newWindow = 0L;
	ChatView *detachedView;

	if( m_popupView )
		detachedView = m_popupView;
	else
		detachedView = m_activeView;

	if( !detachedView )
		return;

	//if we don't do this, we might crash
	createGUI(0L);
	guiFactory()->removeClient(detachedView->msgManager());

	if( newWindowIndex == -1 )
		newWindow = new KopeteChatWindow();
	else
		newWindow = windows.at( newWindowIndex );

	newWindow->show();
	newWindow->raise();

	detachChatView( detachedView );
	newWindow->attachChatView( detachedView );
}

void KopeteChatWindow::slotPreviousTab()
{
	int curPage = m_tabBar->currentPageIndex();
	if( curPage > 0 )
		m_tabBar->setCurrentPage( curPage - 1 );
	else
		m_tabBar->setCurrentPage( m_tabBar->count() - 1 );
}

void KopeteChatWindow::slotNextTab()
{
	int curPage = m_tabBar->currentPageIndex();
	if( curPage == ( m_tabBar->count() - 1 ) )
		m_tabBar->setCurrentPage( 0 );
	else
		m_tabBar->setCurrentPage( curPage + 1 );
}

void KopeteChatWindow::slotSetCaption( bool active )
{
	if( active && m_activeView )
	{
		setCaption( m_activeView->caption(), false );
	}
}

void KopeteChatWindow::updateBackground( const QPixmap &pm )
{
	if( updateBg )
	{
		updateBg = false;
		if( backgroundFile != 0L )
		{
			backgroundFile->close();
			backgroundFile->unlink();
		}

		backgroundFile = new KTempFile( QString::null, QString::fromLatin1( ".bmp" ) );
		pm.save( backgroundFile->name(), "BMP" );
		QTimer::singleShot( 100, this, SLOT( slotEnableUpdateBg() ) );
	}
}

void KopeteChatWindow::setActiveView( QWidget *widget )
{
	ChatView *view = static_cast<ChatView*>(widget);

//	kdDebug(14010) << k_funcinfo << m_activeView << "  |||||||||  " << view << endl;

	if( m_activeView == view )
		return;

	if(m_activeView)
		guiFactory()->removeClient(m_activeView->msgManager());
	guiFactory()->addClient(view->msgManager());
	createGUI( view->part() );

	if( m_activeView )
		m_activeView->setActive( false );

	m_activeView = view;

	if( !chatViewList.contains( view ) )
		attachChatView( view );

	//Tell it it is active
	m_activeView->setActive( true );

	//Update icons to match
	slotUpdateCaptionIcons( m_activeView );

	//Update chat members actions
	updateMembersActions();

	if ( m_activeView->sendInProgress() )
	{
		anim->setMovie( animIcon );
		animIcon.unpause();
	}
	else
	{
		anim->setPixmap( normalIcon );
		animIcon.pause();
	}

	if ( chatViewList.count() > 1 )
	{
		if( !m_tabBar )
			createTabBar();

		m_tabBar->showPage( m_activeView );
	}

	setCaption( m_activeView->caption() );
	setStatus( m_activeView->status() );
	m_activeView->setFocus();
}

void KopeteChatWindow::slotUpdateCaptionIcons( const ChatView *view )
{
	if(!view||!m_activeView||view!=m_activeView )
		return; //(pas de charit�)
	QPtrList<KopeteContact> chatMembers=view->msgManager()->members();
	KopeteContact *c=0L;
	for ( KopeteContact *contact = chatMembers.first(); contact; contact = chatMembers.next() )
	{
		if(!c || c->onlineStatus() < contact->onlineStatus())
			c=contact;
	}
	QPixmap icon16 = c ? view->msgManager()->contactOnlineStatus( c ).iconFor( c , 16) : SmallIcon( view->msgManager()->protocol()->pluginIcon() );
	QPixmap icon32 = c ? view->msgManager()->contactOnlineStatus( c ).iconFor( c , 32) : SmallIcon( view->msgManager()->protocol()->pluginIcon() );

	KWin::setIcons( winId(), icon32, icon16 );
}

void KopeteChatWindow::slotChatClosed()
{
	if( m_popupView )
		m_popupView->closeView();
	else
		m_activeView->closeView();
}

void KopeteChatWindow::slotPrepareDetachMenu(void)
{
	QPopupMenu *detachMenu = actionDetachMenu->popupMenu();
	detachMenu->clear();

	for ( unsigned id=0; id < windows.count(); id++ )
	{
		KopeteChatWindow *win = windows.at( id );
		if( win != this )
			detachMenu->insertItem( win->caption(), id );
	}
}

void KopeteChatWindow::slotSendMessage()
{
	if ( m_activeView )
	{
		anim->setMovie( animIcon );
		animIcon.unpause();
		m_activeView->sendMessage();
	}
}

void KopeteChatWindow::slotPrepareContactMenu(void)
{
	QPopupMenu *contactsMenu = actionContactMenu->popupMenu();
	contactsMenu->clear();

	KopeteContact *contact;
	KopeteContactPtrList m_them;

	if( m_popupView )
		m_them = m_popupView->msgManager()->members();
	else
		m_them = m_activeView->msgManager()->members();

	//TODO: don't display a menu with one contact in it, display that
	// contact's menu instead. Will require changing text and icon of
	// 'Contacts' action, or something cleverer.
	uint contactCount = 0;

	for ( contact = m_them.first(); contact; contact = m_them.next() )
	{
		KPopupMenu *p = contact->popupMenu();
		connect ( actionContactMenu->popupMenu(), SIGNAL(aboutToHide()),
			p, SLOT(deleteLater() ) );

		if( contact->metaContact() )
			contactsMenu->insertItem( contact->onlineStatus().iconFor( contact ) , contact->metaContact()->displayName(), p );
		else
			contactsMenu->insertItem( contact->onlineStatus().iconFor( contact ) , contact->displayName(), p );

		//FIXME: This number should be a config option
		if( ++contactCount == 15 && contact != m_them.getLast() )
		{
			KActionMenu *moreMenu = new KActionMenu( i18n("More..."),
				 QString::fromLatin1("folder_open"), contactsMenu );
			connect ( actionContactMenu->popupMenu(), SIGNAL(aboutToHide()),
				moreMenu, SLOT(deleteLater() ) );
			moreMenu->plug( contactsMenu );
			contactsMenu = moreMenu->popupMenu();
			contactCount = 0;
		}
	}
}

void KopeteChatWindow::slotPreparePlacementMenu()
{
	QPopupMenu *placementMenu = actionTabPlacementMenu->popupMenu();
	placementMenu->clear();

	placementMenu->insertItem( i18n("Top"), 0 );
	placementMenu->insertItem( i18n("Bottom"), 1 );
}

void KopeteChatWindow::slotPlaceTabs( int placement )
{
	if( m_tabBar )
	{
//		kdDebug(14010) << k_funcinfo << "Placement:" << placement << endl;
		if( placement == 0 )
			m_tabBar->setTabPosition( QTabWidget::Top );
		else
			m_tabBar->setTabPosition( QTabWidget::Bottom );

		saveOptions();
	}
}

void KopeteChatWindow::readOptions()
{
	// load and apply config file settings affecting the appearance of the UI
//	kdDebug(14010) << k_funcinfo << endl;
	KConfig *config = KGlobal::config();
	applyMainWindowSettings( config, QString::fromLatin1( "KopeteChatWindow" ) );
	config->setGroup( QString::fromLatin1("ChatWindowSettings") );
}

void KopeteChatWindow::saveOptions()
{
//	kdDebug(14010) << k_funcinfo << endl;

	KConfig *config = KGlobal::config();

	// saves menubar,toolbar and statusbar setting
	saveMainWindowSettings( config, QString::fromLatin1( "KopeteChatWindow" ) );
	config->setGroup( QString::fromLatin1("ChatWindowSettings") );
	if( m_tabBar )
		config->writeEntry ( QString::fromLatin1("Tab Placement"), m_tabBar->tabPosition() );

	config->sync();
}

void KopeteChatWindow::slotChatSave()
{
//	kdDebug(14010) << "KopeteChatWindow::slotChatSave()" << endl;
	if( isActiveWindow() && m_activeView )
		m_activeView->save();
}

void KopeteChatWindow::windowActivationChange( bool )
{
	if( isActiveWindow() && m_activeView )
		m_activeView->setActive( true );
}

void KopeteChatWindow::slotChatPrint()
{
	m_activeView->print();
}

void KopeteChatWindow::slotToggleStatusBar()
{
#if !KDE_IS_VERSION( 3, 1, 90 )
	if (statusBar()->isVisible())
		statusBar()->hide();
	else
		statusBar()->show();
#endif
}

void KopeteChatWindow::slotViewMenuBar()
{
	if( !menuBar()->isHidden() )
		menuBar()->hide();
	else
		menuBar()->show();
}

void KopeteChatWindow::slotSmileyActivated(const QString &sm)
{
	if ( !sm.isNull() )
		m_activeView->addText( sm );
}

void KopeteChatWindow::closeEvent( QCloseEvent *e )
{
//	kdDebug( 14010 ) << k_funcinfo << endl;

	// FIXME: This should only check if it *can* close
	// and not start closing if the close can be aborted halfway, it would
	// leave us with half the chats open and half of them closed. - Martijn
	bool canClose = true;

//	kdDebug( 14010 ) << " Windows left open:" << endl;
//	for( QPtrListIterator<ChatView> it( chatViewList ); it; ++it)
//		kdDebug( 14010 ) << "  " << *it << " (" << (*it)->caption() << ")" << endl;

	for( QPtrListIterator<ChatView> it( chatViewList ); it; )
	{
		ChatView *view = *it;
		// move out of the way before view is removed
		++it;

		// if the view is closed, it is removed from chatViewList for us
		if( !view->closeView() )
			canClose = false;
	}

	if( canClose )
		e->accept();
	else
		e->ignore();
}

void KopeteChatWindow::slotConfKeys()
{
	KKeyDialog dlg( false, this );
	dlg.insert( actionCollection() );
	if( m_activeView )
	{
		dlg.insert(m_activeView->msgManager()->actionCollection() , i18n("Plugin Actions") );
		QPtrListIterator<KXMLGUIClient> it( *m_activeView->msgManager()->childClients() );
		KXMLGUIClient *c = 0;
		while( (c = it.current()) != 0 )
		{
			dlg.insert( c->actionCollection() , i18n("Plugin Actions") );
		}

		if( m_activeView->part() )
			dlg.insert( m_activeView->part()->actionCollection(), m_activeView->part()->name() );
	}

	dlg.configure();
}

void KopeteChatWindow::slotConfToolbar()
{
	saveMainWindowSettings(KGlobal::config(), QString::fromLatin1( "KopeteChatWindow" ));
	KEditToolbar *dlg = new KEditToolbar(factory(), this );
	if (dlg->exec())
	{
		if( m_activeView )
		{
			createGUI( m_activeView->part() );
			//guiFactory()->addClient(m_activeView->msgManager());
		}
		else
			createGUI( 0L );
		applyMainWindowSettings(KGlobal::config(), QString::fromLatin1( "KopeteChatWindow" ));
	}
	delete dlg;
}

bool KopeteChatWindow::queryExit()
{
//	 never quit kopete
	return false;
}

#include "kopetechatwindow.moc"
// vim: set noet ts=4 sts=4 sw=4:
