/*
    translatorplugin.cpp

    Kopete Translator plugin

    Copyright (c) 2001-2002 by Duncan Mac-Vicar Prett       <duncan@kde.org>
    Copyright (c) 2002-2003 by Olivier Goffart      <ogoffart@tiscalinet.be>

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

#include <qapplication.h>
#include <qregexp.h>
#include <qsignal.h>
#include <qstring.h>

#include <kdebug.h>
#include <kaction.h>
#include <kgenericfactory.h>
#include <kglobal.h>
#include <kconfig.h>

#include "kopetemetacontact.h"
#include "kopetecontactlist.h"
#include "kopetemessagemanagerfactory.h"
#include "kopeteview.h"

#include "translatorplugin.h"
#include "translatordialog.h"
#include "translatorguiclient.h"
#include "translatorlanguages.h"

typedef KGenericFactory<TranslatorPlugin> TranslatorPluginFactory;
K_EXPORT_COMPONENT_FACTORY( kopete_translator, TranslatorPluginFactory( "kopete_translator" )  )

TranslatorPlugin::TranslatorPlugin( QObject *parent, const char *name, const QStringList & /* args */ )
: KopetePlugin( TranslatorPluginFactory::instance(), parent, name )
{


	if ( pluginStatic_ )
		kdDebug(14308)<<"####"<<"Translator already initialized"<<endl;
	else
		pluginStatic_ = this;

	m_languages = new TranslatorLanguages;

	connect( KopeteMessageManagerFactory::factory(), SIGNAL( aboutToDisplay( KopeteMessage & ) ), SLOT( slotIncomingMessage( KopeteMessage & ) ) );
	connect( KopeteMessageManagerFactory::factory(), SIGNAL( aboutToSend( KopeteMessage & ) ),    SLOT( slotOutgoingMessage( KopeteMessage & ) ) );

	connect( KopeteMessageManagerFactory::factory(), SIGNAL( messageManagerCreated( KopeteMessageManager * )) , SLOT( slotNewKMM( KopeteMessageManager * ) ) );

	QStringList keys;
	int k;
	QMap<QString, QString> m = m_languages->languagesMap();
	for ( k=0; k <= m_languages->numLanguages(); k++)
	{

		keys << m[ m_languages->languageKey(k) ];
	}
	m_actionLanguage=new KListAction(i18n("Set &Language"),"",0,  actionCollection() ,"contactLanguage");
	m_actionLanguage->setItems( keys );
	connect( m_actionLanguage, SIGNAL( activated() ), this, SLOT(slotSetLanguage()) );
	connect( KopeteContactList::contactList() , SIGNAL( metaContactSelected(bool) ) , this , SLOT(slotSelectionChanged(bool)));

	setXMLFile("translatorui.rc");

	//Add GUI action to all already existing kmm (if the plugin is launched when kopete already rining)
	QIntDict<KopeteMessageManager> sessions = KopeteMessageManagerFactory::factory()->sessions();
	QIntDictIterator<KopeteMessageManager> it( sessions );
	for ( ; it.current() ; ++it )
	{
		slotNewKMM(it.current());
	}
	loadSettings();
	connect(this, SIGNAL(settingsChanged()), this, SLOT( loadSettings() ) );
}

TranslatorPlugin::~TranslatorPlugin()
{
	pluginStatic_ = 0L;
}

TranslatorPlugin* TranslatorPlugin::plugin()
{
	return pluginStatic_ ;
}

TranslatorPlugin* TranslatorPlugin::pluginStatic_ = 0L;

void TranslatorPlugin::loadSettings()
{
	KConfig *config = KGlobal::config();
	int mode = 0;

	config->setGroup("Translator Plugin");
	m_myLang = config->readEntry("myLang", "null");
	m_service = config->readEntry("Service", "babelfish");

	if (config->readBoolEntry("IncomingDontTranslate", true))
		mode = 0;
	else if (config->readBoolEntry("IncomingShowOriginal", false))
		mode = 1;
	else if (config->readBoolEntry("IncomingTranslate", false))
		mode = 2;

	m_incomingMode = mode;

	if (config->readBoolEntry("OutgoingDontTranslate", true))
		mode = 0;
	else if (config->readBoolEntry("OutgoingShowOriginal", false))
		mode = 1;
	else if (config->readBoolEntry("OutgoingTranslate", false))
		mode = 2;
	else if (config->readBoolEntry("OutgoingAsk", false))
		mode = 3;

	m_outgoingMode = mode;

}


void TranslatorPlugin::slotSelectionChanged(bool b)
{
	m_actionLanguage->setEnabled(b);

	if(!b)
		return;

	KopeteMetaContact *m=KopeteContactList::contactList()->selectedMetaContacts().first();

	if(!m)
		return;

	//Update the checkmark

	/*QStringList keys;
	int k;
	for ( k=0; k <= m_lc; k++)
	{
		keys << m_langs[ languageKey(k) ];
	}*/

	QString languageKey = m->pluginData( this, "languageKey" );
	if( !languageKey.isEmpty() && languageKey != "null" )
		m_actionLanguage->setCurrentItem( m_languages->languageIndex( languageKey ) );
	else
		m_actionLanguage->setCurrentItem( m_languages->languageIndex( "null" ) );
}

void TranslatorPlugin::slotNewKMM(KopeteMessageManager *KMM)
{
	new TranslatorGUIClient(KMM);
}

void TranslatorPlugin::slotIncomingMessage( KopeteMessage& msg )
{
	if(m_incomingMode == DontTranslate)
		return;

//	kdDebug(14308) << "TranslatorPlugin::slotIncomingMessage " << m_prefs->incommingMode() << DontTranslate << endl;

	QString src_lang;
	QString dst_lang;

	if ( (msg.direction() == KopeteMessage::Inbound) && ( !msg.plainBody().isEmpty() ) )
	{
		KopeteMetaContact *from = msg.from()->metaContact();
		if( !from )
		{
			kdDebug(14308) << "TranslatorPlugin::slotIncomingMessage : No metaContact for source contact" << endl;
			return;
		}
		src_lang = from->pluginData( this, "languageKey" );
		if( src_lang.isEmpty() || src_lang == "null" )
		{
			kdDebug(14308) << "TranslatorPlugin::slotIncomingMessage : Cannot determine src Metacontact language (" << from->displayName() << ")" << endl;
			return;
		}

		dst_lang = m_myLang;

		if ( src_lang == dst_lang )
		{
			kdDebug(14308) << "TranslatorPlugin::slotIncomingMessage : Src and Dst languages are the same" << endl;
			return;
		}

		/* We search for src_dst */

		QStringList s = m_languages->supported( m_service );
		QStringList::ConstIterator i;

		for ( i = s.begin(); i != s.end() ; ++i )
		{
			if ( *i == src_lang + "_" + dst_lang )
			{
				sendTranslation(msg , translateMessage( msg.plainBody() , src_lang, dst_lang));
				return;
			}
		}
	}
	else
	{
		kdDebug(14308) << "TranslatorPlugin::slotIncomingMessage , outgoing or empty body" << endl;
	}
}

void TranslatorPlugin::slotOutgoingMessage( KopeteMessage& msg )
{
/*	kdDebug(14308) << "[Translator] Outgoing message " << msg.timestamp().toString("hhmmsszzz") << endl;
	kdDebug(14308) << "[Translator] Outgoing info: " << endl
		<< msg.body() << endl << "Direction: " << msg.direction();*/

	if(m_outgoingMode == DontTranslate)
		return;


	QString src_lang;
	QString dst_lang;

	if ( ( msg.direction() == KopeteMessage::Outbound ) && ( !msg.plainBody().isEmpty() ) )
	{
		src_lang = m_myLang;
//		kdDebug(14308) << "[Translator] ( Outgoing ) My lang is: " << src_lang << endl;

		// Sad, we have to consider only the first To: metacontact
		KopeteMetaContact *to = msg.to().first()->metaContact();
		if( !to )
		{
			kdDebug(14308) << "TranslatorPlugin::slotOutgoingMessage : No metaContact for first contact" << endl;
			return;
		}
		dst_lang = to->pluginData( this, "languageKey" );
		if( dst_lang.isEmpty() || dst_lang == "null" )
		{
			kdDebug(14308) << "TranslatorPlugin::slotOutgoingMessage :  Cannot determine dst Metacontact language (" << to->displayName() << ")" << endl;
			return;
		}

		if ( src_lang == dst_lang )
		{
			kdDebug(14308) << "TranslatorPlugin::slotOutgoingMessage :  Src and Dst languages are the same" << endl;
			return;
		}

		/* We search for src_dst */

		QStringList s = m_languages->supported( m_service );
		QStringList::ConstIterator i;

		for ( i = s.begin(); i != s.end() ; ++i )
		{
			if ( *i == src_lang + "_" + dst_lang )
			{
				sendTranslation(msg , translateMessage( msg.plainBody() , src_lang, dst_lang));
				return;
//				kdDebug(14308) << "[Translator] Outgoing, DONE" << endl;
				return;
			}
			else
			{
//				kdDebug(14308) << "[Translator] ( Outgoing ) " << src_lang << " and " << dst_lang << " != " << *i<< endl;
			}
		}
	}
	else
	{
		kdDebug(14308) << "TranslatorPlugin::slotOutgoingMessage : incoming or empty body" << endl;
	}
}

void TranslatorPlugin::translateMessage(const QString &msg , const QString &from, const QString &to , QObject *obj , const char* slot)
{
	QSignal completeSignal;
	completeSignal.connect( obj, slot );

	QString result=translateMessage(msg,from,to);

	completeSignal.setValue( result );
	completeSignal.activate();
}

QString TranslatorPlugin::translateMessage(const QString &msg , const QString &from, const QString &to)
{
	if ( m_service == "babelfish" )
		return babelTranslateMessage( msg ,from, to);
	if ( m_service == "google" )
		return googleTranslateMessage( msg ,from, to);
	return QString::null;
}

QString TranslatorPlugin::googleTranslateMessage( const QString &msg , const QString &from, const QString &to)
{
	kdDebug(14308) << "[Translator] Google Translating: [" << from << "_" << to << "] " << endl
		<< msg << endl << endl;

	QString body, lp;
	KURL translatorURL;
	QCString postData;
	KIO::TransferJob *job;

	translatorURL = "http://translate.google.com/translate_t";

	//body = KURL::encode_string("*-*-* " + msg.body() + " *-*-*");
	body = KURL::encode_string( msg );

	lp = from + "|" + to;

	postData = QString( "text=" + body + "&langpair=" + lp ).utf8();

	QString gurl = "http://translate.google.com/translate_t?text=" + body +"&langpair=" + lp;
	kdDebug(14308) << "[Translator] URL: " << gurl << endl;
	KURL geturl = gurl;

	//job = KIO::http_post( translatorURL, postData, true );
	job = KIO::get( geturl, false, true );

	//job->addMetaData("content-type", "application/x-www-form-urlencoded" );
	//job->addMetaData("referrer", "http://www.google.com");

	QObject::connect( job, SIGNAL(data( KIO::Job *,const QByteArray&)), this, SLOT(slotDataReceived( KIO::Job *,const QByteArray&)) );
	QObject::connect( job, SIGNAL(result( KIO::Job *)), this, SLOT(slotJobDone( KIO::Job *)) );

	/* KIO is async and we use a sync API, hay que dentrar a picarle nomas */
	while ( ! m_completed[ job ] )
		qApp->processEvents();

	QString data = QString::fromUtf8(m_data[job]);

	/* After hacks, we need to clean */
	m_data.remove( job );
	m_completed.remove( job );

	kdDebug(14308) << "[Translator]: Google response: "<< endl << data << endl;

	//QRegExp re("*-*-* (.*) *-*-*");
	QRegExp re("<textarea name=q rows=5 cols=45 wrap=PHYSICAL>(.*)</textarea>");
	re.setMinimal(true);
	re.search( data );

	QString translated = re.cap(1);

	return translated;

//	sendTranslation(msg,translated);
}

QString TranslatorPlugin::babelTranslateMessage( const QString &msg , const QString &from, const QString &to)
{
	kdDebug(14308) << "TranslatorPlugin::babelTranslateMessage : [" << from << "_" << to << "] " << endl ;

	QString body, lp;
	//KURL translatorURL;
	//QCString postData;
	KIO::TransferJob *job;

	//translatorURL = "http://babelfish.altavista.com/babelfish/tr";

	//body = KURL::encode_string("*-*-* " + msg.body() + " *-*-*");
	body = KURL::encode_string( msg);

	lp = from + "_" + to;

	//postData = "enc=utf8&doit=done&tt=urltext&urltext=" + body +"&lp=" + lp ;

	QString gurl = "http://babelfish.altavista.com/babelfish/tr?enc=utf8&doit=done&tt=urltext&urltext=" + body +"&lp=" + lp;
	kdDebug(14308) << "TranslatorPlugin::babelTranslateMessage : URL: " << gurl << endl;
	KURL geturl = gurl;

	//job = KIO::http_post( translatorURL, postData, true );
	job = KIO::get( geturl, false, true );

	//job->addMetaData("content-type", "application/x-www-form-urlencoded" );
	//job->addMetaData("referrer", "http://www.google.com");

	QObject::connect( job, SIGNAL(data( KIO::Job *,const QByteArray&)), this, SLOT(slotDataReceived( KIO::Job *,const QByteArray&)) );
	QObject::connect( job, SIGNAL(result( KIO::Job *)), this, SLOT(slotJobDone( KIO::Job *)) );

	/* KIO is async and we use a sync API, hay que dentrar a picarle nomas */
	while ( ! m_completed[ job ] )
		qApp->processEvents();

	QString data = QString::fromUtf8(m_data[job]);

	/* After hacks, we need to clean */
	m_data.remove( job );
	m_completed.remove( job );

//	kdDebug(14308) << "[Translator]: Babelfish response: "<< endl << data << endl;

	//QRegExp re("*-*-* (.*) *-*-*");
	QRegExp re("<Div style=padding:10px; lang=..>(.*)</div");
	re.setMinimal(true);
	re.search( data );

	QString translated = re.cap(1);
	return translated;

//	sendTranslation(msg,translated);
}

void TranslatorPlugin::sendTranslation(KopeteMessage &msg, const QString &translated)
{
	if ( translated.isEmpty() )
	{
		kdDebug(14308) << "TranslatorPlugin::sendTranslation - WARNING: Translated text is empty" <<endl;
		return;
	}

	TranslateMode mode=DontTranslate;

	switch (msg.direction())
	{
		case KopeteMessage::Outbound:
			mode=(TranslateMode)m_outgoingMode;
			break;
		case KopeteMessage::Inbound:
			mode=(TranslateMode)m_incomingMode;
			break;
		default:
			kdDebug(14308) << "TranslatorPlugin::sendTranslation - WARNING: can't determine if it is an incoming or outgoing message" <<endl;
	};

	switch (mode)
	{
		case JustTranslate:
			msg.setBody(translated, msg.format());
			break;
		case ShowOriginal:
			msg.setBody(i18n("%2\nAuto Translated: %1")
#if QT_VERSION < 0x030200
					.arg(translated).arg(msg.plainBody()), msg.format());
#else
					.arg(translated , msg.plainBody()), msg.format());
#endif
			break;
		case ShowDialog:
		{
			TranslatorDialog *d=new TranslatorDialog(translated);
			d->exec();
			msg.setBody(d->translatedText(),msg.format());
			delete d;
			break;
		}
		case DontTranslate:
		default:
			//do nothing
			break;
	};

}


void TranslatorPlugin::slotDataReceived ( KIO::Job *job, const QByteArray &data)
{
	m_data[job] += QCString( data, data.size()+1 );
}

void TranslatorPlugin::slotJobDone ( KIO::Job *job)
{
	m_completed[job] = true;
	QObject::disconnect( job, SIGNAL(data( KIO::Job *,const QByteArray&)), this, SLOT(slotDataReceived( KIO::Job *,const QByteArray&)) );
	QObject::disconnect( job, SIGNAL(result( KIO::Job *)), this, SLOT(slotJobDone( KIO::Job *)) );

}

void TranslatorPlugin::slotSetLanguage()
{
	KopeteMetaContact *m=KopeteContactList::contactList()->selectedMetaContacts().first();
	if( m && m_actionLanguage )
	{
		m->setPluginData( this, "languageKey", m_languages->languageKey( m_actionLanguage->currentItem() ) );
	}
}

#include "translatorplugin.moc"

// vim: set noet ts=4 sts=4 sw=4:

