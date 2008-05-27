/*
   notifypresencetask.cpp - Notify about presence changes of contacts

   Copyright (c) 2006 by Michaël Larouche <larouche@kde.org>

   *************************************************************************
   *                                                                       *
   * This library is free software; you can redistribute it and/or         *
   * modify it under the terms of the GNU Lesser General Public            *
   * License as published by the Free Software Foundation; either          *
   * version 2 of the License, or (at your option) any later version.      *
   *                                                                       *
   *************************************************************************
*/
#include "Papillon/Tasks/NotifyPresenceTask"

// Qt includes
#include <QtCore/QStringList>
#include <QtCore/QLatin1String>
#include <QtDebug>

// Papillon includes
#include "Papillon/NetworkMessage"
#include "Papillon/Global"

namespace Papillon
{

class NotifyPresenceTask::Private
{
public:
	Private()
	{}
};

NotifyPresenceTask::NotifyPresenceTask(Papillon::Task *parent)
 : Papillon::Task(parent), d(new Private)
{}

NotifyPresenceTask::~NotifyPresenceTask()
{
	delete d;
}


bool NotifyPresenceTask::take(NetworkMessage *networkMessage)
{
	if( forMe(networkMessage) )
	{
		QString contactId;
		Papillon::Presence::Status newPresence = Presence::Offline;

		// ILN is initial presence and NLN normal presence change.
		if( networkMessage->command() == QLatin1String("NLN") || networkMessage->command() == QLatin1String("ILN") )
		{
			newPresence = Papillon::Global::stringToPresence( networkMessage->arguments()[0] );
			contactId = networkMessage->arguments()[1];
			// TODO: Handle nickname, features, MsnObject
		}
		// Contact went offline
		else if( networkMessage->command() == QLatin1String("FLN") )
		{
			newPresence = Presence::Offline;
			contactId = networkMessage->arguments()[0];
		}

		emit contactPresenceChanged(contactId, newPresence);

		return true;
	}

	return false;
}

bool NotifyPresenceTask::forMe(NetworkMessage *networkMessage) const
{
	if( networkMessage->command() == QLatin1String("ILN") ||
		networkMessage->command() == QLatin1String("NLN") ||
		networkMessage->command() == QLatin1String("FLN") )
	{
		return true;
	}
	
	return false;
}

}

#include "notifypresencetask.moc"
