//
// C++ Implementation: privacymanager
//
// Description: 
//
//
// Author: SUSE AG <>, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "client.h"
#include "tasks/privacyitemtask.h"
#include "userdetailsmanager.h"

#include "privacymanager.h"

PrivacyManager::PrivacyManager( Client * client, const char *name)
 : QObject(client, name), m_client( client )
{
}

PrivacyManager::~PrivacyManager()
{
}

bool PrivacyManager::defaultAllow()
{
	return !m_defaultDeny;
}

bool PrivacyManager::defaultDeny()
{
	return m_defaultDeny;
}

QStringList PrivacyManager::allowList()
{
	return m_allowList;
}

QStringList PrivacyManager::denyList()
{
	return m_denyList;
}

bool PrivacyManager::isPrivacyLocked()
{
	return m_locked;
}

bool PrivacyManager::isBlocked( const QString & dn )
{
	if ( m_defaultDeny )
		return !m_allowList.contains( dn );
	else
		return m_denyList.contains( dn );
}

void PrivacyManager::setAllow( const QString & dn )
{
	if ( m_defaultDeny )
	{
		if ( !m_allowList.contains( dn ) )
			addAllow( dn );
	}
	else
	{
		if ( m_denyList.contains( dn ) )
			removeDeny( dn );
	}
}

void PrivacyManager::setDeny( const QString & dn )
{
	if ( m_defaultDeny )
	{
		if ( m_allowList.contains( dn ) )
			removeAllow( dn );
	}
	else
	{
		if ( !m_denyList.contains( dn ) )
			addDeny( dn );
	}
}


void PrivacyManager::setDefaultAllow( bool allow )
{
	PrivacyItemTask * pit = new PrivacyItemTask( m_client->rootTask() );
	pit->defaultPolicy( !allow );
	connect( pit, SIGNAL( finished() ), SLOT( slotDefaultPolicyChanged() ) );
	pit->go( true );
}

void PrivacyManager::setDefaultDeny( bool deny )
{
	PrivacyItemTask * pit = new PrivacyItemTask( m_client->rootTask() );
	pit->defaultPolicy( deny);
	connect( pit, SIGNAL( finished() ), SLOT( slotDefaultPolicyChanged() ) );
	pit->go( true );
}

void PrivacyManager::addAllow( const QString & dn )
{
	// start off a CreatePrivacyItemTask
	PrivacyItemTask * pit = new PrivacyItemTask( m_client->rootTask() );
	pit->allow( dn );
	connect( pit, SIGNAL( finished() ), SLOT( slotAllowAdded() ) );
	pit->go( true );
}

void PrivacyManager::addDeny( const QString & dn )
{
	// start off a CreatePrivacyItemTask
	PrivacyItemTask * pit = new PrivacyItemTask( m_client->rootTask() );
	pit->deny( dn );
	connect( pit, SIGNAL( finished() ), SLOT( slotDenyAdded() ) );
	pit->go( true );
}

void PrivacyManager::removeAllow( const QString & dn )
{
	PrivacyItemTask * pit = new PrivacyItemTask( m_client->rootTask() );
	pit->removeAllow( dn );
	connect( pit, SIGNAL( finished() ), SLOT( slotAllowRemoved() ) );
	pit->go( true );
}

void PrivacyManager::removeDeny( const QString & dn )
{
	// start off a CreatePrivacyItemTask
	PrivacyItemTask * pit = new PrivacyItemTask( m_client->rootTask() );
	pit->removeDeny( dn );
	connect( pit, SIGNAL( finished() ), SLOT( slotDenyRemoved() ) );
	pit->go( true );
}

void PrivacyManager::setPrivacy( bool defaultIsDeny, const QStringList & allowList, const QStringList & denyList )
{
	if ( defaultIsDeny != m_defaultDeny )
		setDefaultDeny( defaultIsDeny );
	// find the DNs no longer in the allow list
	QStringList allowsToRemove = difference( m_allowList, allowList );
	// find the DNs no longer in the deny list
	QStringList denysToRemove = difference( m_denyList, denyList );
	// find the DNs new in the allow list
	QStringList allowsToAdd = difference( allowList, m_allowList );
	// find the DNs new in the deny list
	QStringList denysToAdd = difference( denyList, m_denyList );
	
	QStringList::ConstIterator end = allowsToRemove.end();
	for ( QStringList::ConstIterator it = allowsToRemove.begin(); it != end; ++it )
		removeAllow( *it );
	end = denysToRemove.end();
	for ( QStringList::ConstIterator it = denysToRemove.begin(); it != end; ++it )
		removeDeny( *it );
	end = allowsToAdd.end();
	for ( QStringList::ConstIterator it = allowsToAdd.begin(); it != end; ++it )
		addAllow( *it );
	end = denysToAdd.end();
	for ( QStringList::ConstIterator it = denysToAdd.begin(); it != end; ++it )
		addDeny( *it );
}

void PrivacyManager::slotGotPrivacySettings( bool locked, bool defaultDeny, const QStringList & allowList, const QStringList & denyList )
{
	m_locked = locked;
	m_defaultDeny = defaultDeny;
	m_allowList = allowList;
	m_denyList = denyList;
}

void PrivacyManager::getDetailsForPrivacyLists()
{
	if ( !m_allowList.isEmpty() )
	{
		m_client->userDetailsManager()->requestDetails( m_allowList );
	}
	if ( !m_denyList.isEmpty() )
		m_client->userDetailsManager()->requestDetails( m_denyList );
}

void PrivacyManager::slotDefaultPolicyChanged()
{
	PrivacyItemTask * pit = ( PrivacyItemTask * )sender();
	if ( pit->success() )
		m_defaultDeny = pit->defaultDeny();
}

void PrivacyManager::slotAllowAdded()
{
	PrivacyItemTask * pit = ( PrivacyItemTask * )sender();
	if ( pit->success() )
	{
		m_allowList.append( pit->dn() );
		emit privacyChanged( pit->dn(), isBlocked( pit->dn() ) );
	}
}

void PrivacyManager::slotDenyAdded()
{
	PrivacyItemTask * pit = ( PrivacyItemTask * )sender();
	if ( pit->success() )
	{
		m_denyList.append( pit->dn() );
		emit privacyChanged( pit->dn(), isBlocked( pit->dn() ) );
	}
}

void PrivacyManager::slotAllowRemoved()
{
	PrivacyItemTask * pit = ( PrivacyItemTask * )sender();
	if ( pit->success() )
	{
		m_allowList.remove( pit->dn() );
		emit privacyChanged( pit->dn(), isBlocked( pit->dn() ) );
	}
}

void PrivacyManager::slotDenyRemoved()
{
	PrivacyItemTask * pit = ( PrivacyItemTask * )sender();
	if ( pit->success() )
	{
		m_denyList.remove( pit->dn() );
		emit privacyChanged( pit->dn(), isBlocked( pit->dn() ) );
	}
}

QStringList PrivacyManager::difference( const QStringList & lhs, const QStringList & rhs )
{
	QStringList diff;
	const QStringList::ConstIterator lhsEnd = lhs.end();
	const QStringList::ConstIterator rhsEnd = rhs.end();
	for ( QStringList::ConstIterator lhsIt = lhs.begin(); lhsIt != lhsEnd; ++lhsIt )
	{
		if ( rhs.find( *lhsIt ) == rhsEnd )
			diff.append( *lhsIt );
	}
	return diff;
}
#include "privacymanager.moc"
