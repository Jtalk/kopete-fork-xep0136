/*
    icqstatusmanager.cpp  -  ICQ Status Manager
    
    Copyright (c) 2004      by Richard Smith          <kde@metafoo.co.uk>
    Copyright (c) 2006,2007 by Roman Jarosz           <kedgedev@centrum.cz>
    Kopete    (c) 2002-2007 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "icqstatusmanager.h"

#include <klocale.h>
#include <ksharedconfig.h>
#include <kglobal.h>
#include <kconfig.h>
#include <kconfiggroup.h>

#include "icqprotocol.h"
#include "oscarpresencesdataclasses.h"

class ICQStatusManager::Private
{
public:
	// connecting and unknown should have the same internal status as offline, so converting to a Presence gives an Offline one
	Private()
		: connecting(     Kopete::OnlineStatus::Connecting, 99, ICQProtocol::protocol(),
					      99,                QStringList(QString("icq_connecting")), i18n("Connecting...") )
		, unknown(        Kopete::OnlineStatus::Unknown,     0, ICQProtocol::protocol(),
		                  Oscar::Presence::Offline, QStringList(QString("status_unknown")), i18n("Unknown") )
		, waitingForAuth( Kopete::OnlineStatus::Unknown,     1, ICQProtocol::protocol(),
		                  Oscar::Presence::Offline, QStringList(QString("dialog-cancel")),  i18n("Waiting for Authorization") )
		, invisible(      Kopete::OnlineStatus::Invisible,   2, ICQProtocol::protocol(),
		                  Oscar::Presence::Offline, QStringList(),    QString(),
						  QString(), Kopete::OnlineStatusManager::Invisible,
						  Kopete::OnlineStatusManager::HideFromMenu )

	{
	}
	
	Kopete::OnlineStatus connecting;
	Kopete::OnlineStatus unknown;
	Kopete::OnlineStatus waitingForAuth;
	Kopete::OnlineStatus invisible;

	QList<Xtraz::Status> xtrazStatusList;
};

ICQStatusManager::ICQStatusManager()
	: OscarStatusManager( ICQProtocol::protocol() ),  d( new Private )
{
	using namespace Oscar;
	using namespace Kopete;
	using namespace StatusCode;

	/**
	 * The order here is important - this is the order the IS_XXX flags will be checked for in.
	 * That, in particular, means that NA, Occupied and DND must appear before Away, and that
	 * DND must appear before Occupied. Offline (testing all bits) must go first, and Online
	 * (testing no bits - will always match a status) must go last.
	 *
	 * Free For Chat is currently listed after Away, since if someone is Away + Free For Chat we
	 * want to show them as Away more than we want to show them FFC.
	 */

	QList<PresenceType> data;

	data << PresenceType( Presence::Offline, OnlineStatus::Offline, OFFLINE, OFFLINE, i18n( "O&ffline" ), i18n("Offline"), QStringList(), Kopete::OnlineStatusManager::Offline, 0, PresenceType::FlagsList() << Presence::None << Presence::AIM << Presence::Invisible );

	data << PresenceType( Presence::DoNotDisturb, OnlineStatus::Away, SET_DND, IS_DND, i18n( "&Do Not Disturb" ), i18n("Do Not Disturb"), QStringList(QString("contact_busy_overlay")), Kopete::OnlineStatusManager::Busy, Kopete::OnlineStatusManager::HasStatusMessage, PresenceType::FlagsList() << Presence::None << Presence::Invisible << Presence::Wireless << (Presence::Wireless | Presence::Invisible) );

	data << PresenceType( Presence::Occupied, OnlineStatus::Away, SET_OCC, IS_OCC, i18n( "O&ccupied" ), i18n("Occupied"), QStringList(QString("contact_busy_overlay")), 0, Kopete::OnlineStatusManager::HasStatusMessage, PresenceType::FlagsList() << Presence::None << Presence::Invisible );

	data << PresenceType( Presence::NotAvailable, OnlineStatus::Away, SET_NA, IS_NA, i18n( "Not A&vailable" ), i18n("Not Available"), QStringList(QString("contact_xa_overlay")), Kopete::OnlineStatusManager::ExtendedAway, Kopete::OnlineStatusManager::HasStatusMessage, PresenceType::FlagsList() << Presence::None << Presence::Invisible );

	data << PresenceType( Presence::Away, OnlineStatus::Away, SET_AWAY, IS_AWAY, i18n( "&Away" ), i18n("Away"), QStringList(QString("contact_away_overlay")), Kopete::OnlineStatusManager::Away, Kopete::OnlineStatusManager::HasStatusMessage, PresenceType::FlagsList() << Presence::None << Presence::Invisible << Presence::AIM << (Presence::AIM | Presence::Invisible) << Presence::Wireless << (Presence::Wireless | Presence::Invisible) );

	data << PresenceType( Presence::FreeForChat,  OnlineStatus::Online,  SET_FFC,  IS_FFC,  i18n( "&Free for Chat" ),  i18n("Free For Chat"), QStringList(QString("icq_ffc")), Kopete::OnlineStatusManager::FreeForChat,  0, PresenceType::FlagsList() << Presence::None << Presence::Invisible );

	data << PresenceType( Presence::Online, OnlineStatus::Online, ONLINE, ONLINE, i18n( "O&nline" ), i18n("Online"), QStringList(), Kopete::OnlineStatusManager::Online, Kopete::OnlineStatusManager::HasStatusMessage, PresenceType::FlagsList() << Presence::None << Presence::Invisible << Presence::AIM << (Presence::AIM | Presence::Invisible) << Presence::Wireless << (Presence::Wireless | Presence::Invisible) );

	setPresenceType( data );

	QList<PresenceOverlay> overlay;
	overlay << PresenceOverlay( Presence::Invisible, i18n("Invisible"), QStringList(QString("contact_invisible_overlay")) );
	overlay << PresenceOverlay( Presence::Wireless, i18n("Mobile"), QStringList(QString("contact_phone_overlay")) );
	overlay << PresenceOverlay( Presence::AIM, i18n("AIM"), QStringList(QString("aim_overlay")) );
	setPresenceOverlay( overlay );

	setPresenceFlagsMask( ~(Presence::Flags)Presence::ICQ );
	//weight 0, 1 and 2 are used by KOS unknown, waitingForAuth and invisible
	initialize( 3 );

	loadXtrazStatuses();
}

ICQStatusManager::~ICQStatusManager()
{
	delete d;
}

Kopete::OnlineStatus ICQStatusManager::connectingStatus() const
{
	return d->connecting;
}

Kopete::OnlineStatus ICQStatusManager::unknownStatus() const
{
	return d->unknown;
}

Kopete::OnlineStatus ICQStatusManager::waitingForAuth() const
{
	return d->waitingForAuth;
}

void ICQStatusManager::loadXtrazStatuses()
{
	KConfigGroup config = KGlobal::config()->group( "Xtraz Statuses" );

	QList<int> statusList = config.readEntry( "Statuses", QList<int>() );
	QList<QString> descriptionList = config.readEntry( "Descriptions", QList<QString>() );
	QList<QString> messageList = config.readEntry( "Messages", QList<QString>() );

	const int count = qMin( qMin( statusList.count(), descriptionList.count() ), messageList.count() );
	for ( int i = 0; i < count; i++ )
	{
		Xtraz::Status status;
		status.setStatus( statusList.at( i ) );
		status.setDescription( descriptionList.at( i ) );
		status.setMessage( messageList.at( i ) );

		d->xtrazStatusList.append( status );
	}
}

void ICQStatusManager::saveXtrazStatuses()
{
	KConfigGroup config = KGlobal::config()->group( "Xtraz Statuses" );

	QList<int> statusList;
	QList<QString> descriptionList;
	QList<QString> messageList;

	for ( int i = 0; i < d->xtrazStatusList.count(); i++ )
	{
		Xtraz::Status status = d->xtrazStatusList.at(i);
		statusList.append( status.status() );
		descriptionList.append( status.description() );
		messageList.append( status.message() );
	}

	config.writeEntry( "Statuses", statusList );
	config.writeEntry( "Descriptions", descriptionList );
	config.writeEntry( "Messages", messageList );

	config.sync();
}

QList<Xtraz::Status> ICQStatusManager::xtrazStatuses() const
{
	return d->xtrazStatusList;
}

void ICQStatusManager::setXtrazStatuses( const QList<Xtraz::Status> &statusList )
{
	d->xtrazStatusList = statusList;
	saveXtrazStatuses();
}

void ICQStatusManager::appendXtrazStatus( const Xtraz::Status &status )
{
	d->xtrazStatusList.append( status );
	saveXtrazStatuses();
}
