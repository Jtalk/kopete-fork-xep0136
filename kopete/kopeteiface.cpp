/*
    kopeteiface.cpp - Kopete DCOP Interface

    Copyright (c) 2002 by Hendrik vom Lehn       <hennevl@hennevl.de>

    Kopete    (c) 2002-2003      by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include <kmessagebox.h>
#include <klocale.h>
#include <kconfig.h>
#include <kglobal.h>

#include "kopeteiface.h"
#include "kopetecontactlist.h"
#include "kopeteaccount.h"
#include "kopeteaccountmanager.h"
#include "kopetepluginmanager.h"
#include "kopeteprotocol.h"
#include "kopeteaway.h"
#include "kopetecontact.h"


KopeteIface::KopeteIface() : DCOPObject( "KopeteIface" )
{
#if 0 //disabled because of feature frezee.   TODO: enable after kde 3.2
	KConfig *config = KGlobal::config();
	config->setGroup("AutoAway");

	if (config->readBoolEntry("UseAutoAway", true))
		connectDCOPSignal("kdesktop", "KScreensaverIface", "KDE_start_screensaver()", "setAutoAway()", false);
	else
		disconnectDCOPSignal("kdesktop", "KScreensaverIface", "KDE_start_screensaver()", "setAutoAway()");
#endif
}

QStringList KopeteIface::contacts()
{
	return KopeteContactList::contactList()->contacts();
}

QStringList KopeteIface::reachableContacts()
{
	return KopeteContactList::contactList()->reachableContacts();
}

QStringList KopeteIface::onlineContacts()
{
	QStringList result;
	QPtrList<KopeteContact> list = KopeteContactList::contactList()->onlineContacts();
	QPtrListIterator<KopeteContact> it( list );
	for( ; it.current(); ++it )
		result.append( it.current()->contactId() );

	return result;
}

QStringList KopeteIface::contactsStatus()
{
	return KopeteContactList::contactList()->contactStatuses();
}

QStringList KopeteIface::fileTransferContacts()
{
	return KopeteContactList::contactList()->fileTransferContacts();
}

QStringList KopeteIface::contactFileProtocols(const QString &displayName)
{
	return KopeteContactList::contactList()->contactFileProtocols(displayName);
}

void KopeteIface::messageContact( const QString &displayName, const QString &messageText )
{
	KopeteContactList::contactList()->messageContact( displayName, messageText );
}
/*
void KopeteIface::sendFile(const QString &displayName, const KURL &sourceURL,
	const QString &altFileName, uint fileSize)
{
	return KopeteContactList::contactList()->sendFile(displayName, sourceURL, altFileName, fileSize);
}

*/
bool KopeteIface::addContact( const QString &protocolName, const QString &accountId, const QString &contactId,
	const QString &displayName, const QString &groupName )
{
		//Get the protocol instance
	KopeteAccount *myAccount = KopeteAccountManager::manager()->findAccount( protocolName, accountId );

	if( myAccount )
	{
		QString contactName;

		//If the nickName isn't specified we need to display the userId in the prompt
		if( displayName.isEmpty() || displayName.isNull() )
			contactName = contactId;
		else
			contactName = displayName;

		// Confirm with the user before we add the contact
		if( KMessageBox::questionYesNo( 0, i18n( "An external application is attempting to add the "
			" '%1' contact '%2' to your contact list. Do you want to allow this?" )
			.arg( protocolName ).arg( contactName ), i18n( "Allow Contact?" ) ) == 3 ) // Yes == 3
		{
			//User said Yes
			myAccount->addContact( contactId, displayName, 0L, KopeteAccount::DontChangeKABC, groupName, false );
			return true;
		} else {
			//User said No
			return false;
		}

	} else {
		//This protocol is not loaded
		KMessageBox::queuedMessageBox( 0L, KMessageBox::Error,
				 i18n("An external application has attempted to add a contact using "
				      " the %1 protocol, which either does not exist or is not loaded.").arg( protocolName ),
				i18n("Missing Protocol"));

		return false;
	}
}

QStringList KopeteIface::accounts()
{
	QStringList list;
	QPtrList<KopeteAccount> m_accounts=KopeteAccountManager::manager()->accounts();
	QPtrListIterator<KopeteAccount> it( m_accounts );
	KopeteAccount *account;
	while ( ( account = it.current() ) != 0 )
	{
		++it;

		list += ( account->protocol()->pluginId() +"||" + account->accountId() );
	}

	return list;

}

void KopeteIface::connect(const QString &protocolId, const QString &accountId )
{
	QPtrListIterator<KopeteAccount> it( KopeteAccountManager::manager()->accounts() );
	KopeteAccount *account;
	while ( ( account = it.current() ) != 0 )
	{
		++it;

		if( ( account->accountId() == accountId) )
		{
			if( protocolId.isEmpty() || account->protocol()->pluginId() == protocolId )
			{
				account->connect();
				break;
			}
		}
	}
}

void KopeteIface::disconnect(const QString &protocolId, const QString &accountId )
{
	QPtrListIterator<KopeteAccount> it( KopeteAccountManager::manager()->accounts() );
	KopeteAccount *account;
	while ( ( account = it.current() ) != 0 )
	{
		++it;

		if( ( account->accountId() == accountId) )
		{
			if( protocolId.isEmpty() || account->protocol()->pluginId() == protocolId )
			{
				account->disconnect();
				break;
			}
		}
	}
}

bool KopeteIface::loadPlugin( const QString& name )
{
	QString argument=name;
	if ( !argument.startsWith( "kopete_" ) )
		argument.prepend( "kopete_" );
	return KopetePluginManager::self()->loadPlugin(argument);
}

bool KopeteIface::unloadPlugin( const QString& name )
{
	QString argument=name;
	if ( !argument.startsWith( "kopete_" ) )
		argument.prepend( "kopete_" );
	return KopetePluginManager::self()->unloadPlugin( argument );
}

void KopeteIface::setAway()
{
	KopeteAccountManager::manager()->setAwayAll();
}

void KopeteIface::setAvailable()
{
	KopeteAccountManager::manager()->setAvailableAll();
}

void KopeteIface::setAutoAway()
{
	KopeteAway::getInstance()->setAutoAway();
}

// vim: set noet ts=4 sts=4 sw=4:

