/*  *************************************************************************
    *   copyright: (C) 2003 Richard L�rk�ng <nouseforaname@home.se>         *
    *   copyright: (C) 2003 Gav Wood <gav@kde.org>                          *
    *************************************************************************
*/

/*  *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#undef KDE_NO_COMPAT
#include <kaction.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "kopetemessagemanagerfactory.h"
#include "kopeteaccount.h"
#include "serviceloader.h"

#include "smscontact.h"
#include "smsprotocol.h"
#include "smsaccount.h"
#include "smsuserpreferences.h"

SMSContact::SMSContact( KopeteAccount* _account, const QString &phoneNumber,
	const QString &displayName, KopeteMetaContact *parent )
: KopeteContact( _account, phoneNumber, parent ), m_phoneNumber( phoneNumber )
{
	kdWarning( 14160 ) << k_funcinfo << " this = " << this << ", phone = " << phoneNumber << endl;
	setDisplayName( displayName );

	m_msgManager = 0L;
	m_actionPrefs = 0L;

	setOnlineStatus( SMSProtocol::protocol()->SMSUnknown );
}

void SMSContact::slotSendingSuccess(const KopeteMessage &msg)
{
//	KMessageBox::information(0L, i18n("Message sent"), output.join("\n"), i18n("Message Sent"));
	manager()->messageSucceeded();
	manager()->appendMessage((KopeteMessage &)msg);
}

void SMSContact::slotSendingFailure(const KopeteMessage &/*msg*/, const QString &error)
{
	KMessageBox::detailedError(0L, i18n("Something went wrong when sending message."), error,
			i18n("Could Not Send Message"));
//	manager()->messageFailed();
	// TODO: swap for failed as above. show it anyway for now to allow closing of window.
	manager()->messageSucceeded();
}

void SMSContact::serialize( QMap<QString, QString> &serializedData,
	QMap<QString, QString> & /* addressBookData */ )
{
	// Contact id and display name are already set for us
	if (m_phoneNumber != contactId())
		serializedData[ "contactId" ] = m_phoneNumber;
}

KopeteMessageManager* SMSContact::manager( bool )
{
	kdWarning( 14160 ) << k_funcinfo << " this = " << this << endl;
	if ( m_msgManager )
	{
		return m_msgManager;
	}
	else
	{
		QPtrList<KopeteContact> contacts;
		contacts.append(this);
		m_msgManager = KopeteMessageManagerFactory::factory()->create(account()->myself(), contacts, protocol());
		connect(m_msgManager, SIGNAL(messageSent(KopeteMessage&, KopeteMessageManager*)),
		this, SLOT(slotSendMessage(KopeteMessage&)));
		connect(m_msgManager, SIGNAL(destroyed()), this, SLOT(slotMessageManagerDestroyed()));
		connect(this, SIGNAL(messageSuccess()), m_msgManager, SIGNAL(messageSuccess()));
		return m_msgManager;
	}
}

void SMSContact::slotMessageManagerDestroyed()
{
	m_msgManager = 0L;
}

void SMSContact::slotSendMessage(KopeteMessage &msg)
{
	kdWarning( 14160 ) << k_funcinfo << " this = " << this << endl;
	QString sName = account()->pluginData(protocol(), "ServiceName");

	SMSService *s = ServiceLoader::loadService(sName, account());

	if (s == 0L) return;

	connect (s, SIGNAL(messageSent(const KopeteMessage &)), this, SLOT(slotSendingSuccess(const KopeteMessage &)));
	connect (s, SIGNAL(messageNotSent(const KopeteMessage &, const QString &)), this, SLOT(slotSendingFailure(const KopeteMessage &, const QString &)));

	int msgLength = msg.plainBody().length();

	if (s->maxSize() == -1)
		s->send(msg);
	else if (s->maxSize() < msgLength)
	{	if (dynamic_cast<SMSAccount *>(account())->splitNowMsgTooLong(s->maxSize(), msgLength))
			for (int i=0; i < msgLength / s->maxSize() + 1; i++)
			{	QString text = msg.plainBody();
				text = text.mid( s->maxSize() * i, s->maxSize() );
				KopeteMessage m( msg.from(), msg.to(), text, KopeteMessage::Outbound);
				s->send(m);
			}
		else
			slotSendingFailure(msg, i18n("Message too long."));
	}
	else
		s->send(msg);

//	delete s;

	kdWarning( 14160 ) << "<<<" << endl;
}

bool SMSContact::isReachable()
{
	if ( onlineStatus().status() != KopeteOnlineStatus::Unknown )
		return true;
	else
		return false;
}

void SMSContact::slotUserInfo()
{
}

void SMSContact::slotDeleteContact()
{
	kdWarning( 14160 ) << k_funcinfo << " this = " << this << endl;
	deleteLater();
}

const QString SMSContact::qualifiedNumber()
{
	QString number = m_phoneNumber;
	dynamic_cast<SMSAccount *>(account())->translateNumber(number);
	return number;
}

const QString &SMSContact::phoneNumber()
{
	return m_phoneNumber;
}

void SMSContact::setPhoneNumber( const QString phoneNumber )
{
	deleteLater();
	new SMSContact(account(), phoneNumber, displayName(), metaContact());
}

QPtrList<KAction>* SMSContact::customContextMenuActions()
{
	QPtrList<KAction> *m_actionCollection = new QPtrList<KAction>();
	if( !m_actionPrefs )
		m_actionPrefs = new KAction(i18n("&Contact Settings"), 0, this, SLOT(userPrefs()), this, "userPrefs");

	m_actionCollection->append( m_actionPrefs );

	return m_actionCollection;
}

void SMSContact::userPrefs()
{
	SMSUserPreferences* p = new SMSUserPreferences( this );
	p->show();
}

#include "smscontact.moc"

// vim: set noet ts=4 sts=4 sw=4:

