/*
  oscarcontact.cpp  -  Oscar Protocol Plugin

  Copyright (c) 2002 by Tom Linsky <twl6@po.cwru.edu>
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

#include "oscarcontact.h"

#include <time.h>

#include <qapplication.h>

#include <kaction.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <kdeversion.h>
#include <kinputdialog.h>

#include "kopetemessagemanagerfactory.h"
#include "kopetemetacontact.h"
#include "kopetecontactlist.h"
#include "kopetegroup.h"
#include "kopeteuiglobal.h"
#include <kopeteglobal.h>

#include "aim.h"
#include "oscarsocket.h"
#include "oscaraccount.h"

#include <assert.h>

OscarContact::OscarContact(const QString& name, const QString& displayName,
	KopeteAccount *account, KopeteMetaContact *parent)
	: KopeteContact(account, name, parent)
{
	/*kdDebug(14150) << k_funcinfo <<
		"name='" << name <<
		"', displayName='" << displayName << "'" << endl;*/

	assert(account);
	mAccount = static_cast<OscarAccount*>(account);

	mName = tocNormalize(name); // We store normalized names (lowercase no spaces)
	mEncoding=0;
	mGroupId=0;
	mMsgManager=0L;
	mIsServerSide = false;
	mIgnore = false;
	mVisibleTo = false;
	mInvisibleTo = false;

	setFileCapable(false);

	if (!displayName.isEmpty())
		setDisplayName(displayName);
	else
		setDisplayName(name);

	// fill userinfo with default values until we receive a userinfo block from the contact
	mInfo.sn = name;
	mInfo.capabilities = 0;
	mInfo.icqextstatus = ICQ_STATUS_OFFLINE;
	mInfo.idletime = 0;
	mInfo.version = 0;

	initSignals();
}


void OscarContact::initSignals()
{
//	kdDebug(14150) << k_funcinfo << "Called" << endl;
	// Buddy offline
	connect(
		mAccount->engine(), SIGNAL(gotOffgoingBuddy(QString)),
		this, SLOT(slotOffgoingBuddy(QString)));

	// kopete-users's status changed
	connect(
		mAccount->engine(), SIGNAL(statusChanged(const unsigned int)),
		this, SLOT(slotMainStatusChanged(const unsigned int)));

	/*connect(
		mAccount->engine(), SIGNAL(gotContactChange(const UserInfo &)),
		this, SLOT(slotParseUserInfo(const UserInfo &)));*/

	connect(mAccount->engine(),
		SIGNAL(gotAuthReply(const QString &, const QString &, bool)),
		this, SLOT(slotGotAuthReply(const QString &, const QString &, bool)));

	// Incoming minitype notification
	connect(mAccount->engine(),
		SIGNAL(recvMTN(const QString &, OscarConnection::TypingNotify)),
		this, SLOT(slotGotMiniType(const QString &, OscarConnection::TypingNotify)));
#if 0
	// New direct connection
	connect(
		mAccount->engine(), SIGNAL(connectionReady(QString)),
		this, SLOT(slotDirectIMReady(QString)));

	// Direct connection closed
	connect(
		mAccount->engine(), SIGNAL(directIMConnectionClosed(QString)),
		this, SLOT(slotDirectIMConnectionClosed(QString)));

	// File transfer request
	connect(
		mAccount->engine(), SIGNAL(gotFileSendRequest(QString,QString,QString,unsigned long)),
		this, SLOT(slotGotFileSendRequest(QString,QString,QString,unsigned long)));

	// File transfer started
	connect(
		mAccount->engine(), SIGNAL(transferBegun(OscarConnection *, const QString &,
			const unsigned long, const QString &)),
		this, SLOT(slotTransferBegun(OscarConnection *,
			const QString &,
			const unsigned long,
			const QString &)));

	// File transfer manager stuff
	connect(
		KopeteTransferManager::transferManager(), SIGNAL(accepted(KopeteTransfer *, const QString &)),
				this, SLOT(slotTransferAccepted(KopeteTransfer *, const QString &)) );

	// When the file transfer is refused
	connect(
		KopeteTransferManager::transferManager(), SIGNAL(refused(const KopeteFileTransferInfo &)),
		this, SLOT(slotTransferDenied(const KopeteFileTransferInfo &)));
#endif
}


OscarContact::~OscarContact()
{
}


KopeteMessageManager* OscarContact::manager(bool canCreate)
{
	if(!mMsgManager && canCreate)
	{
		/*kdDebug(14190) << k_funcinfo <<
			"Creating new MessageManager for contact '" << displayName() << "'" << endl;*/

		QPtrList<KopeteContact> theContact;
		theContact.append(this);

		mMsgManager = KopeteMessageManagerFactory::factory()->create(account()->myself(), theContact, protocol());

		// This is for when the user types a message and presses send
		connect(mMsgManager,
			SIGNAL(messageSent(KopeteMessage&, KopeteMessageManager *)),
			this, SLOT(slotSendMsg(KopeteMessage&, KopeteMessageManager *)));

		// For when the message manager is destroyed
		connect(mMsgManager, SIGNAL(destroyed()),
			this, SLOT(slotMessageManagerDestroyed()));

		connect(mMsgManager, SIGNAL(typingMsg(bool)),
			this, SLOT(slotTyping(bool)));
	}
	return mMsgManager;
}


void OscarContact::slotMessageManagerDestroyed()
{
	/*kdDebug(14190) << k_funcinfo <<
		"MessageManager for contact '" << displayName() << "' destroyed" << endl;*/
	mMsgManager = 0L;
}


void OscarContact::slotTyping(bool typing)
{
	mAccount->engine()->sendMiniTypingNotify(contactName(),
		typing ? OscarSocket::TypingBegun : OscarSocket::TypingFinished);
}


void OscarContact::slotGotMiniType(const QString &name,
	OscarConnection::TypingNotify type)
{
	// Check to see if it's us
	if(tocNormalize(name) != contactName())
		return;

	// Only if we already have a message manager
	if(mMsgManager == 0L)
		return;

	/*kdDebug(14190) << k_funcinfo <<
		"Got minitype notification for " << contactName() << endl;*/

	switch(type)
	{
		case (OscarConnection::TypingFinished):
		case (OscarConnection::TextTyped): // Both of these are types of "not typing"
			mMsgManager->receivedTypingMsg(this, false);
			break;
		case (OscarConnection::TypingBegun): // Typing started
			mMsgManager->receivedTypingMsg(this, true);
			break;
		default:
			break;
	}
}


void OscarContact::slotMainStatusChanged(const unsigned int newStatus)
{
	if(newStatus == OSCAR_OFFLINE)
	{
		setStatus(OSCAR_OFFLINE);
		mInfo.idletime = 0;
		setIdleTime(0);
		emit idleStateChanged(this);
	}
}


void OscarContact::slotOffgoingBuddy(QString sn)
{
	if(tocNormalize(sn) != contactName())
		return;

	setStatus(OSCAR_OFFLINE);
	mInfo.idletime = 0;
	setIdleTime(0);
	emit idleStateChanged(this);
}


void OscarContact::slotUpdateNickname(const QString newNickname)
{
	setDisplayName(newNickname);
}


void OscarContact::slotDeleteContact()
{
	kdDebug(14150) << k_funcinfo << "contact '" << displayName() << "'" << endl;

	QString grpName;
	if (metaContact())
	{
		if (metaContact()->groups().count() > 0)
			grpName = metaContact()->groups().first()->displayName();
	}

	if (mAccount->engine()->isICQ())
		mAccount->engine()->sendDelBuddylist(contactName());

	mAccount->engine()->sendDelBuddy(contactName(), grpName);
	deleteLater();
}


// Called when the metacontact owning this contact has changed groups
void OscarContact::syncGroups()
{
	if (!metaContact())
		return;

	kdDebug(14150) << k_funcinfo << "Called for '" << displayName() <<
		"' (" << contactId() << ")" << endl;

	// Get the (kopete) group that we belong to
	KopeteGroupList groups = metaContact()->groups();
	if(groups.count() == 0)
	{
		kdDebug(14150) << k_funcinfo <<
			"Contact is in no group in Kopete contactlist, aborting!" << endl;
		return;
	}

	//Don't modify the group if we're moving the contact to the top-level
	//or the temporary group. This modifies our local list, but doesn't change
	//the server.
	if (groups.contains(KopeteGroup::topLevel()) || groups.contains(KopeteGroup::temporary()))
		return;

	// Oscar only supports one group per contact, so just get the first one
	KopeteGroup *firstKopeteGroup = groups.first();

	if(!firstKopeteGroup)
	{
		kdDebug(14150) << k_funcinfo << "Could not get first kopete group" << endl;
		return;
	}

	/*kdDebug(14150) << k_funcinfo << "SSI Data before change of group" << endl;
	mAccount->engine()->ssiData().print();*/

	if (!mAccount->engine()->ssiData().findGroup(firstKopeteGroup->displayName()))
	{
		//We don't have the group in SSI yet. Add it.
		kdDebug(14150) << k_funcinfo << "Adding missing group '" <<
			firstKopeteGroup->displayName() << "'" << endl;
		mAccount->engine()->sendAddGroup(firstKopeteGroup->displayName());
	}
	else
	{
		kdDebug(14150) << k_funcinfo <<
			"New group '" << firstKopeteGroup->displayName() <<
			" 'found in SSI already" << endl;
	}

	/*
	 * Another possibility is moving a buddy in the blm, but in that case, we don't need
	 * to move him on BLM or SSI or anywhere, since BLM doesn't keep track of groups.
	 *
	 * Due to a bug in libkopete, temporary contacts don't have syncGroups called on them
	 */
	SSI* movedItem = mAccount->engine()->ssiData().findContact(contactId());
	if (movedItem)
	{
		//hey, contact is on SSI, move him
		SSI* oldGroup = mAccount->engine()->ssiData().findGroup(movedItem->gid);
		//I'm not checking the old group pointer because since we're using
		//the gid, the group is guaranteed to be found.
		mAccount->engine()->sendChangeBuddyGroup(movedItem->name, oldGroup->name,
			firstKopeteGroup->displayName());
	}
	else
	{
		kdDebug(14150) << k_funcinfo <<
			"Contact not on server. Adding it." << endl;
		mAccount->engine()->sendAddBuddy(contactId(), firstKopeteGroup->displayName(), false);
	}

	/*kdDebug(14150) << k_funcinfo << "SSI Data after change of group" << endl;
	mAccount->engine()->ssiData().print();*/
}


#if 0
void OscarContact::slotGotFileSendRequest(QString sn, QString message, QString filename,
	unsigned long filesize)
{
	if(tocNormalize(sn) != mName)
		return;

	kdDebug(14150) << k_funcinfo << "Got file transfer request for '" <<
		displayName() << "'" << endl;

	KopeteTransferManager::transferManager()->askIncomingTransfer(
		this, filename, filesize, message);
}

void OscarContact::slotTransferAccepted(KopeteTransfer *tr, const QString &fileName)
{
	if (tr->info().contact() != this)
		return;

	kdDebug(14150) << k_funcinfo << "Transfer of '" << fileName <<
		"' from '" << mName << "' accepted." << endl;

	OscarConnection *fs = mAccount->engine()->sendFileSendAccept(mName, fileName);

	//connect to transfer manager
	connect(
		fs, SIGNAL(percentComplete(unsigned int)),
		tr, SLOT(slotPercentCompleted(unsigned int)));
}

void OscarContact::slotTransferDenied(const KopeteFileTransferInfo &tr)
{
	// Check if we're the one who is directly connected
	if(tr.contact() != this)
		return;

	kdDebug(14150) << k_funcinfo << "Transfer denied." << endl;
	mAccount->engine()->sendFileSendDeny(mName);
}

void OscarContact::slotTransferBegun(OscarConnection *con,
	const QString& file,
	const unsigned long size,
	const QString &recipient)
{
	if (tocNormalize(con->connectionName()) != mName)
		return;

	kdDebug(14150) << k_funcinfo << "adding transfer of " << file << endl;
	KopeteTransfer *tr = KopeteTransferManager::transferManager()->addTransfer(
		this, file, size, recipient, KopeteFileTransferInfo::Outgoing );

	//connect to transfer manager
	connect(
		con, SIGNAL(percentComplete(unsigned int)),
		tr, SLOT(slotPercentCompleted(unsigned int)));
}

void OscarContact::slotDirectConnect()
{
	kdDebug(14150) << k_funcinfo << "Requesting direct IM with " << mName << endl;

	int result = KMessageBox::questionYesNo(
		Kopete::UI::Global::mainWidget(),
		i18n("<qt>Are you sure you want to establish a direct connection to %1? \
		This will allow %2 to know your IP address, which can be dangerous if \
		you do not trust this contact.</qt>")
#if QT_VERSION < 0x030200
			.arg(mName).arg(mName),
#else
			.arg(mName , mName),
#endif
		i18n("Request Direct IM with %1?").arg(mName));
	if(result == KMessageBox::Yes)
	{
		execute();
		KopeteContactPtrList p;
		p.append(this);
		KopeteMessage msg = KopeteMessage(
			this, p,
			i18n("Waiting for %1 to connect...").arg(mName),
			KopeteMessage::Internal, KopeteMessage::PlainText );

		manager(true)->appendMessage(msg);
		mAccount->engine()->sendDirectIMRequest(mName);
	}
}

void OscarContact::slotDirectIMReady(QString name)
{
	// Check if we're the one who is directly connected
	if(tocNormalize(name) != contactName())
		return;

	kdDebug(14150) << k_funcinfo << "Setting direct connect state for '" <<
		displayName() << "' to true" << endl;

	mDirectlyConnected = true;
	KopeteContactPtrList p;
	p.append(this);
	KopeteMessage msg = KopeteMessage(
		this, p,
		i18n("Direct connection to %1 established").arg(mName),
		KopeteMessage::Internal, KopeteMessage::PlainText ) ;

	manager(true)->appendMessage(msg);
}

void OscarContact::slotDirectIMConnectionClosed(QString name)
{
	// Check if we're the one who is directly connected
	if ( tocNormalize(name) != tocNormalize(mName) )
		return;

	kdDebug(14150) << "[OscarContact] Setting direct connect state for '"
		<< mName << "' to false." << endl;

	mDirectlyConnected = false;
}

void OscarContact::sendFile(const KURL &sourceURL, const QString &/*altFileName*/,
	const long unsigned int /*fileSize*/)
{
	KURL filePath;

	//If the file location is null, then get it from a file open dialog
	if( !sourceURL.isValid() )
		filePath = KFileDialog::getOpenURL(QString::null ,"*", 0L, i18n("Kopete File Transfer"));
	else
		filePath = sourceURL;

	if(!filePath.isEmpty())
	{
		KFileItem finfo(KFileItem::Unknown, KFileItem::Unknown, filePath);

		kdDebug(14150) << k_funcinfo << "File size is " <<
			(unsigned long)finfo.size() << endl;

		//Send the file
		mAccount->engine()->sendFileSendRequest(mName, finfo);
	}
}
#endif


void OscarContact::rename(const QString &newNick)
{
	kdDebug(14150) << k_funcinfo << "Rename '" << displayName() << "' to '" <<
		newNick << "'" << endl;

	//TODO: group handling!
	setDisplayName(newNick);
}


void OscarContact::slotParseUserInfo(const UserInfo &u)
{
	if(tocNormalize(u.sn) != contactName())
		return;

	if(mInfo.idletime != u.idletime)
	{
		setIdleTime(u.idletime * 60);
		if(u.idletime == 0)
			emit idleStateChanged(this);
	}

	// Overwrites the onlineSince property set by KopeteContact, but that's ok :)
	if(u.onlinesince.isValid())
	{
		//kdDebug(14150) << k_funcinfo << "onlinesince = " << u.onlinesince.toString() << endl;
		setProperty(Kopete::Global::Properties::self()->onlineSince(), u.onlinesince);
	}
	else
	{
		//kdDebug(14150) << k_funcinfo << "invalid onlinesince, removing property!" << endl;
		removeProperty(Kopete::Global::Properties::self()->onlineSince());
	}

	//kdDebug(14150) << k_funcinfo << "Called for '" << displayName() << "'" << endl;
	mInfo.updateInfo(u); // merge data from mInfo and u
}

bool OscarContact::hasCap(int capNumber)
{
	return (mInfo.hasCap(capNumber));
}

void OscarContact::slotRequestAuth()
{
	kdDebug(14150) << k_funcinfo << "Called for '" << displayName() << "'" << endl;
	requestAuth();
}


int OscarContact::requestAuth()
{
	QString reason = KInputDialog::getText(
		i18n("Request Authorization"),i18n("Reason for requesting authorization:"));
	if(!reason.isNull())
	{
		kdDebug(14150) << k_funcinfo << "Sending auth request to '" <<
			displayName() << "'" << endl;
		mAccount->engine()->sendAuthRequest(contactName(), reason);
		return(1);
	}
	else
		return(0);
}


void OscarContact::slotSendAuth()
{
	kdDebug(14150) << k_funcinfo << "Called for '" << displayName() << "'" << endl;

	// TODO: custom dialog also allowing a refusal
	QString reason = KInputDialog::getText(
		i18n("Request Authorization"),i18n("Reason for granting authorization:"));
	if(!reason.isNull())
	{
		kdDebug(14150) << k_funcinfo << "Sending auth granted to '" <<
			displayName() << "'" << endl;
		mAccount->engine()->sendAuthReply(contactName(), reason, true);
	}
}


void OscarContact::slotGotAuthReply(const QString &contact, const QString &reason, bool granted)
{
	if(contact != contactName())
		return;

	kdDebug(14150) << k_funcinfo << "Called for '" << displayName() << "' reason='" <<
		reason << "' granted=" << granted << endl;

// TODO: Reimplement
//	setWaitAuth(granted);

/*	if (!waitAuth())
		mAccount->engine()->sendDelBuddylist(tocNormalize(contact));*/

	// FIXME: remove this method and handle auth in oscaraccount already!
	/*
	if(granted)
	{
		QString message = i18n("<b>[Granted Authorization:]</b> %1").arg(reason);
		gotIM(OscarSocket::GrantedAuth, message);
	}
	else
	{
		QString message = i18n("<b>[Declined Authorization:]</b> %1").arg(reason);
		gotIM(OscarSocket::DeclinedAuth, message);
	}
	*/
}

//void OscarContact::receivedIM(OscarSocket::OscarMessageType type, const OscarMessage &msg)
void OscarContact::receivedIM(KopeteMessage &msg)
{
	//kdDebug(14190) << k_funcinfo << "called" << endl;
	// Tell the message manager that the buddy is done typing
	manager(true)->receivedTypingMsg(this, false);

/*
	// Build a KopeteMessage and set the body as Rich Text
	KopeteContactPtrList tmpList;
	tmpList.append(account()->myself());
	KopeteMessage kmsg(this, tmpList, msg.text, KopeteMessage::Inbound,
		KopeteMessage::RichText);
	manager(true)->appendMessage(kmsg);
*/
	manager(true)->appendMessage(msg);

#if 0
	// send our away message in fire-and-forget-mode :)
	if(mAccount->isAway())
	{
		// Compare current time to last time we sent a message
		// We'll wait 2 minutes between responses
		if((time(0L) - mLastAutoResponseTime) > 120)
		{
			kdDebug(14190) << k_funcinfo << " while we are away, " \
				"sending away-message to annoy buddy :)" << endl;
			// Send the autoresponse
			mAccount->engine()->sendIM(KopeteAway::getInstance()->message(), this, true);
			// Build a pointerlist to insert this contact into
			KopeteContactPtrList toContact;
			toContact.append(this);
			// Display the autoresponse
			// Make it look different
			// UGLY hardcoded color
			QString responseDisplay =
				"<font color='#666699'>Autoresponse: </font>" +
				KopeteAway::getInstance()->message();

			KopeteMessage message(mAccount->myself(), toContact,
				responseDisplay, KopeteMessage::Outbound, KopeteMessage::RichText);

			manager(true)->appendMessage(message);
			// Set the time we last sent an autoresponse
			// which is right now
			mLastAutoResponseTime = time(0L);
		}
	}
#endif
}

#if 0
bool OscarContact::waitAuth() const
{
	// TODO: move var to OscarContact
	return mListContact->waitAuth();
}

void OscarContact::setWaitAuth(bool b) const
{
	mListContact->setWaitAuth(b);
}

#endif

const unsigned int OscarContact::encoding()
{
	return mEncoding;
}

void OscarContact::setEncoding(const unsigned int mib)
{
	mEncoding = mib;
}

const int OscarContact::groupId()
{
	//kdDebug(14150) << k_funcinfo << "returning" << mGroupId << endl;
	return mGroupId;
}

void OscarContact::setGroupId(const int newgid)
{
	if(newgid > 0)
	{
		mGroupId = newgid;
		//kdDebug(14150) << k_funcinfo << "updated group id to " << mGroupId << endl;
	}
}

void OscarContact::serialize(QMap<QString, QString> &serializedData,
	QMap<QString, QString> &/*addressBookData*/)
{
//	serializedData["awaitingAuth"] = waitAuth() ? "1" : "0";
	serializedData["Encoding"] = QString::number(mEncoding);
	serializedData["groupID"] = QString::number(mGroupId);
}

void OscarContact::setIgnore(bool val, bool updateServer)
{
	mIgnore = val;
	if (updateServer)
	{
		if (val)
			mAccount->engine()->sendSSIAddIgnore(mName);
		else
			mAccount->engine()->sendSSIRemoveIgnore(mName);
	}
}

void OscarContact::setVisibleTo(bool val, bool updateServer)
{
	mVisibleTo = val;
	if (updateServer)
	{
		if (val)
			mAccount->engine()->sendSSIAddVisible(mName);
		else
			mAccount->engine()->sendSSIRemoveVisible(mName);
	}
}

void OscarContact::setInvisibleTo(bool val, bool updateServer)
{
	mInvisibleTo = val;
	if (updateServer)
	{
		if (val)
			mAccount->engine()->sendSSIAddInvisible(mName);
		else
			mAccount->engine()->sendSSIRemoveInvisible(mName);
	}
}

void OscarContact::slotInvisibleTo()
{
	kdDebug(14150) << k_funcinfo <<
		"Called; invisible = " << actionInvisibleTo->isChecked() << endl;
	setInvisibleTo(actionInvisibleTo->isChecked(), true);
}

#include "oscarcontact.moc"
// vim: set noet ts=4 sts=4 sw=4:
