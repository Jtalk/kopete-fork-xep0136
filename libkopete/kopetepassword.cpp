/*
    kopetepassword.cpp - Kopete Password

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

#include "kopetepassword.h"
#include "kopetepassworddialog.h"
#include "kopetewalletmanager.h"

#include <qapplication.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qobjectcleanuphandler.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kdeversion.h>
#include <kdialogbase.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kwallet.h>
#include <kiconloader.h>

namespace
{

/**
 * Function for symmetrically (en/de)crypting strings for config file,
 * taken from KMail.
 *
 * @author Stefan Taferner <taferner@alpin.or.at>
 */
QString cryptStr( const QString &aStr )
{
	QString result;
	for ( uint i = 0; i < aStr.length(); i++ )
		result += ( aStr[ i ].unicode() < 0x20) ? aStr[ i ] : QChar( 0x1001F - aStr[ i ].unicode() );

	return result;
}

}

struct KopetePassword::KopetePasswordPrivate
{
	KopetePasswordPrivate( const QString &group )
	 : configGroup( group ), remembered( false )
	{
	}
	/** Group to use for KConfig and KWallet */
	QString configGroup;
	/** Is the password being remembered? */
	bool remembered;
	/** The current password in the KConfig file, or QString::null if no password there */
	QString passwordFromKConfig;
};

/**
 * Implementation detail of KopetePassword: manages a single password request
 * @internal
 * @author Richard Smith <kde@metafoo.co.uk>
 */
class KopetePasswordRequest : public KopetePasswordRequestBase
{
public:
	KopetePasswordRequest( KopetePassword &pass )
	 : QObject( &pass ), mPassword( pass ), mWallet( 0 )
	{
	}

	/**
	 * Start the request - ask for the wallet
	 */
	void begin()
	{
		kdDebug( 14010 ) << k_funcinfo << endl;
		KopeteWalletManager::self()->openWallet( this, SLOT( walletReceived( KWallet::Wallet* ) ) );
	}

	void walletReceived( KWallet::Wallet *wallet )
	{
		kdDebug( 14010 ) << k_funcinfo << endl;
		mWallet = wallet;
		processRequest();
	}

	/**
	 * Got wallet; now carry out whatever action this request represents
	 */
	virtual void processRequest() = 0;

	void slotOkPressed() {}
	void slotCancelPressed() {}

protected:
	KopetePassword &mPassword;
	KWallet::Wallet *mWallet;
};

/**
 * Implementation detail of KopetePassword: manages a single password retrieval request
 * @internal
 * @author Richard Smith <kde@metafoo.co.uk>
 */
class KopetePasswordGetRequest : public KopetePasswordRequest
{
public:
	KopetePasswordGetRequest( KopetePassword &pass )
	 : KopetePasswordRequest( pass )
	{
	}

	QString grabPassword()
	{
		// Before trying to read from the wallet, check if the config file holds a password.
		// If so, remove it from the config and set it through KWallet instead.
		QString pwd;
		if ( mPassword.d->remembered && !mPassword.d->passwordFromKConfig.isNull() )
		{
			pwd = mPassword.d->passwordFromKConfig;
			mPassword.set( pwd );
			return pwd;
		}
	
		if ( mWallet && mWallet->readPassword( mPassword.d->configGroup, pwd ) == 0 && !pwd.isNull() )
			return pwd;
	
		if ( mPassword.d->remembered && !mPassword.d->passwordFromKConfig.isNull() )
			return mPassword.d->passwordFromKConfig;
	
		return QString::null;
	}

	void finished( const QString &result )
	{
		emit requestFinished( result );
		delete this;
	}
};

class KopetePasswordGetRequestPrompt : public KopetePasswordGetRequest
{
public:
	KopetePasswordGetRequestPrompt( KopetePassword &pass,  const QPixmap &image, const QString &prompt, bool error, unsigned int maxLength )
	 : KopetePasswordGetRequest( pass ), mImage( image ), mPrompt( prompt ), mError( error ), mMaxLength( maxLength ), mView( 0 )
	{
	}

	void processRequest()
	{
		QString result = grabPassword();
		if ( mError || result.isNull() )
			doPasswordDialog( result );
		else
			finished( result );
	}

	void doPasswordDialog( const QString &password )
	{
		kdDebug( 14010 ) << k_funcinfo << endl;

		KDialogBase *passwdDialog = new KDialogBase( qApp->mainWidget(), "passwdDialog", true, i18n( "Password Required" ),
			KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Ok, true );
	
		mView = new KopetePasswordDialog( passwdDialog );
		passwdDialog->setMainWidget( mView );
	
		mView->m_text->setText( mPrompt );
		mView->m_image->setPixmap( mImage );
		mView->m_password->setText( password );
		if ( mMaxLength != 0 )
			mView->m_password->setMaxLength( mMaxLength );
	
		// FIXME: either document what these are for or remove them - lilac
		mView->adjustSize();
		passwdDialog->adjustSize();

		connect( passwdDialog, SIGNAL( okClicked() ), SLOT( slotOkPressed() ) );
		connect( passwdDialog, SIGNAL( cancelClicked() ), SLOT( slotCancelPressed() ) );
		connect( this, SIGNAL( destroyed() ), passwdDialog, SLOT( deleteLater() ) );
		passwdDialog->show();
	}

	void slotOkPressed()
	{
		QString result = mView->m_password->text();
		if ( mView->m_save_passwd->isChecked() )
			mPassword.set( result );
	
		finished( result );
	}

	void slotCancelPressed()
	{
		finished( QString::null );
	}

private:
	QPixmap mImage;
	QString mPrompt;
	bool mError;
	unsigned int mMaxLength;
	KopetePasswordDialog *mView;
};

class KopetePasswordGetRequestNoPrompt : public KopetePasswordGetRequest
{
public:
	KopetePasswordGetRequestNoPrompt( KopetePassword &pass )
	 : KopetePasswordGetRequest( pass )
	{
	}

	void processRequest()
	{
		finished( grabPassword() );
	}
};

/**
 * Implementation detail of KopetePassword: manages a single password change request
 * @internal
 * @author Richard Smith <kde@metafoo.co.uk>
 */
class KopetePasswordSetRequest : public KopetePasswordRequest
{
public:
	KopetePasswordSetRequest( KopetePassword &pass, const QString &newPass )
	 : KopetePasswordRequest( pass ), mNewPass( newPass )
	{
		if ( KApplication *app = KApplication::kApplication() )
			app->ref();
	}
	~KopetePasswordSetRequest()
	{
		if ( KApplication *app = KApplication::kApplication() )
			app->deref();
		kdDebug( 14010 ) << k_funcinfo << "job complete" << endl;
	}
	void processRequest()
	{
		setPassword();
		delete this;
	}
	void setPassword()
	{
		if ( mNewPass.isNull() )
		{
			kdDebug( 14010 ) << k_funcinfo << " clearing password" << endl;
	
			mPassword.d->remembered = false;
			mPassword.d->passwordFromKConfig = QString::null;
			mPassword.writeConfig();
			if ( mWallet )
				mWallet->removeEntry( mPassword.d->configGroup );
			return;
		}
	
		kdDebug( 14010 ) << k_funcinfo << " setting password for " << mPassword.d->configGroup << endl;
	
		if ( mWallet && mWallet->writePassword( mPassword.d->configGroup, mNewPass ) == 0 )
		{
			mPassword.d->remembered = true;
			mPassword.d->passwordFromKConfig = QString::null;
			mPassword.writeConfig();
			return;
		}

		if ( KWallet::Wallet::isEnabled() )
		{
			// If we end up here, the wallet is enabled, but failed somehow.
			// Ask the user what to do now.

			// if the KopetePassword object is destroyed during this message box's
			// nested event loop, and the user clicks 'Store Unsafe', we will crash!

			// solution: reparent to none, and track the parent. if it's deleted, don't use it.
			parent()->removeChild( this );
			QObjectCleanupHandler watchParent;
			watchParent.add( &mPassword );

			if ( KMessageBox::warningContinueCancel( qApp->mainWidget(),
			        i18n( "<qt>Kopete is unable to save your password securely in your wallet!<br>"
			              "Do you want to save the password in the <b>unsafe</b> configuration file instead?</qt>" ),
			        i18n( "Unable to Store Secure Password" ),
			        KGuiItem( i18n( "Store &Unsafe" ), QString::fromLatin1( "unlock" ) ),
			        QString::fromLatin1( "KWalletFallbackToKConfig" ) ) != KMessageBox::Continue )
			{
				return;
			}

			// if our parent was deleted, we can't save the password.
			// this is a corner case, so we don't worry about handling it properly. just make
			// sure we don't crash; tell the user and abort.
			if ( watchParent.isEmpty() )
			{
				KMessageBox::error( qApp->mainWidget(), i18n( "Sorry, your password could not be saved at this time." ),
				                    i18n( "Unable to Store Password" ) );
				return;
			}
		}
	
		mPassword.d->remembered = true;
		mPassword.d->passwordFromKConfig = mNewPass;
		mPassword.writeConfig();
	}

private:
	QString mNewPass;
};

KopetePassword::KopetePassword( const QString &configGroup, const char *name )
 : QObject( 0, name ), d( new KopetePasswordPrivate( configGroup ) ) 
{
	readConfig();
}

KopetePassword::~KopetePassword()
{
	delete d;
}

void KopetePassword::readConfig()
{
	KConfig *config = KGlobal::config();
	config->setGroup( d->configGroup );

	QString passwordCrypted = config->readEntry( "Password" );
	if ( passwordCrypted.isNull() )
		d->passwordFromKConfig = QString::null;
	else
		d->passwordFromKConfig = cryptStr( passwordCrypted );

	d->remembered = config->readBoolEntry( "RememberPassword", false );
}

void KopetePassword::writeConfig()
{
	KConfig *config = KGlobal::config();
	config->setGroup( d->configGroup );

	if ( d->remembered && !d->passwordFromKConfig.isNull() )
		config->writeEntry( "Password", cryptStr( d->passwordFromKConfig ) );
	else
		config->deleteEntry( "Password" );

	config->writeEntry( "RememberPassword", d->remembered );
}

int KopetePassword::preferredImageSize()
{
	return IconSize(KIcon::Toolbar);
}

void KopetePassword::requestWithoutPrompt( QObject *returnObj, const char *slot )
{
	KopetePasswordRequest *request = new KopetePasswordGetRequestNoPrompt( *this );
	connect( request, SIGNAL( requestFinished( const QString & ) ), returnObj, slot );
	request->begin();
}

void KopetePassword::request( QObject *returnObj, const char *slot, const QPixmap &image, const QString &prompt, bool error, unsigned int maxLength )
{
	KopetePasswordRequest *request = new KopetePasswordGetRequestPrompt( *this, image, prompt, error, maxLength );
	connect( request, SIGNAL( requestFinished( const QString & ) ), returnObj, slot );
	request->begin();
}

QString KopetePassword::retrieve( const QPixmap &image, const QString &prompt, bool error, unsigned int maxLength )
{
	if ( !error )
	{
		if( KWallet::Wallet *wallet = KopeteWalletManager::self()->wallet() )
		{
			// Before trying to read from the wallet, check if the config file holds a password.
			// If so, remove it from the config and set it through KWallet instead.
			QString pwd;
			if ( d->remembered && !d->passwordFromKConfig.isNull() )
			{
				pwd = d->passwordFromKConfig;
				set( pwd );
				return pwd;
			}

			if ( wallet->readPassword( d->configGroup, pwd ) == 0 && !pwd.isNull() )
				return pwd;
		}

		if ( d->remembered && !d->passwordFromKConfig.isNull() )
			return d->passwordFromKConfig;
	}
	else
	{
		// Error? Invalidate any stored pass
		set();
	}

	// FIXME: why is this allocated on the heap?
	KDialogBase *passwdDialog = new KDialogBase( qApp->mainWidget(), "passwdDialog", true, i18n( "Password Required" ),
		KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Ok, true );

	KopetePasswordDialog *view = new KopetePasswordDialog( passwdDialog );
	passwdDialog->setMainWidget( view );

	view->m_text->setText( prompt );
	view->m_image->setPixmap( image );
	if ( maxLength != 0 )
		view->m_password->setMaxLength( maxLength );

	// FIXME: either document what these are for or remove them - lilac
	view->adjustSize();
	passwdDialog->adjustSize();

	QString pass;
	if ( passwdDialog->exec() == QDialog::Accepted )
	{
		d->remembered = view->m_save_passwd->isChecked();
		pass = view->m_password->text();
		if ( d->remembered )
			set( pass );
	}

	passwdDialog->deleteLater();
	return pass;
}

void KopetePassword::set( const QString &pass )
{
	KopetePasswordRequest *request = new KopetePasswordSetRequest( *this, pass );
	request->begin();

#if 0
	if ( pass.isNull() )
	{
		kdDebug( 14010 ) << k_funcinfo << " clearing password" << endl;

		// FIXME: This is a quick workaround for the problem that after Jason
		//        added the rememberPassword flag he didn't accordingly update
		//        all plugins to setRememberPassword( false ), so they now
		//        try to set a null pass when the pass is not to be remembered.
		//
		//        After KDE 3.2 this should be fixed by disallowing null
		//        passwords here and adding said property setter method - Martijn
		d->remembered = false;
		d->passwordFromKConfig = QString::null;
		writeConfig();
		return;
	}

	kdDebug( 14010 ) << k_funcinfo << " setting password for " << d->configGroup << endl;

	if ( KWallet::Wallet::isEnabled() )
	{
		if ( KWallet::Wallet *wallet = KopeteWalletManager::self()->wallet() )
		{
			if ( wallet->writePassword( d->configGroup, pass ) == 0 )
			{
				d->remembered = true;
				d->passwordFromKConfig = QString::null;
				writeConfig();
				return;
			}
		}

		// If we end up here, the wallet is enabled, but failed somehow.
		// Ask the user what to do now.
		if ( KMessageBox::warningContinueCancel( qApp->mainWidget(),
			i18n( "<qt>Kopete is unable to save your password securely in your wallet!<br>"
			"Do you want to save the password in the <b>unsafe</b> configuration file instead?</qt>" ),
			i18n( "Unable to Store Secure Password" ),
			KGuiItem( i18n( "Store &Unsafe" ), QString::fromLatin1( "unlock" ) ),
			QString::fromLatin1( "KWalletFallbackToKConfig" ) ) != KMessageBox::Continue )
		{
			return;
		}
	}

	d->remembered = true;
	d->passwordFromKConfig = pass;
	writeConfig();
#endif
}

bool KopetePassword::remembered()
{
	return d->remembered;
}

#include "kopetepassword.moc"

// vim: set noet ts=4 sts=4 sw=4:

