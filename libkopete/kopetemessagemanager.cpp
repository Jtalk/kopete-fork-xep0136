/*
    kopetemessagemanager.cpp - Manages all chats

    Copyright (c) 2002      by Duncan Mac-Vicar Prett <duncan@kde.org>
    Copyright (c) 2002      by Daniel Stone <dstone@kde.org>
    Copyright (c) 2002      by Martijn Klingens <klingens@kde.org>
    Copyright (c) 2002-2003 by Olivier Goffart <ogoffart@tiscalinet.be>
    Copyright (c) 2003      by Jason Keirstead   <jason@keirstead.org>

    Kopete    (c) 2002-2003 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include <kdebug.h>
#include <klocale.h>
#include <kopetenotifyclient.h>
#include <qapplication.h>
#include <kglobal.h>
#include <qregexp.h>
#include <kmessagebox.h>

#include "kopeteaccount.h"
#include "kopetemessagemanager.h"
#include "kopetemessagemanagerfactory.h"
#include "kopeteprefs.h"
#include "kopetemetacontact.h"
#include "kopetecommandhandler.h"
#include "kopeteview.h"


struct KMMPrivate
{
	KopeteContactPtrList mContactList;
	const KopeteContact *mUser;
	QMap<const KopeteContact *, KopeteOnlineStatus> contactStatus;
	KopeteProtocol *mProtocol;
	int mId;
	bool isEmpty;
	bool mCanBeDeleted;
	QDateTime awayTime;
	QString displayName;
	KopeteView *view;
};

KopeteMessageManager::KopeteMessageManager( const KopeteContact *user,
	KopeteContactPtrList others, KopeteProtocol *protocol, int id, const char *name )
: QObject( user->account(), name )
{
	d = new KMMPrivate;
	d->mUser = user;
	d->mProtocol = protocol;
	d->mId = id;
	d->isEmpty= others.isEmpty();
	d->mCanBeDeleted = true;
	d->view=0L;

	for( KopeteContact *c = others.first(); c; c = others.next() )
	{
		addContact(c,true);
	}

	connect( user, SIGNAL( onlineStatusChanged( KopeteContact *, const KopeteOnlineStatus &, const KopeteOnlineStatus & ) ), this,
		SLOT( slotStatusChanged( KopeteContact *, const KopeteOnlineStatus &, const KopeteOnlineStatus & ) ) );

	connect( this, SIGNAL( contactChanged() ), this, SLOT( slotUpdateDisplayName() ) );
}

KopeteMessageManager::~KopeteMessageManager()
{
/*	for( KopeteContact *c = d->mContactList.first(); c; c = d->mContactList.next() )
		c->setConversations( c->conversations() - 1 );*/

	if (!d) return;
	d->mCanBeDeleted = false; //prevent double deletion
	KopeteMessageManagerFactory::factory()->removeSession( this );
	emit(closing( this ) );
	delete d;
}

void KopeteMessageManager::slotStatusChanged( KopeteContact *c, const KopeteOnlineStatus &status, const KopeteOnlineStatus &oldStatus )
{
	if(!KopetePrefs::prefs()->notifyAway())
		return;
	if( status.status() == KopeteOnlineStatus::Away )
	{
		d->awayTime = QDateTime::currentDateTime();
		KopeteMessage msg(c, d->mContactList, i18n("%1 has been marked as away.")
			.arg( QString::fromLatin1("/me") ), KopeteMessage::Outbound, KopeteMessage::PlainText);
		sendMessage( msg );
	}
	else if( oldStatus.status() == KopeteOnlineStatus::Away && status.status() == KopeteOnlineStatus::Online )
	{
		KopeteMessage msg(c, d->mContactList, i18n("%1 is no longer marked as away. Gone since %1")
			.arg( QString::fromLatin1("/me") ).arg(KGlobal::locale()->formatDateTime(d->awayTime, true)), KopeteMessage::Outbound, KopeteMessage::PlainText);
		sendMessage( msg );
	}
}

void KopeteMessageManager::setContactOnlineStatus( const KopeteContact *contact, const KopeteOnlineStatus &status )
{
	d->contactStatus[ contact ] = status;
}

const KopeteOnlineStatus KopeteMessageManager::contactOnlineStatus( const KopeteContact *contact ) const
{
	if( d->contactStatus.contains( contact ) )
		return d->contactStatus[ contact ];

	return contact->onlineStatus();
}

const QString KopeteMessageManager::displayName()
{
	if( d->displayName.isNull() )
	{
		connect( this, SIGNAL( contactChanged() ), this, SLOT( slotUpdateDisplayName() ) );
		slotUpdateDisplayName();
	}

	return d->displayName;
}

void KopeteMessageManager::setDisplayName( const QString &newName )
{
	disconnect( this, SIGNAL( contactChanged() ), this, SLOT( slotUpdateDisplayName() ) );

	d->displayName = newName;

	emit( displayNameChanged() );
}

void KopeteMessageManager::slotUpdateDisplayName()
{
	QString nextDisplayName;

	KopeteContact *c = d->mContactList.first();
	if( c->metaContact() )
		d->displayName = c->metaContact()->displayName();
	else
		d->displayName = c->displayName();

	//If we have only 1 contact, add the status of him
	if( d->mContactList.count() == 1 )
	{
		d->displayName.append( QString::fromLatin1( " (%1)").arg( c->onlineStatus().description() ) );
	}
	else
	{
		while( ( c = d->mContactList.next() ) )
		{
			if( c->metaContact() )
				nextDisplayName = c->metaContact()->displayName();
			else
				nextDisplayName = c->displayName();
			d->displayName.append( QString::fromLatin1( ", " ) ).append( nextDisplayName );
		}
	}

	emit( displayNameChanged() );
}

const KopeteContactPtrList& KopeteMessageManager::members() const
{
	return d->mContactList;
}

const KopeteContact* KopeteMessageManager::user() const
{
	return d->mUser;
}

KopeteProtocol* KopeteMessageManager::protocol() const
{
	return d->mProtocol;
}

int KopeteMessageManager::mmId() const
{
	return d->mId;
}

void KopeteMessageManager::setMMId( int id )
{
	d->mId = id;
}

void KopeteMessageManager::sendMessage(KopeteMessage &message)
{
	message.setManager(this);
	KopeteMessage sentMessage = message;
	if( !KopeteCommandHandler::commandHandler()->processMessage( message, this ) )
	{
		emit messageSent(sentMessage, this);
		if ( !account()->isAway() || KopetePrefs::prefs()->soundIfAway() )
			KNotifyClient::event( 0 , QString::fromLatin1( "kopete_outgoing" ), i18n( "Outgoing Message Sent" ) );
	}
	else
		emit( messageSuccess() );
}

void KopeteMessageManager::messageSucceeded()
{
	emit( messageSuccess() );
}

void KopeteMessageManager::appendMessage( KopeteMessage &msg )
{
	msg.setManager(this);

	if( msg.direction() == KopeteMessage::Inbound )
	{
		if( KopetePrefs::prefs()->highlightEnabled() && !user()->displayName().isEmpty() && msg.plainBody().contains( QRegExp(QString::fromLatin1("\\b(%1)\\b").arg(user()->displayName()),false) ) )
			msg.setImportance( KopeteMessage::Highlight );

		emit( messageReceived( msg, this ) );
	}

	emit messageAppended( msg, this );
}

void KopeteMessageManager::addContact( const KopeteContact *c, bool suppress )
{
	if ( d->mContactList.contains(c) )
	{
		kdDebug(14010) << k_funcinfo << "Contact already exists" <<endl;
		emit contactAdded(c, suppress);
	}
	else
	{
		if(d->mContactList.count()==1 && d->isEmpty)
		{
			KopeteContact *old=d->mContactList.first();
			d->mContactList.remove(old);
			d->mContactList.append(c);
			//disconnect (old, SIGNAL(displayNameChanged(const QString &, const QString &)), this, SIGNAL(contactChanged()));
			disconnect (old, SIGNAL(onlineStatusChanged( KopeteContact*, const KopeteOnlineStatus&, const KopeteOnlineStatus&)), this, SIGNAL(contactChanged()));
			if(old->metaContact())
				disconnect (old->metaContact(), SIGNAL(displayNameChanged(const QString &, const QString &)), this, SIGNAL(contactChanged()));
			emit contactAdded(c, suppress);
			emit contactRemoved(old, QString::null);
		}
		else
		{
			d->mContactList.append(c);
			emit contactAdded(c, suppress);
		}
		//c->setConversations( c->conversations() + 1 );
		//connect (c, SIGNAL(displayNameChanged(const QString &,const QString &)), this, SIGNAL(contactChanged()));
		connect (c, SIGNAL(onlineStatusChanged( KopeteContact*, const KopeteOnlineStatus&, const KopeteOnlineStatus&)), this, SIGNAL(contactChanged()));
		if(c->metaContact())
			connect (c->metaContact(), SIGNAL(displayNameChanged(const QString &, const QString &)), this, SIGNAL(contactChanged()));
		connect (c, SIGNAL(contactDestroyed(KopeteContact*)) , this , SLOT(slotContactDestroyed(KopeteContact*)));
	}
	d->isEmpty=false;
	slotUpdateDisplayName();
}

void KopeteMessageManager::removeContact( const KopeteContact *c, const QString& raison )
{
	if(!c || !d->mContactList.contains(c))
		return;

	if(d->mContactList.count()==1)
	{
		kdDebug(14010) << k_funcinfo << "Contact not removed. Keep always one contact" <<endl;
		d->isEmpty=true;
	}
	else
	{
		d->mContactList.remove( c );
		//disconnect (c, SIGNAL(displayNameChanged(const QString &, const QString &)), this, SIGNAL(contactChanged()));
		disconnect (c, SIGNAL(onlineStatusChanged( KopeteContact*, const KopeteOnlineStatus&, const KopeteOnlineStatus&)), this, SIGNAL(contactChanged()));
		if(c->metaContact())
			disconnect (c->metaContact(), SIGNAL(displayNameChanged(const QString &, const QString &)), this, SIGNAL(contactChanged()));
		disconnect (c, SIGNAL(contactDestroyed(KopeteContact*)) , this , SLOT(slotContactDestroyed(KopeteContact*)));
		//c->setConversations( c->conversations() - 1 );
	}
	emit contactRemoved(c, raison);
	slotUpdateDisplayName();
}

void KopeteMessageManager::receivedTypingMsg( const KopeteContact *c , bool t )
{
	emit(remoteTyping( c, t ));
}

void KopeteMessageManager::receivedTypingMsg( const QString &contactId , bool t )
{
	for( KopeteContact *it = d->mContactList.first(); it; it = d->mContactList.next() )
	{
		if( it->contactId() == contactId )
		{
			receivedTypingMsg( it, t );
			return;
		}
	}
}

void KopeteMessageManager::typing ( bool t )
{
	emit typingMsg(t);
}

void KopeteMessageManager::setCanBeDeleted ( bool b )
{
	d->mCanBeDeleted = b;
	if(b && !d->view)
		deleteLater();
}

KopeteView* KopeteMessageManager::view(bool canCreate  , KopeteMessage::MessageType type )
{
	if(!d->view && canCreate)
	{
		d->view=KopeteMessageManagerFactory::factory()->createView( this , type );
		if(d->view)
			connect( d->view->mainWidget(), SIGNAL( closing( KopeteView * ) ), this, SLOT( slotViewDestroyed( ) ) );
		else
			KMessageBox::error( 0L, i18n( "<qt>An error has occurred when creating a new chat window. The chat window has not been created.</qt>" ),
							i18n( "Error while creating the chat window - Kopete" )  );

	}
	return d->view;
}

void KopeteMessageManager::slotViewDestroyed()
{
	d->view=0L;
	if(d->mCanBeDeleted)
		deleteLater();
}

KopeteAccount *KopeteMessageManager::account() const
{
	return user()->account();
}

void KopeteMessageManager::slotContactDestroyed(KopeteContact* c)
{
	if(!c || !d->mContactList.contains(c))
		return;

	if(d->mContactList.count()==1)
	{
		deleteLater(); //the contact has been deleted, it is better to delete this
	}
	else
	{
		d->mContactList.remove( c );
		emit contactRemoved(c , QString::null);
	}
}

#include "kopetemessagemanager.moc"


// vim: set noet ts=4 sts=4 sw=4:
