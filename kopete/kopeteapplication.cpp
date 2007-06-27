/*
    kopete.cpp

    Kopete Instant Messenger Main Class

    Copyright (c) 2001-2002  by Duncan Mac-Vicar Prett   <duncan@kde.org>
    Copyright (c) 2002-2003  by Martijn Klingens         <klingens@kde.org>

    Kopete    (c) 2001-2003  by the Kopete developers    <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "kopeteapplication.h"

#include <qtimer.h>
#include <qregexp.h>

#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <kmessagebox.h>

#include "addaccountwizard.h"
#include "kabcpersistence.h"
#include "kopeteaccount.h"
#include "kopeteaccountmanager.h"
#include "kopetebehaviorsettings.h"
#include "kopetecommandhandler.h"
#include "kopetecontactlist.h"
#include "kopeteglobal.h"
#include "kopeteidentitymanager.h"
#include "kopetemimesourcefactory.h"
#include "kopetemimetypehandler.h"
#include "kopetepluginmanager.h"
#include "kopeteprotocol.h"
#include "kopetestdaction.h"
#include "kopeteuiglobal.h"
#include "kopetewindow.h"
#include "kopeteviewmanager.h"

KopeteApplication::KopeteApplication()
: KUniqueApplication( true, true )
{
	m_isShuttingDown = false;
	m_mainWindow = new KopeteWindow( 0, "mainWindow" );

	Kopete::PluginManager::self();

	Kopete::UI::Global::setMainWidget( m_mainWindow );

	//Create the identity manager
	Kopete::IdentityManager::self()->load();

	/*
	 * FIXME: This is a workaround for a quite odd problem:
	 * When starting up kopete and the msn plugin gets loaded it can bring up
	 * a messagebox, in case the msg configuration is missing. This messagebox
	 * will result in a QApplication::enter_loop() call, an event loop is
	 * created. At this point however the loop_level is 0, because this is all
	 * still inside the KopeteApplication constructor, before the exec() call from main.
	 * When the messagebox is finished the loop_level will drop down to zero and
	 * QApplication thinks the application shuts down (this is usually the case
	 * when the loop_level goes down to zero) . So it emits aboutToQuit(), to
	 * which KApplication is connected and re-emits shutdown() , to which again
	 * KXmlGuiWindow (a KopeteWindow instance exists already) is connected. KXmlGuiWindow's
	 * shuttingDown() slot calls queryExit() which results in KopeteWindow::queryExit()
	 * calling unloadPlugins() . This of course is wrong and just shouldn't happen.
	 * The workaround is to simply delay the initialization of all this to a point
	 * where the loop_level is already > 0 . That is why I moved all the code from
	 * the constructor to the initialize() method and added this single-shot-timer
	 * setup. (Simon)
	 *
	 * Additionally, it makes the GUI appear less 'blocking' during startup, so
	 * there is a secondary benefit as well here. (Martijn)
	 */
	QTimer::singleShot( 0, this, SLOT( slotLoadPlugins() ) );

	m_mimeFactory = new Kopete::MimeSourceFactory;
	Q3MimeSourceFactory::addFactory( m_mimeFactory );

	//Create the emoticon installer
	m_emoticonHandler = new Kopete::EmoticonMimeTypeHandler;
}

KopeteApplication::~KopeteApplication()
{
	kDebug( 14000 ) << k_funcinfo << endl;

	delete m_mainWindow;
	delete m_emoticonHandler;
	delete m_mimeFactory;
	//kDebug( 14000 ) << k_funcinfo << "Done" << endl;
}

void KopeteApplication::slotLoadPlugins()
{
	// we have to load the address book early, because calling this enters the Qt event loop when there are remote resources.
	// The plugin manager is written with the assumption that Kopete will not reenter the event loop during plugin load,
	// otherwise lots of things break as plugins are loaded, then contacts are added to incompletely initialised MCLVIs
	Kopete::KABCPersistence::self()->addressBook();

	//Create the command handler (looks silly)
	Kopete::CommandHandler::commandHandler();

	//Create the view manager
	KopeteViewManager::viewManager();

	// the account manager should be created after the identity manager is created
	Kopete::AccountManager::self()->load();
	Kopete::ContactList::self()->load();

	KSharedConfig::Ptr config = KGlobal::config();

	// Parse command-line arguments
	KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

	bool showConfigDialog = false;

	KConfigGroup pluginsGroup = config->group( "Plugins" );

	/* FIXME: This is crap, if something purged that groups but your accounts
	 * are still working kopete will load the necessary plugins but still show the
	 * stupid accounts dialog (of course empty at that time because account data
	 * gets loaded later on). [mETz - 29.05.2004]
	 */
	if ( !pluginsGroup.exists() )
		showConfigDialog = true;

	// Listen to arguments
	/*
	// TODO: conflicts with emoticon installer and the general meaning
	// of %U in kopete.desktop
	if ( args->count() > 0 )
	{
		showConfigDialog = false;
		for ( int i = 0; i < args->count(); i++ )
			Kopete::PluginManager::self()->setPluginEnabled( args->arg( i ), true );
	}
	*/

	// Prevent plugins from loading? (--disable=foo,bar)
	foreach ( const QString &disableArg, args->getOption( "disable" ).split( ',' ))
	{
		showConfigDialog = false;
		Kopete::PluginManager::self()->setPluginEnabled( disableArg, false );
	}

	// Load some plugins exclusively? (--load-plugins=foo,bar)
	if ( args->isSet( "load-plugins" ) )
	{
		pluginsGroup.deleteGroup( KConfigBase::Global );
		showConfigDialog = false;
		foreach ( const QString &plugin, args->getOption( "load-plugins" ).split( ',' ))
			Kopete::PluginManager::self()->setPluginEnabled( plugin, true );
	}

	config->sync();

	// Disable plugins altogether? (--noplugins)
	if ( !args->isSet( "plugins" ) )
	{
		// If anybody reenables this I'll get a sword and make a nice chop-suy out
		// of your body :P [mETz - 29.05.2004]
		// This screws up kopeterc because there is no way to get the Plugins group back!
		//config->deleteGroup( "Plugins", true );

		showConfigDialog = false;
		// pretend all plugins were loaded :)
		QTimer::singleShot(0, this, SLOT( slotAllPluginsLoaded() ));
	}
	else
	{
		Kopete::PluginManager::self()->loadAllPlugins();
	}

	connect( Kopete::PluginManager::self(), SIGNAL( allPluginsLoaded() ),
		this, SLOT( slotAllPluginsLoaded() ));

	if( showConfigDialog )
	{
		// No plugins specified. Show the config dialog.
		// FIXME: Although it's a bit stupid it is theoretically possible that a user
		//        explicitly configured Kopete to not load plugins on startup. In this
		//        case we don't want this dialog. We need some other config setting
		//        like a bool hasRunKopeteBefore or so to trigger the loading of the
		//        wizard. Maybe using the last run version number is more useful even
		//        as it also allows for other features. - Martijn
		// FIXME: Possibly we need to influence the showConfigDialog bool based on the
		//        command line arguments processed below. But how exactly? - Martijn
		// NB: the command line args are completely broken atm.
		// I don't want to fix them for 3.5 as plugin loading will change for KDE4.	- Will
		AddAccountWizard *m_addwizard = new AddAccountWizard( Kopete::UI::Global::mainWidget(), true );
		m_addwizard->exec();
		Kopete::AccountManager::self()->save();
	}
}

void KopeteApplication::slotAllPluginsLoaded()
{
	KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

	//FIXME: this should probably ask for the identities to connect instead of all accounts
	// --noconnect not specified?
	if ( args->isSet( "connect" )  && Kopete::BehaviorSettings::self()->autoConnect() )
		Kopete::AccountManager::self()->connectAll();

	QByteArrayList connectArgs = args->getOptionList( "autoconnect" );
	for ( QByteArrayList::ConstIterator i = connectArgs.begin(); i != connectArgs.end(); ++i )
	{
		foreach ( const QString connectArg, QString::fromLocal8Bit(*i).split(','))
			connectArgs.append( connectArg.toLocal8Bit() );
	}

	for ( QByteArrayList::ConstIterator i = connectArgs.begin(); i != connectArgs.end(); ++i )
	{
		QRegExp rx( QLatin1String( "([^\\|]*)\\|\\|(.*)" ) );
		rx.indexIn( *i );
		QString protocolId = rx.cap( 1 );
		QString accountId = rx.cap( 2 );

		if ( accountId.isEmpty() )
		{
			if ( protocolId.isEmpty() )
				accountId = *i;
			else
				continue;
		}

		QListIterator<Kopete::Account *> it( Kopete::AccountManager::self()->accounts() );
		Kopete::Account *account;
		while ( it.hasNext() )
		{
			account = it.next();
			if ( ( account->accountId() == accountId ) )
			{
				if ( protocolId.isEmpty() || account->protocol()->pluginId() == protocolId )
				{
					account->connect();
					break;
				}
			}
		}
	}

	// Parse any passed URLs/files
	handleURLArgs();
}

int KopeteApplication::newInstance()
{
//	kDebug(14000) << k_funcinfo << endl;
	handleURLArgs();

	return KUniqueApplication::newInstance();
}

void KopeteApplication::handleURLArgs()
{
	KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
//	kDebug(14000) << k_funcinfo << "called with " << args->count() << " arguments to handle." << endl;

	if ( args->count() > 0 )
	{
		for ( int i = 0; i < args->count(); i++ )
		{
			KUrl u( args->url( i ) );
			if ( !u.isValid() )
				continue;

			Kopete::MimeTypeHandler::dispatchURL( u );
		} // END for()
	} // END args->count() > 0
}

void KopeteApplication::quitKopete()
{
	kDebug( 14000 ) << k_funcinfo << endl;

	m_isShuttingDown = true;

	// close all windows
	QList<KMainWindow*> members = KMainWindow::memberList();
	QList<KMainWindow*>::iterator it, itEnd = members.end();
	for ( it = members.begin(); it != itEnd; ++it)
	{
		if ( (*it)->close() )
		{
			m_isShuttingDown = false;
			break;
		}
	}
}


void KopeteApplication::commitData( QSessionManager &sm )
{
	m_isShuttingDown = true;
	KUniqueApplication::commitData( sm );
}

#include "kopeteapplication.moc"
// vim: set noet ts=4 sts=4 sw=4:
