/*
    kopetewindow.cpp  -  Kopete Main Window

    Copyright (c) 2001-2002 by Duncan Mac-Vicar Prett <duncan@kde.org>
    Copyright (c) 2001-2002 by Stefan Gehn            <metz AT gehn.net>
    Copyright (c) 2002-2003 by Martijn Klingens       <klingens@kde.org>

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

#include "kopetewindow.h"

#include <qhbox.h>
#include <qtooltip.h>

#include <kaction.h>
#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kaccel.h>
#include <kkeydialog.h>
#include <kedittoolbar.h>
#include <kmenubar.h>
#include <kstatusbar.h>
#include <kglobalaccel.h>
#include <kwin.h>
#include <kdeversion.h>
#include <kinputdialog.h>

#include "addcontactwizard.h"
#include "kopete.h"
#include "kopeteaccount.h"
#include "kopeteaway.h"
#include "kopeteaccountmanager.h"
#include "kopeteaccountstatusbaricon.h"
#include "kopetecontact.h"
#include "kopetecontactlist.h"
#include "kopetecontactlistview.h"
#include "kopetemessagemanagerfactory.h"
#include "kopetepluginconfig.h"
#include "kopetepluginmanager.h"
#include "kopeteprefs.h"
#include "kopeteprotocol.h"
#include "kopetestdaction.h"
#include "kopeteawayaction.h"
#include "systemtray.h"

KopeteWindow::KopeteWindow( QWidget *parent, const char *name )
: KMainWindow( parent, name )
{
	// Applications should ensure that their StatusBar exists before calling createGUI()
	// so that the StatusBar is always correctly positioned when KDE is configured to use
	// a MacOS-style MenuBar.
	// This fixes a "statusbar drawn over the top of the toolbar" bug
	// e.g. it can happen when you switch desktops on Kopete startup
	m_statusBarWidget = new QHBox(statusBar(), "m_statusBarWidget");
	m_statusBarWidget->setSpacing( 2 );
	m_statusBarWidget->setMargin( 2 );
	statusBar()->addWidget(m_statusBarWidget, 0, true);

	connect( KopetePrefs::prefs(), SIGNAL( saved() ), this, SLOT( slotSettingsChanged() ) );

	m_pluginConfig = 0L;

	// --------------------------------------------------------------------------------
	initView();
	initActions();
	contactlist->initActions(actionCollection());
	initSystray();
	// --------------------------------------------------------------------------------

	// Trap all loaded plugins, so we can add their status bar icons accordingly , also used to add XMLGUIClient
	connect( KopetePluginManager::self(), SIGNAL( pluginLoaded( KopetePlugin * ) ),
		this, SLOT( slotPluginLoaded( KopetePlugin * ) ) );
	// And accounts too
	connect( KopeteAccountManager::manager(), SIGNAL(accountReady(KopeteAccount*)),
		this, SLOT(slotAccountRegistered(KopeteAccount*)));
	connect( KopeteAccountManager::manager(), SIGNAL(accountUnregistered(KopeteAccount*)),
		this, SLOT(slotAccountUnregistered(KopeteAccount*)));
		
	createGUI ( "kopeteui.rc", false );

	// call this _after_ createGUI(), otherwise menubar is not set up correctly
	loadOptions();

	// If some plugins are already loaded, merge the GUI
	QMap<KPluginInfo *, KopetePlugin *> plugins = KopetePluginManager::self()->loadedPlugins();
	QMap<KPluginInfo *, KopetePlugin *>::ConstIterator it;
	for ( it = plugins.begin(); it != plugins.end(); ++it )
		slotPluginLoaded( it.data() );

	// If some account alrady loaded, build the status icon
	QPtrList<KopeteAccount>  accounts = KopeteAccountManager::manager()->accounts();
	for(KopeteAccount *a=accounts.first() ; a; a=accounts.next() )
		slotAccountRegistered(a);
}

void KopeteWindow::initView()
{
	contactlist = new KopeteContactListView(this);
	setCentralWidget(contactlist);
}

void KopeteWindow::initActions()
{
	actionAddContact = new KAction( i18n( "&Add Contact..." ), "bookmark_add",
		0, this, SLOT( showAddContactDialog() ),
		actionCollection(), "AddContact" );

	actionConnect = new KAction( i18n( "&Connect All" ), "connect_creating",
		0, KopeteAccountManager::manager(), SLOT( connectAll() ),
		actionCollection(), "ConnectAll" );

	actionDisconnect = new KAction( i18n( "&Disconnect All" ), "connect_no",
		0, KopeteAccountManager::manager(), SLOT( disconnectAll() ),
		actionCollection(), "DisconnectAll" );

	actionConnectionMenu = new KActionMenu( i18n("Connection"),"connect_established",
							actionCollection(), "Connection" );

	actionConnectionMenu->insert(actionConnect);
	actionConnectionMenu->insert(actionDisconnect);

	selectAway = new KopeteAwayAction( i18n("&Set Away Globally"), SmallIcon("kopeteaway"), 0, 
		this, SLOT( slotGlobalAwayMessageSelect( const QString & ) ), actionCollection(),
		"SetAwayAll" );

	actionSetAvailable = new KAction( i18n( "Set Availa&ble Globally" ),
		"kopeteavailable", 0 , KopeteAccountManager::manager(),
		SLOT( setAvailableAll() ), actionCollection(),
		"SetAvailableAll" );

	actionAwayMenu = new KActionMenu( i18n("Status"),"kopeteaway",
							actionCollection(), "Status" );
	actionAwayMenu->setDelayed( false );
	actionAwayMenu->insert(actionSetAvailable);
	actionAwayMenu->insert(selectAway);
	actionPrefs = KopeteStdAction::preferences( actionCollection(), "settings_prefs" );

	actionSave = new KAction( i18n("Save &Contact List"), "filesave", KStdAccel::shortcut(KStdAccel::Save),
							this, SLOT(slotSaveContactList()),
							actionCollection(), "save_contactlist" );

	KStdAction::quit(this, SLOT(slotQuit()), actionCollection());

//	toolbarAction = KStdAction::showToolbar(this, SLOT(showToolbar()), actionCollection(), "settings_showtoolbar" );
	menubarAction = KStdAction::showMenubar(this, SLOT(showMenubar()), actionCollection(), "settings_showmenubar" );
	setStandardToolBarMenuEnabled(true);
	statusbarAction = KStdAction::showStatusbar(this, SLOT(showStatusbar()), actionCollection(), "settings_showstatusbar");

	KStdAction::keyBindings( this, SLOT( slotConfKeys() ), actionCollection(), "settings_keys" );
	new KAction( i18n( "Configure Plugins..." ), "input_devices_settings", 0, this,
		SLOT( slotConfigurePlugins() ), actionCollection(), "settings_plugins" );
	new KAction( i18n( "Configure &Global Shortcuts..." ), "configure_shortcuts", 0, this,
		SLOT( slotConfGlobalKeys() ), actionCollection(), "settings_global" );

	KStdAction::configureToolbars(this, SLOT(slotConfToolbar()), actionCollection(), "settings_toolbars" );

	actionShowOffliners = new KToggleAction( i18n( "Show Offline &Users" ), "viewmag", CTRL + Key_F,
			this, SLOT( slotToggleShowOffliners() ), actionCollection(), "settings_show_offliners" );
	actionShowEmptyGroups = new KToggleAction( i18n( "Show &Empty Groups" ), "folder_green", CTRL + Key_E,
			this, SLOT( slotToggleShowEmptyGroups() ), actionCollection(), "settings_show_empty_groups" );

	// sync actions, config and prefs-dialog
	connect ( KopetePrefs::prefs(), SIGNAL(saved()), this, SLOT(slotConfigChanged()) );
	slotConfigChanged();

	globalAccel = new KGlobalAccel( this );
	globalAccel->insert( QString::fromLatin1("Read Message"), i18n("Read Message"), i18n("Read the next pending message"),
		CTRL+SHIFT+Key_I, KKey::QtWIN+CTRL+Key_I, KopeteMessageManagerFactory::factory() , SIGNAL(readMessage()) );

	globalAccel->insert( QString::fromLatin1("Show/Hide Contact List"), i18n("Show/Hide Contact List"), i18n("Show or hide the contact list"),
		CTRL+SHIFT+Key_C, KKey::QtWIN+CTRL+Key_C, this, SLOT(slotShowHide()) );

	globalAccel->readSettings();
	globalAccel->updateConnections();
}

void KopeteWindow::slotShowHide()
{
	if(isActiveWindow())
		hide();
	else
	{
		show();
		//raise() and show() should normaly deIconify the window. but it doesn't do here due
		// to a bug in QT or in KDE  (qt3.1.x or KDE 3.1.x) then, i have to call KWin's method
		if(isMinimized())
			KWin::deIconifyWindow(winId());
		if(!KWin::info(winId()).onAllDesktops)
			KWin::setOnDesktop(winId(), KWin::currentDesktop());
		raise();
		setActiveWindow();
	}
}

void KopeteWindow::initSystray()
{
	tray = KopeteSystemTray::systemTray( this, "KopeteSystemTray" );
	KPopupMenu *tm = tray->contextMenu();

	// NOTE: This is in reverse order because we insert
	// at the top of the menu, not at bottom!
	actionAddContact->plug( tm,1 );
	actionPrefs->plug(tm,1);
	tm->insertSeparator(1);
	actionAwayMenu->plug( tm,1 );
	actionConnectionMenu->plug ( tm,1 );
	tm->insertSeparator(1);

	QObject::connect( tray, SIGNAL( aboutToShowMenu( KPopupMenu * ) ),
		this, SLOT( slotTrayAboutToShowMenu( KPopupMenu * ) ) );
	QObject::connect( tray, SIGNAL( quitSelected() ), this, SLOT( slotQuit() ) );
}

KopeteWindow::~KopeteWindow()
{
	delete m_pluginConfig;
}

bool KopeteWindow::queryExit()
{
	saveOptions();
	return true;
}

void KopeteWindow::loadOptions()
{
	KConfig *config = KGlobal::config();

	toolBar("mainToolBar")->applySettings( config, "ToolBar Settings" );

	applyMainWindowSettings( config, "General Options" );

	QPoint pos = config->readPointEntry("Position");
	move(pos);

	QSize size = config->readSizeEntry("Geometry");
	if(size.isEmpty())
	{
		// Default size
		resize( QSize(220, 350) );
	}
	else
	{
		resize(size);
	}

	KopetePrefs *p = KopetePrefs::prefs();
	QString tmp = config->readEntry("State", "Shown");
	if ( tmp == "Minimized" && p->showTray())
	{
		showMinimized();
	}
	else if ( tmp == "Hidden" && p->showTray())
	{
		hide();
	}
	else if ( !p->startDocked() || !p->showTray() )
		show();

//	toolbarAction->setChecked( !toolBar("mainToolBar")->isHidden() );
	menubarAction->setChecked( !menuBar()->isHidden() );
	statusbarAction->setChecked( !statusBar()->isHidden() );
}

void KopeteWindow::saveOptions()
{
	KConfig *config = KGlobal::config();

	toolBar("mainToolBar")->saveSettings ( config, "ToolBar Settings" );

	saveMainWindowSettings( config, "General Options" );

	globalAccel->writeSettings();
	config->setGroup("General Options");
	config->writeEntry("Position", pos());
	config->writeEntry("Geometry", size());

	if(isMinimized())
	{
		config->writeEntry("State", "Minimized");
	}
	else if(isHidden())
	{
		config->writeEntry("State", "Hidden");
	}
	else
	{
		config->writeEntry("State", "Shown");
	}

	config->sync();
}

/*void KopeteWindow::showToolbar()
{
	if( toolbarAction->isChecked() )
		toolBar("mainToolBar")->show();
	else
		toolBar("mainToolBar")->hide();
}*/

void KopeteWindow::showMenubar()
{
	if(menubarAction->isChecked())
		menuBar()->show();
	else
		menuBar()->hide();
}

void KopeteWindow::showStatusbar()
{
	if( statusbarAction->isChecked() )
		statusBar()->show();
	else
		statusBar()->hide();
}

void KopeteWindow::slotToggleShowOffliners()
{
	KopetePrefs *p = KopetePrefs::prefs();
	p->setShowOffline ( actionShowOffliners->isChecked() );

	disconnect ( KopetePrefs::prefs(), SIGNAL(saved()), this, SLOT(slotConfigChanged()) );
	p->save();
	connect ( KopetePrefs::prefs(), SIGNAL(saved()), this, SLOT(slotConfigChanged()) );
}

void KopeteWindow::slotToggleShowEmptyGroups()
{
	KopetePrefs *p = KopetePrefs::prefs();
	p->setShowEmptyGroups ( actionShowEmptyGroups->isChecked() );

	disconnect ( KopetePrefs::prefs(), SIGNAL(saved()), this, SLOT(slotConfigChanged()) );
	p->save();
	connect ( KopetePrefs::prefs(), SIGNAL(saved()), this, SLOT(slotConfigChanged()) );
}

void KopeteWindow::slotConfigChanged()
{
	KopetePrefs *pref = KopetePrefs::prefs();

	if( isHidden() && !pref->showTray()) // user disabled systray while kopete is hidden, show it!
		show();

	actionShowOffliners->setChecked( pref->showOffline() );
	actionShowEmptyGroups->setChecked( pref->showEmptyGroups() );
}

void KopeteWindow::slotConfKeys()
{
	KKeyDialog::configure(actionCollection(), this, true);
}

void KopeteWindow::slotConfigurePlugins()
{
	if ( !m_pluginConfig )
		m_pluginConfig = new KopetePluginConfig( this );
	m_pluginConfig->show();
}

void KopeteWindow::slotConfGlobalKeys()
{
	KKeyDialog::configure( globalAccel );
}

void KopeteWindow::slotConfToolbar()
{
	saveMainWindowSettings(KGlobal::config(), "General Options");
	KEditToolbar *dlg = new KEditToolbar(actionCollection(), "kopeteui.rc");
	connect( dlg, SIGNAL(newToolbarConfig()), this, SLOT(slotUpdateToolbar()) );
	connect( dlg, SIGNAL(finished()) , dlg, SLOT(deleteLater()));
	dlg->show();
}

void KopeteWindow::slotUpdateToolbar()
{
	createGUI("kopeteui.rc", false);
	applyMainWindowSettings(KGlobal::config(), "General Options");
}

void KopeteWindow::slotGlobalAwayMessageSelect( const QString &awayReason )
{
	KopeteAway::getInstance()->setGlobalAwayMessage( awayReason );
	KopeteAccountManager::manager()->setAwayAll( awayReason );
}

void KopeteWindow::closeEvent( QCloseEvent *e )
{
	Kopete *app = static_cast<Kopete *>( kapp );

	// also close if our tray icon is hidden!
	if( app->isShuttingDown() || !KopetePrefs::prefs()->showTray() || !isShown() )
	{
		KMainWindow::closeEvent( e );
		return;
	}

	// FIXME: KDE 3.3:  use gueuedMessageBox
	KMessageBox::information( this,
		i18n( "<qt>Closing the main window will keep Kopete running in the "
		"system tray. Use 'Quit' from the 'File' menu to quit the application.</qt>" ),
		i18n( "Docking in System Tray" ), "hideOnCloseInfo" );

	// FIXME: Instead of ignoring the close event it's much better to have
	//        KopeteSystemTray ref() the KApplication object and deref it on
	//        quit. This way we can pass on the event to Qt unmodified instead.
	//        We may or may not need to remove the destructive close from the
	//        WFlags of the window though when doing this. Ideally the code
	//        should be able to handle the case where no main window exists,
	//        but I'm not 100% sure this is actually the case. - Martijn
	hide();
	e->ignore();
}

void KopeteWindow::slotQuit()
{
	Kopete *app = static_cast<Kopete *>( kapp );
	app->quitKopete();
}

void KopeteWindow::slotPluginLoaded( KopetePlugin *  p  )
{
	guiFactory()->addClient(p);
}

void KopeteWindow::slotAccountRegistered( KopeteAccount *a )
{
//	kdDebug(14000) << k_funcinfo << "Called." << endl;
	if ( !a )
		return;

	connect( a->myself(),
		SIGNAL(onlineStatusChanged( KopeteContact *, const KopeteOnlineStatus &, const KopeteOnlineStatus &) ),
		this, SLOT( slotAccountStatusIconChanged( KopeteContact * ) ) );

	KopeteAccountStatusBarIcon *i = new KopeteAccountStatusBarIcon( a, m_statusBarWidget );
	connect( i, SIGNAL( rightClicked( KopeteAccount *, const QPoint & ) ),
		SLOT( slotAccountStatusIconRightClicked( KopeteAccount *,
		const QPoint & ) ) );
	// Wanted by pmax, not sure if we should leave both in
	// it'll confuse users if we take out the RMB function
	connect( i, SIGNAL( leftClicked( KopeteAccount *, const QPoint & ) ),
		SLOT( slotAccountStatusIconRightClicked( KopeteAccount *,
		const QPoint & ) ) );

	m_accountStatusBarIcons.insert( a, i );

	// this should be placed in the contactlistview insteads of, but i am lazy to redo a new slot
	contactlist->actionAddContact->insert(new KAction( a->accountId() , a->protocol()->pluginIcon() , 0 , contactlist , SLOT( slotAddContact() ) , a));
	slotAccountStatusIconChanged( a->myself() );
}

void KopeteWindow::slotAccountUnregistered( KopeteAccount *a)
{
//	kdDebug(14000) << k_funcinfo << "Called." << endl;

	KopeteAccountStatusBarIcon *i = static_cast<KopeteAccountStatusBarIcon *>( m_accountStatusBarIcons[ a ] );
	if( !i )
		return;

	delete i;
	m_accountStatusBarIcons.remove( a );
}

void KopeteWindow::slotAccountStatusIconChanged( KopeteContact *contact )
{
	KopeteOnlineStatus status = contact->onlineStatus();
//	kdDebug(14000) << k_funcinfo << "Icon: '" <<
//		status.overlayIcon() << "'" << endl;

	KopeteAccountStatusBarIcon *i = static_cast<KopeteAccountStatusBarIcon *>( m_accountStatusBarIcons[ contact->account() ] );
	if( !i )
		return;


	// Adds tooltip for each status icon,
	// useful in case you have many accounts
	// over one protocol

	QToolTip::remove( i );

	QString contactLabel;
	if ( contact->displayName() == contact->account()->accountId() )
	{
		contactLabel = contact->displayName();
	}
	else
	{
		// FIXME: Make this string i18n() after 0.7 - Martijn
		contactLabel = QString::fromLatin1( "%1 <%2>" ).
#if QT_VERSION < 0x030200
			arg( contact->displayName() ).arg( contact->account()->accountId() );
#else
			arg( contact->displayName(), contact->account()->accountId() );
#endif
	}

	// FIXME: Make this string i18n() after 0.7 - Martijn
	QString tooltip = QString::fromLatin1( "%1 (%2)" ).
#if QT_VERSION < 0x030200
		arg( contactLabel ).arg( status.description() );
#else
		arg( contactLabel, status.description() );
#endif

	if ( !contact->statusDescription().isNull() )
		tooltip += QString::fromLatin1( "\n" ) + contact->statusDescription();

	QToolTip::add( i, tooltip );

	// Because we want null pixmaps to detect the need for a loadMovie
	// we can't use the SmallIcon() method directly
	KIconLoader *loader = KGlobal::instance()->iconLoader();

	QMovie mv = loader->loadMovie( status.overlayIcon(),
		 KIcon::User);

	if ( mv.isNull() )
	{
		// No movie found, fallback to pixmap
		// Get the icon for our status

		//QPixmap pm = SmallIcon( icon );
		QPixmap pm = status.iconFor( contact->account() );

		// BEGIN REMOVE
		// Compat for the non-themed icons
		// FIXME: When all icons are converted, remove this - Martijn
		if( pm.isNull() )
		{
			kdDebug(14000) << k_funcinfo << " FIXME: loading non-themed icon for "
					<< contact->protocol()->pluginId() << ", needs converting!" << endl;
			pm = loader->loadIcon( status.overlayIcon(),
				 KIcon::User, 0, KIcon::DefaultState, 0L, true );
		}
		// END REMOVE

		if( pm.isNull() )
		{
			// No Pixmap found, fallback to Unknown
			kdDebug(14000) << k_funcinfo <<
				"Using unknown pixmap for status icon '" << status.overlayIcon() << "'."
				<< endl;
			i->setPixmap( KIconLoader::unknown() );
		}
		else
		{
			i->setPixmap( pm );
		}
	}
	else
	{
		//kdDebug( 14000 ) << k_funcinfo << "Using movie."  << endl;
		i->setMovie( mv );
	}
}

void KopeteWindow::slotAccountStatusIconRightClicked( KopeteAccount *account,
	const QPoint &p )
{
	KActionMenu *actionMenu=account->actionMenu();
	if(!actionMenu)
		return;
	connect(actionMenu->popupMenu(), SIGNAL(aboutToHide) , actionMenu , SLOT(deleteLater()) );
	actionMenu->popupMenu()->popup( p );

}

void KopeteWindow::slotTrayAboutToShowMenu( KPopupMenu * popup )
{
	//KPopupMenu *popup=tray->contextMenu();

	QPtrList<KopeteAccount>  accounts = KopeteAccountManager::manager()->accounts();
	for(KopeteAccount *a=accounts.first() ; a; a=accounts.next() )
	{
		KActionMenu *menu = a->actionMenu();
		if( menu )
			menu->plug(popup , 1 );

		connect(popup , SIGNAL(aboutToHide()) , menu , SLOT(deleteLater()));
	}

}

void KopeteWindow::slotProtocolStatusIconRightClicked( KopeteProtocol *proto,
	const QPoint &p )
{
//	kdDebug(14000) << "KopeteWindow::slotProtocolStatusIconRightClicked()" << endl;
	// if the protocol has accounts, show its menu
	// otherwise just show a menu containing "Add New Account"
	KActionMenu *menu = 0L;

	QDict<KopeteAccount> dict = KopeteAccountManager::manager()->accounts( proto );
	if ( dict.count() > 0 )
		menu = proto->protocolActions();

	if( menu )
	{
		connect( menu->popupMenu() , SIGNAL(aboutToHide) , menu , SLOT(deleteLater()) );
		menu->popupMenu()->popup( p );
	}
}

void KopeteWindow::slotSaveContactList()
{
	KopeteContactList::contactList()->save();
	KMessageBox::information(this, i18n("Contact list saved."), i18n("Contact List Saved"));
}

void KopeteWindow::showAddContactDialog()
{
	(new AddContactWizard(qApp->mainWidget()))->show();
}

void KopeteWindow::slotSettingsChanged()
{
	// Account colouring may have changed, so tell our status bar to redraw
//	kdDebug(14000) << k_funcinfo << endl;

	QMap<KPluginInfo *, KopetePlugin *> plugins = KopetePluginManager::self()->loadedPlugins( "Protocols" );
	QMap<KPluginInfo *, KopetePlugin *>::ConstIterator it;
	for ( it = plugins.begin(); it != plugins.end(); ++it )
	{
		KopeteProtocol *proto = static_cast<KopeteProtocol *>( it.data() );
		QDict<KopeteAccount> dict = KopeteAccountManager::manager()->accounts( proto );
		QDictIterator<KopeteAccount> accountIt( dict );
		KopeteAccount *a;
		while( ( a = accountIt.current() ) != 0 )
		{
			slotAccountStatusIconChanged( a->myself() );
			++accountIt;
		}
	}
}

#include "kopetewindow.moc"

// vim: set noet ts=4 sts=4 sw=4:

