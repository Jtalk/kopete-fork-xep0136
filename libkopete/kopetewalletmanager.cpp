/*
    kopetewalletmanager.cpp - Kopete Wallet Manager

    Copyright (c) 2004      by Richard Smith         <kde@metafoo.co.uk>
    Kopete    (c) 2002-2004 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include <kdeversion.h>
#if KDE_IS_VERSION( 3, 1, 90 )

#include "kopetewalletmanager.h"

#include <kstaticdeleter.h>
#include <kwallet.h>

#include <qtimer.h>

struct KopeteWalletManager::KopeteWalletManagerPrivate
{
	KopeteWalletManagerPrivate() : wallet(0) {}
	~KopeteWalletManagerPrivate() { delete wallet; }

	KWallet::Wallet *wallet;
};

KopeteWalletManager::KopeteWalletManager()
 : d( new KopeteWalletManagerPrivate )
{
}

KopeteWalletManager::~KopeteWalletManager()
{
	closeWallet();
	delete d;
}

namespace
{
	KStaticDeleter<KopeteWalletManager> s_deleter;
	KopeteWalletManager *s_self = 0;
}

KopeteWalletManager *KopeteWalletManager::self()
{
	if ( !s_self )
		s_deleter.setObject( s_self, new KopeteWalletManager() );
	return s_self;
}

void KopeteWalletManager::openWallet()
{
	// do we already have a wallet?
	if ( d->wallet )
	{
		// if the wallet isn't open yet, we're pending a slotWalletChangedStatus
		// anyway, so we don't set up a single shot.
		if ( d->wallet->isOpen() )
			QTimer::singleShot( 0, this, SLOT( slotGiveExistingWallet() ) );
		return;
	}

	// we have no wallet: ask for one.
	d->wallet = KWallet::Wallet::openWallet( KWallet::Wallet::NetworkWallet(),
	            /* FIXME: put a real wId here */ 0, KWallet::Wallet::Asynchronous );

	connect( d->wallet, SIGNAL( walletOpened(bool) ), this, SLOT( slotWalletChangedStatus() ) );
}

void KopeteWalletManager::slotWalletChangedStatus()
{
	if( d->wallet->isOpen() )
	{
		if ( !d->wallet->hasFolder( QString::fromLatin1( "Kopete" ) ) )
			d->wallet->createFolder( QString::fromLatin1( "Kopete" ) );

		if ( d->wallet->setFolder( QString::fromLatin1( "Kopete" ) ) )
		{
			// success!
			QObject::connect( d->wallet, SIGNAL( walletClosed() ), this, SLOT( closeWallet() ) );
		}
		else
		{
			// opened OK, but we can't use it
			delete d->wallet;
			d->wallet = 0;
		}
	}
	else
	{
		// failed to open
		delete d->wallet;
		d->wallet = 0;
	}

	emit walletOpened( d->wallet );
}

void KopeteWalletManager::slotGiveExistingWallet()
{
	if ( d->wallet )
	{
		// the wallet was already open
		if ( d->wallet->isOpen() )
			emit walletOpened( d->wallet );
		// if the wallet was not open, but d->wallet is not 0,
		// then we're waiting for it to open, and will be told
		// when it's done: do nothing.
	}
	else
	{
		// the wallet was lost between us trying to open it and
		// getting called back. try to reopen it.
		openWallet();
	}
}

KWallet::Wallet *KopeteWalletManager::wallet()
{
	if ( !KWallet::Wallet::isEnabled() )
		return 0;

	if ( d->wallet && d->wallet->isOpen() )
		return d->wallet;
	
	delete d->wallet;
	d->wallet = KWallet::Wallet::openWallet( KWallet::Wallet::NetworkWallet(),
	            /* FIXME: put a real wId here */ 0, KWallet::Wallet::Synchronous );

	if ( d->wallet )
		slotWalletChangedStatus();

	return d->wallet;
}

void KopeteWalletManager::closeWallet()
{
	if ( !d->wallet ) return;

	delete d->wallet;
	d->wallet = 0L;

	emit walletLost();
}


#include "kopetewalletmanager.moc"

#endif // KDE_IS_VERSION

// vim: set noet ts=4 sts=4 sw=4:

