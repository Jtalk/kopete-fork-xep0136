/*
    yahooaccount.cpp - Manages a single Yahoo account

    Copyright (c) 2003 by Gav Wood               <gav@kde.org>
    Copyright (c) 2003 by Matt Rogers            <mattrogers@sbcglobal.net>
    Based on code by Olivier Goffart             <ogoffart@tiscalinet.be>
    Kopete    (c) 2003 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/
//Standard Header
#include <ctime>

//QT
#include <qfont.h>
#include <qdatetime.h>
#include <qcolor.h>

// KDE
#include <kconfig.h>
#include <kdebug.h>
#include <kaction.h>
#include <kpopupmenu.h>
#include <kmessagebox.h>
#include <kapplication.h>

// Kopete
#include <kopetemessagemanager.h>
#include <kopetemessage.h>

// Yahoo
#include "yahooaccount.h"
#include "yahoocontact.h"

YahooAwayDialog::YahooAwayDialog(YahooAccount* account, QWidget *parent, const char *name) :
	KopeteAwayDialog(parent, name)
{
	theAccount = account;
}

void YahooAwayDialog::setAway(int awayType)
{
	theAccount->setAway(awayType, getSelectedAwayMessage());
}


YahooAccount::YahooAccount(YahooProtocol *parent, const QString& AccountID, const char *name)
: KopeteAccount(parent, AccountID, name)
{
//	kdDebug(14180) << k_funcinfo << AccountID << ", " << QString(name) << ")" << endl;

	// first things first - initialise internals
	theHaveContactList = false;
	stateOnConnection = 0;
	theAwayDialog = new YahooAwayDialog(this);
	m_needNewPassword = false;
	
	// we need this quite early (before initActions at least)
	kdDebug(14180) << "Yahoo: Creating myself with name = " << accountId() << endl;
	setMyself( new YahooContact(this, accountId(), accountId(), 0) );
	static_cast<YahooContact *>( myself() )->setYahooStatus(YahooStatus::Offline);

	QObject::connect(this, SIGNAL(needReconnect()), this, SLOT(slotNeedReconnect()));
	
	if(autoLogin()) connect();
}

YahooAccount::~YahooAccount()
{
  delete theAwayDialog;
}

void YahooAccount::slotGoStatus(int status, const QString &awayMessage)
{
	kdDebug(14180) << k_funcinfo << status << awayMessage << endl;

	if(!isConnected())
	{	if(status == 12)
			// TODO: must connect as invisible
			connect();
		else
			connect();
		stateOnConnection = status;
	}
	else
	{
		m_session->setAway(yahoo_status(status), awayMessage, status ? 1 : 0);
		static_cast<YahooContact *>( myself() )->setYahooStatus(YahooStatus::fromLibYahoo2(status));
	}
}

void YahooAccount::loaded()
{
	kdDebug(14180) << k_funcinfo << endl;
	QString newPluginData;

	newPluginData = pluginData(protocol(), QString::fromLatin1("displayName"));
	if(!newPluginData.isEmpty())
		myself()->rename(newPluginData);	//TODO: might cause problems depending on rename semantics

}

YahooSession *YahooAccount::yahooSession()
{
	return m_session ? m_session : 0L;
}

void YahooAccount::setUseServerGroups(bool newSetting)
{
	m_useServerGroups = newSetting;
}

void YahooAccount::setImportContacts(bool newSetting)
{
	m_importContacts = newSetting;
}

QColor YahooAccount::getMsgColor(const QString& msg)
{
	/* Yahoo sends a message either with color or without color
	 * so we have to use this really hacky method to get colors
	 */
	kdDebug(14180) << k_funcinfo << "msg is " << msg << endl;
	//If we get here, the message uses a standard Yahoo color
	//(selectable from the drop down box)
	//Please note that some of the colors are hard-coded to
	//match the yahoo colors
	if ( msg.find("[38m") != -1 )
		return Qt::red;
	if ( msg.find("[34m") != -1 )
		return Qt::green;
	if ( msg.find("[31m") != -1 )
		return Qt::blue;
	if ( msg.find("[39m") != -1 )
		return Qt::yellow;
	if ( msg.find("[36m") != -1 )
		return Qt::darkMagenta;
	if ( msg.find("[32m") != -1 )
		return Qt::cyan;
	if ( msg.find("[37m") != -1 )
		return QColor("#FFAA39");
	if ( msg.find("[35m") != -1 )
		return QColor("#FFD8D8");
	if ( msg.find("[#") != -1 )
	{
		kdDebug(14180) << "Custom color is " << msg.mid(msg.find("[#")+1,7) << endl;
		return QColor(msg.mid(msg.find("[#")+1,7));
	}
	
	//return a default value just in case
	return Qt::black;
}

void YahooAccount::connect()
{
	QString server = "scs.msg.yahoo.com";
	int port = 5050;

	/* call loaded() here. It shouldn't hurt anything
	 * and ensures that all our settings get loaded
	 */
	loaded();
	
	YahooSessionManager::manager()->setPager(server, port);
	
	
		
	if ((password(m_needNewPassword)).isNull())
	{ //cancel the connection attempt
		static_cast<YahooContact*>(myself())->setYahooStatus(YahooStatus::Offline);
		return;
	}
	
	m_session = YahooSessionManager::manager()->createSession(accountId(), password());

	if(!isConnected())
	{
		kdDebug(14180) << "Attempting to connect to Yahoo on <" << server << ":" << port << ">. user <" << accountId() << ">" << endl;

		if(m_session)
		{
			if( m_session->sessionId() > 0)
			{

				/* We have a session, time to connect its signals to our plugin slots */
				QObject::connect(m_session, SIGNAL(loginResponse(int, const QString &)),
						this, SLOT(slotLoginResponse(int, const QString &)) );
				QObject::connect(m_session, SIGNAL(gotBuddy(const QString &, const QString &, const QString &)),
						this, SLOT(slotGotBuddy(const QString &, const QString &, const QString &)));
				/*QObject::connect( m_session , SIGNAL(gotIgnore( const QStringList & )), this , SLOT(void s
					slotGotIgnore( const QStringList & )) );*/
				QObject::connect(m_session, SIGNAL(statusChanged(const QString&, int, const QString&, int)),
						this, SLOT(slotStatusChanged(const QString&, int, const QString&, int)));
				QObject::connect(m_session, SIGNAL(gotIm(const QString&, const QString&, long, int)),
						this, SLOT(slotGotIm(const QString &, const QString&, long, int)));
				QObject::connect(m_session, SIGNAL(gotConfInvite( const QString&, const QString&, const QString&, 
						const QStringList&)), this, 
						SLOT(slotGotConfInvite(const QString&, const QString&, const QString&, const QStringList&)));
				QObject::connect(m_session, SIGNAL(confUserDecline(const QString&, const QString &, const QString &)), this, 
						SLOT(slotConfUserDecline( const QString &, const QString &, const QString &)) );
				QObject::connect(m_session , SIGNAL(confUserJoin( const QString &, const QString &)), this, 
						SLOT(slotConfUserJoin( const QString &, const QString &)) );
				QObject::connect(m_session , SIGNAL(confUserLeave( const QString &, const QString &)), this, 	
						SLOT(slotConfUserLeave( const QString &, const QString &)) );
				QObject::connect(m_session , SIGNAL(confMessage( const QString &, const QString &, const QString &)), this, 
						SLOT(slotConfMessage( const QString &, const QString &, const QString &)) );
				QObject::connect(m_session,
						SIGNAL(gotFile(const QString &, const QString &, long, const QString &, const QString &, unsigned long)),
						this,
						SLOT(slotGotFile(const QString&, const QString&, long, const QString&, const QString&, unsigned long)));
				QObject::connect(m_session , SIGNAL(contactAdded(const QString &, const QString &, const QString &)), this, 
						SLOT(slotContactAdded(const QString &, const QString &, const QString &)));
				QObject::connect(m_session , SIGNAL(rejected(const QString &, const QString &)), this,
						SLOT(slotRejected(const QString&, const QString&)));
				QObject::connect(m_session, SIGNAL(typingNotify(const QString &, int)), this , 
						SLOT(slotTypingNotify(const QString &, int)));
				QObject::connect(m_session, SIGNAL(gameNotify(const QString &, int)), this,
						SLOT(slotGameNotify( const QString &, int)));
				QObject::connect(m_session, SIGNAL(mailNotify(const QString&, const QString&, int)), this,
						SLOT(slotMailNotify(const QString &, const QString&, int)));
				QObject::connect(m_session, SIGNAL(systemMessage(const QString&)), this,
						SLOT(slotSystemMessage(const QString &)));
				QObject::connect(m_session, SIGNAL(error(const QString&, int)), this, 
						SLOT(slotError(const QString &, int )));
				QObject::connect(m_session, SIGNAL(gotIdentities(const QStringList &)), this,
						SLOT(slotGotIdentities( const QStringList&)));

				kdDebug(14180) << "We appear to have connected on session: " << m_session << endl;
				
				kdDebug(14180) << "Starting the login connection" << endl;
				m_session->login(YAHOO_STATUS_AVAILABLE);

			}
			else
			{
				kdDebug(14180) << "Couldn't connect!" << endl;
				// TODO: message box saying can't connect?
			}
		}
	}
	else if(isAway())
	{	// They're really away, and they want to un-away.
		slotGoOnline();
	}
	else	// ignore
		kdDebug(14180) << "Yahoo plugin: Ignoring connect request (already connected)." <<endl;
}

void YahooAccount::disconnect()
{
//	kdDebug(14180) << k_funcinfo << endl;

	if(isConnected())
	{	kdDebug(14180) <<  "Attempting to disconnect from Yahoo server " << endl;
		m_session->logOff();
		static_cast<YahooContact *>( myself() )->setYahooStatus(YahooStatus::Offline);
		for(QDictIterator<KopeteContact> i(contacts()); i.current(); ++i)
			static_cast<YahooContact *>(i.current())->setYahooStatus(YahooStatus::Offline);
	}
	else
	{ 	//make sure we set everybody else offline explicitly, just for cleanup
		kdDebug(14180) << "Ignoring disconnect request (not connected)." << endl;
		for(QDictIterator<KopeteContact> i(contacts()); i.current(); ++i)
			static_cast<YahooContact *>(i.current())->setYahooStatus(YahooStatus::Offline);
	}
}

void YahooAccount::setAway(bool status, const QString &awayMessage)
{
	kdDebug(14180) << "YahooAccount::setAway(" << status << ", " << awayMessage << ")" << endl;
	if(awayMessage.isEmpty())
		slotGoStatus(status ? 2 : 0);
	else
		slotGoStatus(status ? 99 : 0, awayMessage);
}

void YahooAccount::slotConnected()
{
	kdDebug(14180) << k_funcinfo << "Moved to slotLoginResponse for the moment" << endl;

}

void YahooAccount::slotGoOnline()
{
	if(!isConnected())
		connect();
	else
		slotGoStatus(0);
}

void YahooAccount::slotGoOffline()
{
	if(isConnected())
		disconnect();
	else
		static_cast<YahooContact *>( myself() )->setYahooStatus(YahooStatus::Offline);
}

KActionMenu *YahooAccount::actionMenu()
{
//	kdDebug(14180) << k_funcinfo << endl;

	KActionMenu *theActionMenu = new KActionMenu(myself()->displayName(), myself()->onlineStatus().iconFor(this) ,this);
	theActionMenu->popupMenu()->insertTitle(myself()->icon(), "Yahoo ("+myself()->displayName()+")");

	theActionMenu->insert(new KAction(i18n(YSTAvailable), YahooStatus(YahooStatus::Available).ys2kos().iconFor(this), 0, this, SLOT(slotGoOnline()), this, "actionYahooGoOnline"));
	theActionMenu->insert(new KAction(i18n(YSTBeRightBack), YahooStatus(YahooStatus::BeRightBack).ys2kos().iconFor(this), 0, this, SLOT(slotGoStatus001()), this, "actionYahooGoStatus001"));
	theActionMenu->insert(new KAction(i18n(YSTBusy), YahooStatus(YahooStatus::Busy).ys2kos().iconFor(this), 0, this, SLOT(slotGoStatus002()), this, "actionYahooGoStatus002"));
	theActionMenu->insert(new KAction(i18n(YSTNotAtHome), YahooStatus(YahooStatus::NotAtHome).ys2kos().iconFor(this), 0, this, SLOT(slotGoStatus003()), this, "actionYahooGoStatus003"));
	theActionMenu->insert(new KAction(i18n(YSTNotAtMyDesk), YahooStatus(YahooStatus::NotAtMyDesk).ys2kos().iconFor(this), 0, this, SLOT(slotGoStatus004()), this, "actionYahooGoStatus004"));
	theActionMenu->insert(new KAction(i18n(YSTNotInTheOffice), YahooStatus(YahooStatus::NotInTheOffice).ys2kos().iconFor(this), 0, this, SLOT(slotGoStatus005()), this, "actionYahooGoStatus005"));
	theActionMenu->insert(new KAction(i18n(YSTOnThePhone), YahooStatus(YahooStatus::OnThePhone).ys2kos().iconFor(this), 0, this, SLOT(slotGoStatus006()), this, "actionYahooGoStatus006"));
	theActionMenu->insert(new KAction(i18n(YSTOnVacation), YahooStatus(YahooStatus::OnVacation).ys2kos().iconFor(this), 0, this, SLOT(slotGoStatus007()), this, "actionYahooGoStatus007"));
	theActionMenu->insert(new KAction(i18n(YSTOutToLunch), YahooStatus(YahooStatus::OutToLunch).ys2kos().iconFor(this), 0, this, SLOT(slotGoStatus008()), this, "actionYahooGoStatus008"));
	theActionMenu->insert(new KAction(i18n(YSTSteppedOut), YahooStatus(YahooStatus::SteppedOut).ys2kos().iconFor(this), 0, this, SLOT(slotGoStatus009()), this, "actionYahooGoStatus009"));
	theActionMenu->insert(new KAction(i18n(YSTInvisible), YahooStatus(YahooStatus::Invisible).ys2kos().iconFor(this), 0, this, SLOT(slotGoStatus012()), this, "actionYahooGoStatus012"));
	theActionMenu->insert(new KAction(i18n("Custom"), YahooStatus(YahooStatus::Custom).ys2kos().iconFor(this), 0, this, SLOT(slotGoStatus099()), this, "actionYahooGoStatus099"));
	theActionMenu->insert(new KAction(i18n(YSTOffline), YahooStatus(YahooStatus::Offline).ys2kos().iconFor(this), 0, this, SLOT(slotGoOffline()), this, "actionYahooGoOffline"));

	return theActionMenu;
}

void YahooAccount::slotGotBuddies( const YList */*theList*/ )
{
/* Do Nothing
//	kdDebug(14180) << k_funcinfo << endl;
	theHaveContactList = true;

	// Serverside -> local
	for(QMap<QString, QPair<QString, QString> >::iterator i = IDs.begin(); i != IDs.end(); i++)
		if(!contacts()[i.key()] && m_importContacts)
		{	kdDebug(14180) << "SS Contact " << i.key() << " is not in the contact list. Adding..." << endl;
			QString groupName = m_useServerGroups ? i.data().first : QString("Imported Yahoo Contacts");
			addContact(i.key(), i.data().second == "" || i.data().second.isNull() ? i.key() : i.data().second, 0, KopeteAccount::ChangeKABC, groupName);
		}

	// Local -> serverside
	for(QDictIterator<KopeteContact> i(contacts()); i.current(); ++i)
		if(i.currentKey() != accountId())
			static_cast<YahooContact *>(i.current())->syncToServer();
*/
}

YahooContact *YahooAccount::contact( const QString &id )
{
	return static_cast<YahooContact *>(contacts()[id]);
}

bool YahooAccount::addContactToMetaContact(const QString &contactId, const QString &displayName, KopeteMetaContact *parentContact )
{
	kdDebug(14180) << k_funcinfo << " contactId: " << contactId << endl;

	if(!contact(contactId))
	{
		// FIXME: New Contacts are NOT added to KABC, because:
		// How on earth do you tell if a contact is being deserialised or added brand new here?
		YahooContact *newContact = new YahooContact( this, contactId, displayName, parentContact );
		return newContact != 0;
	}
	else
		kdDebug(14180) << k_funcinfo << "Contact already exists" << endl;

	return false;
}


void YahooAccount::slotNeedReconnect()
{
	m_needNewPassword = true;
	connect();
}


/***************************************************************************
 *                                                                         *
 *   Slot for KYahoo signals                                               *
 *                                                                         *
 ***************************************************************************/

void YahooAccount::slotLoginResponse( int succ , const QString &url )
{
	kdDebug(14180) << k_funcinfo << succ << ", " << url << ")]" << endl;
	QString errorMsg;
	if (succ == YAHOO_LOGIN_OK)
	{
		slotGotBuddies(yahooSession()->getLegacyBuddyList());
		/**
		 * FIXME: Right now, we only support connecting as online
		 * Support needs to be added for invisible
		 */
		static_cast<YahooContact *>( myself() )->setYahooStatus(YahooStatus::Available);
		m_needNewPassword = false;	
		return;
	}
	else if(succ == YAHOO_LOGIN_PASSWD)
	{
		static_cast<YahooContact *>( myself() )->setYahooStatus(YahooStatus::Offline);
		emit needReconnect();
		return;
	}
	else if(succ == YAHOO_LOGIN_LOCK)
	{
		errorMsg = i18n("Could not log into Yahoo service.  Your account has been locked.\nVisit %1 to reactivate it.").arg(url);
		KMessageBox::queuedMessageBox(kapp->mainWidget(), KMessageBox::Error, errorMsg);
		static_cast<YahooContact *>( myself() )->setYahooStatus(YahooStatus::Offline);
		return;
	}
	else if(succ == YAHOO_LOGIN_DUPL)
	{
		errorMsg = i18n("You have been logged out of the yahoo service, possibly due to a duplicate login.");
		KMessageBox::queuedMessageBox(kapp->mainWidget(), KMessageBox::Error, errorMsg);
		static_cast<YahooContact *>( myself() )->setYahooStatus(YahooStatus::Offline);
		return;
	}
	else
	{ //we disconnected for some unknown reason, change the icon
		static_cast<YahooContact *>( myself() )->setYahooStatus(YahooStatus::Offline);
	}
}

void YahooAccount::slotGotBuddy( const QString &userid, const QString &alias, const QString &group )
{
	kdDebug(14180) << k_funcinfo << endl;
	IDs[userid] = QPair<QString, QString>(group, alias);

	// Serverside -> local
	kdDebug(14180) << "SS Contact " << userid << " is not in the contact list. Adding..." << endl;
	QString groupName = group;
	addContact(userid, alias.isEmpty() ? userid : alias, 0, KopeteAccount::ChangeKABC, group);
	
}

void YahooAccount::slotGotIgnore( const QStringList & /* igns */ )
{
	//kdDebug(14180) << k_funcinfo << endl;
}

void YahooAccount::slotGotIdentities( const QStringList & /* ids */ )
{
	//kdDebug(14180) << k_funcinfo << endl;
}

void YahooAccount::slotStatusChanged( const QString &who, int stat, const QString &msg, int away)
{
	kdDebug(14180) << k_funcinfo << who << ", " << stat << ", " << msg << ", " << away << "]" << endl;
	if(contact(who))
		contact(who)->setYahooStatus( YahooStatus::fromLibYahoo2(stat), msg, away);
}

void YahooAccount::slotGotIm( const QString &who, const QString &msg, long tm, int /*stat*/)
{
	QFont msgFont;
	QDateTime msgDT;
	KopeteContactPtrList justMe;

	if(!contact(who))
		addContact( who, who, 0L, KopeteAccount::DontChangeKABC, QString::null, true );

	KopeteMessageManager *mm = contact(who)->manager();

	// Tell the message manager that the buddy is done typing
	mm->receivedTypingMsg(contact(who), false);

	justMe.append(myself());

	if (tm == 0)
		msgDT.setTime_t(time(0L));
	else
		msgDT.setTime_t(tm, Qt::LocalTime);
	
	KopeteMessage kmsg(msgDT, contact(who), justMe, msg,
					KopeteMessage::Inbound , KopeteMessage::PlainText);

	QString newMsg = kmsg.plainBody();
	
	kmsg.setFg(getMsgColor(msg));
//	kdDebug(14180) << "Message color is " << getMsgColor(msg) << endl;
	
	if (newMsg.find("<font") != -1)
	{
		msgFont.setFamily(newMsg.section('"', 1,1));

		if (newMsg.find("size"))
			msgFont.setPointSize(newMsg.section('"', 3,3).toInt());

		//remove the font encoding since we handle them ourselves
		newMsg.remove(newMsg.mid(0, newMsg.find('>')+1));
	}
	//set the new body that has correct HTML
	kmsg.setBody(newMsg, KopeteMessage::RichText);
	kmsg.setFont(msgFont);
	mm->appendMessage(kmsg);

}

void YahooAccount::slotGotConfInvite( const QString & /* who */, const QString & /* room */, const QString & /* msg */, const QStringList & /* members */ )
{
//	kdDebug(14180) << k_funcinfo << endl;
}

void YahooAccount::slotConfUserDecline( const QString & /* who */, const QString & /* room */, const QString & /* msg */ )
{
//	kdDebug(14180) << k_funcinfo << endl;
}

void YahooAccount::slotConfUserJoin( const QString & /* who */, const QString & /* room */ )
{
//	kdDebug(14180) << k_funcinfo << endl;
}

void YahooAccount::slotConfUserLeave( const QString & /* who */, const QString & /* room */ )
{
//	kdDebug(14180) << k_funcinfo << endl;
}

void YahooAccount::slotConfMessage( const QString & /* who */, const QString & /* room */, const QString & /* msg */ )
{
//	kdDebug(14180) << k_funcinfo << endl;
}

void YahooAccount::slotGotFile( const QString & /* who */, const QString & /* url */, long /* expires */, const QString & /* msg */,
	const QString & /* fname */, unsigned long /* fesize */ )
{
//	kdDebug(14180) << k_funcinfo << endl;
}

void YahooAccount::slotContactAdded( const QString & /* myid */, const QString & /* who */, const QString & /* msg */ )
{
//	kdDebug(14180) << k_funcinfo << myid << " " << who << " " << msg << endl;
}

void YahooAccount::slotRejected( const QString & /* who */, const QString & /* msg */ )
{
//	kdDebug(14180) << k_funcinfo << endl;
}

void YahooAccount::slotTypingNotify( const QString &who, int what )
{
	emit receivedTypingMsg(who, what);
}

void YahooAccount::slotGameNotify( const QString & /* who */, int /* stat */ )
{
//	kdDebug(14180) << k_funcinfo << endl;
}

void YahooAccount::slotMailNotify( const QString & /* from */, const QString & /* subj */, int /* cnt */)
{
//	kdDebug(14180) << k_funcinfo << endl;
}

void YahooAccount::slotSystemMessage( const QString & /* msg */ )
{
//	kdDebug(14180) << k_funcinfo << msg << endl;
}

void YahooAccount::slotError( const QString & /* err */, int /* fatal */ )
{
//	kdDebug(14180) << k_funcinfo << endl;
}

void YahooAccount::slotRemoveHandler( int /* fd */ )
{
//	kdDebug(14180) << k_funcinfo << endl;
}

#include "yahooaccount.moc"

// vim: set noet ts=4 sts=4 sw=4:

