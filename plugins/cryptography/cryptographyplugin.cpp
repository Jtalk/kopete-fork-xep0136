/***************************************************************************
                          cryptographyplugin.cpp  -  description
                             -------------------
    begin                : jeu nov 14 2002
    copyright            : (C) 2002 by Olivier Goffart
    email                : ogoffart@tiscalinet.be
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qstylesheet.h>

#include <kdebug.h>
#include <kaction.h>
#include <klocale.h>
#include <kgenericfactory.h>

#include "kopetemessage.h"
#include "kopetemetacontact.h"
#include "kopetemessagemanager.h"
#include "kopetemessagemanagerfactory.h"

#include "cryptographyplugin.h"
#include "cryptographypreferences.h"
#include "cryptographyselectuserkey.h"

#include "kgpginterface.h"

K_EXPORT_COMPONENT_FACTORY( kopete_cryptography, KGenericFactory<CryptographyPlugin> );

CryptographyPlugin::CryptographyPlugin( QObject *parent, const char *name,
	const QStringList &/*args*/ )
: KopetePlugin( parent, name ) ,
		m_cachedPass()
{
	if( !pluginStatic_ )
		pluginStatic_=this;

	//TODO: found a pixmap
	m_prefs = new CryptographyPreferences ( "kgpg", this );

	connect( KopeteMessageManagerFactory::factory(),
		SIGNAL( aboutToDisplay( KopeteMessage & ) ),
		SLOT( slotIncomingMessage( KopeteMessage & ) ) );
	connect( KopeteMessageManagerFactory::factory(),
		SIGNAL( aboutToSend( KopeteMessage & ) ),
		SLOT( slotOutgoingMessage( KopeteMessage & ) ) );

	m_collection=0l;
	m_currentMetaContact=0L;

}

CryptographyPlugin::~CryptographyPlugin()
{
	pluginStatic_ = 0L;
	delete m_collection;
}

CryptographyPlugin* CryptographyPlugin::plugin()
{
	return pluginStatic_ ;
}

CryptographyPlugin* CryptographyPlugin::pluginStatic_ = 0L;

QCString CryptographyPlugin::cachedPass()
{
	return pluginStatic_->m_cachedPass;
}

void CryptographyPlugin::setCachedPass(const QCString& p)
{
	pluginStatic_->m_cachedPass=p;
}


KActionCollection *CryptographyPlugin::customContextMenuActions(KopeteMetaContact *m)
{
	delete m_collection;

	m_collection = new KActionCollection(this);

	KAction *action=new KAction( i18n("&Select Cryptography Public Key"), "kgpg", 0, this, SLOT (slotSelectContactKey()), m_collection);

	m_collection->insert(action);
	m_currentMetaContact=m;
	return m_collection;
}

/*KActionCollection *CryptographyPlugin::customChatActions(KopeteMessageManager *KMM)
{
	delete m_actionCollection;

	m_actionCollection = new KActionCollection(this);
	KAction *actionTranslate = new KAction( i18n ("Translate"), 0,
		this, SLOT( slotTranslateChat() ), m_actionCollection, "actionTranslate" );
	m_actionCollection->insert( actionTranslate );

	m_currentMessageManager=KMM;
	return m_actionCollection;
}*/

void CryptographyPlugin::slotIncomingMessage( KopeteMessage& msg )
{
	QString body=msg.plainBody();
	if(!body.startsWith("-----BEGIN PGP MESSAGE----"))
		return;

	if(msg.direction() != KopeteMessage::Inbound)
	{
		kdDebug(14303) << "CryptographyPlugin::slotIncomingMessage: inbound messages" <<endl;
		QString plainBody;
		if ( m_cachedMessages.contains( body ) ) {
			plainBody = m_cachedMessages[ body ];
			m_cachedMessages.remove( body );
		} else {
			plainBody = KgpgInterface::KgpgDecryptText( body, m_prefs->privateKey() );
		}

		msg.setBody("<table width=\"100%\" border=0 cellspacing=0 cellpadding=0><tr bgcolor=\"#41FFFF\"><td><font size=\"-1\"><b>"+i18n("Outgoing Encrypted Message")+"</b></font></td></tr><tr bgcolor=\"#DDFFFF\"><td>"+QStyleSheet::escape(plainBody)+"</td></tr></table>"
			,KopeteMessage::RichText);

		//if there are too messages in cache, clear the cache
		if(m_cachedMessages.count()>10)
			m_cachedMessages.clear();
		return;
	}

	body=KgpgInterface::KgpgDecryptText(body, m_prefs->privateKey());

	if(!body.isEmpty())
	{
		body="<table width=\"100%\" border=0 cellspacing=0 cellpadding=0><tr bgcolor=\"#41FF41\"><td><font size=\"-1\"><b>"+i18n("Incoming Encrypted Message")+"</b></font></td></tr><tr bgcolor=\"#DDFFDD\"><td>"+QStyleSheet::escape(body)+"</td></tr></table>";
		msg.setBody(body,KopeteMessage::RichText);
	}

}

void CryptographyPlugin::slotOutgoingMessage( KopeteMessage& msg )
{
	if(msg.direction() != KopeteMessage::Outbound)
		return;

	QStringList keys;
	QPtrList<KopeteContact> contactlist = msg.to();
	for( KopeteContact *c = contactlist.first(); c; c = contactlist.next() )
	{
		QString tmpKey = c->metaContact()->pluginData( this, "gpgKey" );
		if( tmpKey.isEmpty() )
		{
			kdDebug( 14303 ) << "CryptographyPlugin::slotOutgoingMessage: no key selected for one contact" <<endl;
			return;
		}
		keys.append( tmpKey );
	}
	// always encrypt to self, too
	keys.append( m_prefs->privateKey() );
	QString key = keys.join( " " );

	if(key.isEmpty())
	{
		kdDebug(14303) << "CryptographyPlugin::slotOutgoingMessage: empty key" <<endl;
		return;
	}

	QString original=msg.plainBody();

	/* Code From KGPG */

	//////////////////              encode from editor
	QString encryptOptions="";

	//if (utrust==true)
		encryptOptions+=" --always-trust ";
	//if (arm==true)
		encryptOptions+=" --armor ";

	/* if (pubcryptography==true)
	{
		if (gpgversion<120) encryptOptions+=" --compress-algo 1 --cipher-algo cast5 ";
		else encryptOptions+=" --cryptography6 ";
	}*/

// if (selec==NULL) {KMessageBox::sorry(0,i18n("You have not choosen an encryption key..."));return;}

	QString resultat=KgpgInterface::KgpgEncryptText(original,key,encryptOptions);
	if (resultat!="")
	{
		msg.setBody(resultat,KopeteMessage::PlainText);
		m_cachedMessages.insert(resultat,original);
	}
	else
		kdDebug(14303) << "CryptographyPlugin::slotOutgoingMessage: empty result" <<endl;

}

void CryptographyPlugin::slotSelectContactKey()
{
	QString key = m_currentMetaContact->pluginData( this, "gpgKey" );
	CryptographySelectUserKey *opts = new CryptographySelectUserKey( key, m_currentMetaContact );
	opts->exec();
	if( opts->result() )
	{
		key = opts->publicKey();
		m_currentMetaContact->setPluginData( this, "gpgKey", key );
	}
	delete opts;
}

#include "cryptographyplugin.moc"

// vim: set noet ts=4 sts=4 sw=4:

