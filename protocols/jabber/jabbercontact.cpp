 /*
    jabbercontact.cpp  -  Base class for the Kopete Jabber protocol contact

    Copyright (c) 2002 by Daniel Stone <dstone@kde.org>
    Copyright (c) 2002 by Till Gerken <till@tantalo.net>

    Kopete    (c) 2002 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include <qapplication.h>
#include <qcursor.h>
#include <qfont.h>
#include <qptrlist.h>
#include <qstrlist.h>

#include <kaction.h>
#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>

#include "tasks.h"
#include "types.h"

#include "dlgjabbervcard.h"
#include "dlgjabberrename.h"
#include "jabbercontact.h"
#include "jabberprotocol.h"
#include "jabberresource.h"
#include "kopetestdaction.h"
#include "kopetemessage.h"
#include "kopetemessagemanager.h"
#include "kopetemessagemanagerfactory.h"
#include "kopetemetacontact.h"
#include "kopetegroup.h"
#include "kopetecontactlist.h"
#include "kopeteviewmanager.h"

/**
 * JabberContact constructor
 */
JabberContact::JabberContact(QString userId, QString nickname, QStringList groups, JabberProtocol *p, KopeteMetaContact *mc, QString identity)
: KopeteContact( p, userId, mc )
{

	// save parent protocol object
	protocol = p;

	parentMetaContact = mc;

	actionCollection = 0L;

	resourceOverride = false;

	messageManager = 0L;

	mIdentityId = identity;

	rosterItem.setJid(Jabber::Jid(userId));
	rosterItem.setName(nickname);
	rosterItem.setGroups(groups);

	initActions();

	// create a default (empty) resource for the contact
	JabberResource *defaultResource = new JabberResource( QString::null, -1, QDateTime::currentDateTime(), JabberProtocol::protocol()->JabberOffline, "" );
	resources.append(defaultResource);

	activeResource = defaultResource;

	// update the displayed name
	setDisplayName(rosterItem.name());

	// specifically cause this instance to update this contact as offline
	slotUpdatePresence( p->JabberOffline, QString::null );
}

/* Return the identity ID */
QString JabberContact::identityId() const
{
	return mIdentityId;
}

/* Return the user ID */
QString JabberContact::userId() const
{
	return rosterItem.jid().userHost();
}

/* Return the currently used resource for this contact */
QString JabberContact::resource() const
{
	return activeResource->resource();
}

KopeteMessageManager *JabberContact::manager( bool )
{

	// create a new message manager if there is none
	if(!messageManager)
	{
		kdDebug(JABBER_DEBUG_GLOBAL) << "[JabberContact] Creating new message manager." << endl;

		KopeteContactPtrList contactList;
		contactList.append(this);

		messageManager = KopeteMessageManagerFactory::factory()->create(
							protocol->myself(), contactList, protocol);

		QObject::connect(messageManager, SIGNAL(destroyed()),
			this, SLOT(slotMessageManagerDeleted()));
		QObject::connect(messageManager, SIGNAL(messageSent(KopeteMessage &, KopeteMessageManager *)),
			this, SLOT(slotSendMessage(KopeteMessage &)));
	}

	return static_cast<KopeteMessageManager*>(messageManager);

}

void JabberContact::slotMessageManagerDeleted()
{

	kdDebug(JABBER_DEBUG_GLOBAL) << "[JabberContact] Message manager has been deleted." << endl;

	messageManager = 0L;

}

/* Return the group this contact resides in */
KopeteGroupList JabberContact::groups() const
{
	QStringList groupStrings;
	groupStrings = rosterItem.groups();

	KopeteGroupList result;
	for( QStringList::ConstIterator it = groupStrings.begin(); it != groupStrings.end(); ++it )
	{
		KopeteGroup *group_;
		group_ = KopeteContactList::contactList()->getGroup( *it, KopeteGroup::Classic);
		if ( group_ )
			result.append( group_ );
	}
	return result;
}

/* Return the reason why we are away */
QString JabberContact::reason() const
{
	return awayReason;
}

/* Return if we are reachable (defaults to true because
   we can send on- and offline */
bool JabberContact::isReachable()
{
	return true;
}

/**
 * Create actions
 */
void JabberContact::initActions()
{
	actionRename = new KAction(i18n("Rename Contact"), "editrename", 0, this, SLOT(slotRenameContact()), this, "actionRename");
	actionSelectResource = new KSelectAction(i18n("Select Resource"), "selectresource", 0, this, SLOT(slotSelectResource()), this, "actionSelectResource");
	actionSendAuth = new KAction(i18n("(Re)send Authorization To"), "", 0, this, SLOT(slotSendAuth()), this, "actionSendAuth");
	actionRequestAuth = new KAction(i18n("(Re)request Authorization From"), "", 0, this, SLOT(slotRequestAuth()), this, "actionRequestAuth");
	actionSetAvailability = new KActionMenu(i18n("Set Availability"), "jabber_online");
	actionStatusOnline = new KAction(i18n("Online"), "jabber_online", 0, this, SLOT(slotStatusOnline()), this, "actionOnline");
	actionStatusChatty = new KAction(i18n("Free to Chat"), "jabber_chatty", 0, this, SLOT(slotStatusChatty()), this, "actionChatty");
	actionStatusAway = new KAction(i18n("Away"), "jabber_away", 0, this,SLOT(slotStatusAway()), this,  "actionAway");
	actionStatusXA = new KAction(i18n("Extended Away"), "jabber_away", 0, this, SLOT(slotStatusXA()),this, "actionXA");
	actionStatusDND = new KAction(i18n("Do Not Disturb"), "jabber_na", 0, this, SLOT(slotStatusDND()), this, "actionDND");
	actionStatusInvisible = new KAction(i18n("Invisible"), "jabber_invisible", 0, this, SLOT(slotStatusInvisible()), this, "actionInvisible");

	actionSetAvailability->insert(actionStatusOnline);
	actionSetAvailability->insert(actionStatusChatty);
	actionSetAvailability->insert(actionStatusAway);
	actionSetAvailability->insert(actionStatusXA);
	actionSetAvailability->insert(actionStatusDND);
	actionSetAvailability->insert(actionStatusInvisible);

}

KActionCollection *JabberContact::customContextMenuActions()
{

	delete actionCollection;

	actionCollection = new KActionCollection(this);

	KGlobal::config()->setGroup("Jabber");


	// if the contact is online,
	// display the resources we have for it
	if( onlineStatus().status() != KopeteOnlineStatus::Offline )
	{
		QStringList items;
		int activeItem = 0;
		JabberResource *tmpBestResource = bestResource();

		// put best resource first
		items.append(i18n("Automatic (best resource)"));

		if(!tmpBestResource->resource().isNull())
			items.append(tmpBestResource->resource());

		// iterate through available resources
		int i = 1;
		for (JabberResource *tmpResource = resources.first(); tmpResource; tmpResource = resources.next(), i++)
		{
			// skip the default (empty) resource
			if(tmpResource->resource().isNull())
			{
				i--;
				continue;
			}

			// only add the item if it is not the best resource
			// (which we already added above)
			if (tmpResource != tmpBestResource)
				items.append(tmpResource->resource());

			// mark the currently active resource
			if (tmpResource->resource() == activeResource->resource() && resourceOverride)
			{
				kdDebug(14130) << "[JabberContact] Activating item " << i << " as active resource." << endl;
				activeItem = i;
			}
		}

		// attach list to the menu action
		actionSelectResource->setItems(items);

		// make sure the active item is selected
		actionSelectResource->setCurrentItem(activeItem);

		// plug it to the menu
		actionCollection->insert(actionSelectResource);
	}

	actionCollection->insert(actionSendAuth);
	actionCollection->insert(actionRequestAuth);

	// Availability popup menu
	actionCollection->insert(actionSetAvailability);

	return actionCollection;

}

void JabberContact::slotUpdatePresence( const KopeteOnlineStatus &newStatus, const QString &reason )
{
	awayReason = reason;

	setOnlineStatus( newStatus );
}

void JabberContact::slotUpdateContact(const Jabber::RosterItem &item)
{

	rosterItem = item;

	// only update the nickname if its not empty
	if(!item.name().isEmpty() && !item.name().isNull())
		setDisplayName(item.name());

}

void JabberContact::slotRenameContact()
{

	kdDebug(14130) << "[JabberContact] Renaming contact." << endl;

	dlgJabberRename *renameDialog = new dlgJabberRename;

	renameDialog->setUserId(userId());
	renameDialog->setNickname(displayName());

	connect(renameDialog, SIGNAL(rename(const QString &)), this, SLOT(slotDoRenameContact(const QString &)));

	renameDialog->show();

}

void JabberContact::slotDoRenameContact(const QString &nickname)
{
	QString name = nickname;

	// if the name has been deleted, revert
	// to using the user ID instead
	if (name == QString(""))
		name = userId();

	rosterItem.setName(name);

	// send rename request to protocol backend
	protocol->updateContact(rosterItem);

	// update display (we cannot use setDisplayName()
	// as parameter here as the above call is asynch
	// and thus our changes did not make it to the server
	// yet)
	setDisplayName(name);

}

void JabberContact::slotDeleteContact()
{

	kdDebug(14130) << "[JabberContact] Removing user " << userId() << endl;

	// unsubscribe
	protocol->removeContact(rosterItem);

}

void JabberContact::slotSendAuth()
{

	kdDebug(14130) << "[JabberContact] (Re)send auth " << userId() << endl;

	protocol->subscribed(Jabber::Jid(userId()));

}

void JabberContact::slotRequestAuth()
{

	kdDebug(14130) << "[JabberContact] (Re)request auth " << userId() << endl;

	protocol->subscribe(Jabber::Jid(userId()));

}

void JabberContact::addToGroup(KopeteGroup *group)
{

	kdDebug(14130) << "[JabberContact] Adding user " << userId() << " to group " << group->displayName() << endl;

	QStringList groups = rosterItem.groups();
	groups.append(group->displayName());
	rosterItem.setGroups(groups);

	protocol->updateContact(rosterItem);

}

void JabberContact::moveToGroup(KopeteGroup *from, KopeteGroup *to)
{

	kdDebug(14130) << "[JabberContact] Moving user " << userId() << " from group " << from->displayName() << " to group " << to->displayName() << endl;

	QStringList groups = rosterItem.groups();
	groups.append(to->displayName());
	groups.remove(from->displayName());
	rosterItem.setGroups(groups);

	protocol->updateContact(rosterItem);

}

void JabberContact::removeFromGroup(KopeteGroup *group)
{

	kdDebug(14130) << "[JabberContact] Removing user " << userId() << " from group " << group->displayName() << endl;

	QStringList groups = rosterItem.groups();
	groups.remove(group->displayName());
	rosterItem.setGroups(groups);

	protocol->updateContact(rosterItem);

}

void JabberContact::km2jm(const KopeteMessage &km, Jabber::Message &jm)
{
	JabberContact *to = dynamic_cast<JabberContact *>(km.to().first());
	const JabberContact *from = dynamic_cast<const JabberContact *>(km.from());

	// ugly hack, Jabber::Message does not have a setFrom() method
	Jabber::Message jabMessage(Jabber::Jid(from->userId()));

	// if a resource has been selected for this contact,
	// send to this special resource - otherwise,
	// just send to the server and let the server decide
	if (!to->resource().isNull())
		jabMessage.setTo(Jabber::Jid(QString("%1/%2").arg(to->userId(), 1).arg(to->resource(), 2)));
	else
		jabMessage.setTo(Jabber::Jid(to->userId()));

	//jabMessage.setFrom(from->userId();
	jabMessage.setBody(km.plainBody());
	jabMessage.setSubject(km.subject());

	// determine type of the widget and set message type accordingly
	KopeteMessage::MessageType type = KopeteViewManager::viewManager()->view(manager(), false)->viewType();
	if( type == KopeteMessage::Chat )
		jabMessage.setType("chat");
	else
		jabMessage.setType("normal");

	jm = jabMessage;

}

void JabberContact::slotReceivedMessage(const Jabber::Message &message)
{
	KopeteMessage::MessageType type;
	KopeteContactPtrList contactList;

	// determine message type
	if( message.type() == "chat" )
		type = KopeteMessage::Chat;
	else
		type = KopeteMessage::Email;

	contactList.append(protocol->myself());

	// convert Jabber::Message into KopeteMessage
	KopeteMessage newMessage( message.timeStamp(),
			this,
			contactList,
			message.body(),
			message.subject(),
			KopeteMessage::Inbound,
			KopeteMessage::PlainText, type );

	// add it to the manager
	manager()->appendMessage(newMessage);

}

void JabberContact::slotSendMessage(KopeteMessage &message)
{
	Jabber::Message jabberMessage;

	if(protocol->isConnected())
	{
		// convert the message
		km2jm(message, jabberMessage);

		// send it
		protocol->jabberClient->sendMessage(jabberMessage);

		// append the message to the manager
		manager()->appendMessage(message);

		// tell the manager that we sent successfully
		manager()->messageSucceeded();
	}
	else
	{
		protocol->errorConnectFirst();

		// FIXME: there is no messageFailed() yet,
		// but we need to stop the animation etc.
		manager()->messageSucceeded();
	}

}

/**
 * Determine the currently best resource for the contact
 */
JabberResource *JabberContact::bestResource()
{
	JabberResource *resource, *tmpResource;

	// iterate through all available resources
	for (resource = tmpResource = resources.first(); tmpResource; tmpResource = resources.next())
	{
		kdDebug(14130) << "[JabberContact] Processing resource " << tmpResource->resource() << endl;

		if (tmpResource->priority() > resource->priority())
		{
			kdDebug(14130) << "[JabberContact] Got better resource " << tmpResource->resource() << " through better priority." << endl;
			resource = tmpResource;
		}
		else
		{
			if (tmpResource->priority() == resource->priority())
			{
				if (tmpResource->timestamp() >= resource->timestamp())
				{
					kdDebug(14130) << "[JabberContact] Got better resource " << tmpResource->resource() << " through newer timestamp." << endl;
					resource = tmpResource;
				}
				else
				{
					kdDebug(14130) << "[JabberContact] Discarding resource " << tmpResource->resource() << " with older timestamp." << endl;
				}
			}
			else
			{
				kdDebug(14130) << "[JabberContact] Discarding resource " << tmpResource->resource() << " with worse priority." << endl;
			}
		}
	}

	return resource;

}

void JabberContact::slotResourceAvailable(const Jabber::Jid &, const Jabber::Resource &resource)
{

	kdDebug(14130) << "[JabberContact] Adding new resource '" << resource.name() << "' for "
			  << userId() << ", name [" << resource.name() << "], priority "
			  << resource.priority() << ", status [" << resource.status().status() << endl;

	/*
	 * if the new resource already exists, remove the current
	 * instance of it (Psi will emit resourceAvailable()
	 * whenever a certain resource changes status)
	 * removing the current instance will prevent double entries
	 * of the same resource.
	 * updated resource will be re-added below
	 * FIXME: this should be done using the QPtrList methods! (needs checking of pointer values)
	 */
	for (JabberResource *tmpResource = resources.first(); tmpResource; tmpResource = resources.next())
	{
		if (tmpResource->resource() == resource.name())
		{
			kdDebug(14130) << "[JabberContact] Resource " << tmpResource->resource() << " already added, removing instance with older timestamp" << endl;
			resources.remove();
		}
	}

	KopeteOnlineStatus status = JabberProtocol::protocol()->JabberOnline;
	if(resource.status().show() == "chat")
	{
		status = JabberProtocol::protocol()->JabberChatty;
	}
	else
	if(resource.status().show() == "away")
	{
		status = JabberProtocol::protocol()->JabberAway;
	}
	else
	if(resource.status().show() == "xa")
	{
		status = JabberProtocol::protocol()->JabberXA;
	}
	else
	if(resource.status().show() == "dnd")
	{
		status = JabberProtocol::protocol()->JabberDND;
	}

	JabberResource *newResource = new JabberResource(resource.name(), resource.priority(), resource.status().timeStamp(), status, resource.status().status());
	resources.append(newResource);

	JabberResource *tmpBestResource = bestResource();

	kdDebug(14130) << "[JabberContact] Best resource is now " << tmpBestResource->resource() << "." << endl;

	slotUpdatePresence(tmpBestResource->status(), tmpBestResource->reason());

	// switch active resource if override is not in effect
	if(!resourceOverride)
		activeResource = tmpBestResource;

}

void JabberContact::slotResourceUnavailable(const Jabber::Jid &jid, const Jabber::Resource &resource)
{
	JabberResource *tmpResource;

	kdDebug(14130) << "[JabberContact] Removing resource '" << jid.resource() << "' for " << userId() << endl;

	for (tmpResource = resources.first(); tmpResource; tmpResource = resources.next())
	{
		if (tmpResource->resource() == resource.name())
		{
			kdDebug(14130) << "[JabberContact] Got a match in " << tmpResource->resource() << ", removing." << endl;

			if (resources.remove())
				kdDebug(14130) << "[JabberContact] Successfully removed, there are now " << resources.count() << " resources!" << endl;
			else
				kdDebug(14130) << "[JabberContact] Ack! Couldn't remove the resource. Bugger!" << endl;

			break;
		}
	}

	JabberResource *newResource = bestResource();

	kdDebug(14130) << "[JabberContact] Best resource is now " << newResource->resource() << "." << endl;
	slotUpdatePresence(newResource->status(), newResource->reason());

	// if override was in effect or we just deleted the current
	// resource, switch to new resource
	if(resourceOverride || (activeResource->resource() == resource.name()))
	{
		resourceOverride = false;
		activeResource = newResource;
	}

}

void JabberContact::slotSelectResource()
{

	if (actionSelectResource->currentItem() == 0)
	{
		kdDebug(14130) << "[JabberContact] Removing active resource, trusting bestResource()." << endl;

		resourceOverride = false;
		activeResource = bestResource();
	}
	else
	{
		QString selectedResource = actionSelectResource->currentText();

		kdDebug(14130) << "[JabberContact] Moving to resource " << selectedResource << endl;

		resourceOverride = true;

		for (JabberResource *resource = resources.first(); resource; resource = resources.next())
		{
			if (resource->resource() == selectedResource)
			{
				kdDebug(14130) << "[JabberContact] New active resource is " << resource->resource() << endl;

				activeResource = resource;

				break;
			}
		}
	}

}


void JabberContact::slotUserInfo()
{

	if(!protocol->isConnected())
	{
		protocol->errorConnectFirst();
		return;
	}

	Jabber::JT_VCard *task = new Jabber::JT_VCard(protocol->jabberClient->rootTask());

	// signal to ourselves when the vCard data arrived
	QObject::connect(task, SIGNAL(finished()),
					this, SLOT(slotGotVCard()));

	task->get(userId());

	task->go(true);

}

void JabberContact::slotGotVCard()
{
	Jabber::JT_VCard *vCard = (Jabber::JT_VCard *) sender();

	if (!vCard->success() || vCard->vcard().isIncomplete())
	{
		// unsuccessful, or incomplete
		KMessageBox::error(qApp->mainWidget(),
						i18n("Unable to retrieve vCard for %1").arg(vCard->jid().userHost()));
		return;
	}

	kdDebug(14130) << "[JabberContact] Got vCard for user " << vCard->jid().userHost() << ", displaying." << endl;

	dlgVCard = new dlgJabberVCard( qApp->mainWidget(), "dlgJabberVCard", vCard );

	if (mEditingVCard) {
		connect(dlgVCard, SIGNAL(saveAsXML(QDomElement &)), this, SLOT(slotSaveVCard(QDomElement &)));
		dlgVCard->setReadOnly(false);
		mEditingVCard = false;
	}
	else
		connect(dlgVCard, SIGNAL(updateNickname(const QString &)), this, SLOT(slotDoRenameContact(const QString &)));

	dlgVCard->show();
	dlgVCard->raise();

}

void JabberContact::slotEditVCard()
{

	mEditingVCard = true;
	slotUserInfo();

}

void JabberContact::slotSaveVCard(QDomElement &vCardXML)
{

	if(!protocol->isConnected())
	{
		protocol->errorConnectFirst();
		return;
	}

	Jabber::JT_VCard *task = new Jabber::JT_VCard(protocol->jabberClient->rootTask());
	Jabber::VCard vCard = Jabber::VCard();

	vCard.fromXml(vCardXML);

	task->set(vCard);
	task->go(true);

}

void JabberContact::slotStatusOnline()
{

	QString id = userId();

	if(resourceOverride)
		id += activeResource->resource();

	protocol->sendPresenceToNode( JabberProtocol::protocol()->JabberOnline, id );

}

void JabberContact::slotStatusChatty()
{

	QString id = userId();

	if(resourceOverride)
		id += activeResource->resource();

	protocol->sendPresenceToNode( JabberProtocol::protocol()->JabberChatty, id );

}

void JabberContact::slotStatusAway()
{

	QString id = userId();

	if(resourceOverride)
		id += activeResource->resource();

	protocol->sendPresenceToNode(JabberProtocol::protocol()->JabberAway, id );

}

void JabberContact::slotStatusXA()
{

	QString id = userId();

	if(resourceOverride)
		id += activeResource->resource();

	protocol->sendPresenceToNode(JabberProtocol::protocol()->JabberXA, id );

}

void JabberContact::slotStatusDND()
{
	QString id = userId();

	if(resourceOverride)
		id += activeResource->resource();

	protocol->sendPresenceToNode(JabberProtocol::protocol()->JabberDND, id );

}

void JabberContact::slotStatusInvisible()
{
	QString id = userId();

	if(resourceOverride)
		id += activeResource->resource();

	protocol->sendPresenceToNode(JabberProtocol::protocol()->JabberInvisible, id );
}

void JabberContact::serialize( QMap<QString, QString> &serializedData,
	QMap<QString, QString> & /* addressBookData */ )
{
	// Contact id and display name are already set for us, only add the rest
	serializedData[ "identityId" ] = identityId();
	serializedData[ "groups" ]     = groups().toStringList().join( "," );
}

#include "jabbercontact.moc"

// vim: set noet ts=4 sts=4 sw=4:

