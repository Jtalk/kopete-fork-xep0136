/*
    yahoocontact.cpp - Yahoo Contact

    Copyright (c) 2003-2004 by Matt Rogers <matt.rogers@kdemail.net>
    Copyright (c) 2002 by Duncan Mac-Vicar Prett <duncan@kde.org>

    Portions based on code by Bruno Rodrigues <bruno.rodrigues@litux.org>

    Copyright (c) 2002 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/
#include "kopetegroup.h"
#include "kopetechatsession.h"
#include "kopeteonlinestatus.h"
#include "kopetemetacontact.h"
#include "kopetechatsessionmanager.h"
#include "kopetemetacontact.h"
#include "kopeteuiglobal.h"
#include "kopeteview.h"

// Local Includes
#include "yahoocontact.h"
#include "yahooaccount.h"
#include "client.h"
#include "yahoowebcamdialog.h"
#include "yahoostealthsetting.h"
#include "yahoochatsession.h"

// QT Includes
#include <qregexp.h>
#include <qfile.h>
#include <qradiobutton.h>

// KDE Includes
#include <kdebug.h>
#include <kaction.h>
#include <kapplication.h>
#include <klocale.h>
#include <krun.h>
#include <kshortcut.h>
#include <kmessagebox.h>
#include <ktempfile.h>
#include <kio/global.h>
#include <kio/job.h>
#include <kurl.h>
#include <kio/jobclasses.h>
#include <kimageio.h>
#include <kstandarddirs.h>
#include <kfiledialog.h>

YahooContact::YahooContact( YahooAccount *account, const QString &userId, const QString &fullName, Kopete::MetaContact *metaContact )
	: Kopete::Contact( account, userId, metaContact )
{
	//kdDebug(YAHOO_GEN_DEBUG) << k_funcinfo << endl;

	m_userId = userId;
	if ( metaContact )
		m_groupName = metaContact->groups().first()->displayName();
	m_manager = 0L;
	m_account = account;
	m_stealthed = false;
	m_receivingWebcam = false;

	// Update ContactList
	setNickName( fullName );
	setOnlineStatus( static_cast<YahooProtocol*>( m_account->protocol() )->Offline );
	setFileCapable( true );
	
	if ( m_account->haveContactList() )
		syncToServer();
	
	m_webcamDialog = 0L;
	m_webcamAction = 0L;
	m_stealthAction = 0L;
	m_inviteWebcamAction = 0L;
	m_inviteConferenceAction = 0L;

	m_buzzAction = 0L;
}

YahooContact::~YahooContact()
{
}

QString YahooContact::userId() const
{
	return m_userId;
}

void YahooContact::setOnlineStatus(const Kopete::OnlineStatus &status)
{
	if( m_stealthed && status.internalStatus() <= 999)	// Not Stealted -> Stealthed
	{
		Contact::setOnlineStatus( 
			Kopete::OnlineStatus(status.status() ,
			(status.weight()==0) ? 0 : (status.weight() -1)  ,
			protocol() ,
			status.internalStatus()+1000 ,
			status.overlayIcons() + QStringList("yahoo_stealthed") ,
			i18n("%1|Stealthed").arg( status.description() ) ) );
	}
	else if( !m_stealthed && status.internalStatus() > 999 )// Stealthed -> Not Stealthed
		Contact::setOnlineStatus( static_cast< YahooProtocol *>( protocol() )->statusFromYahoo( status.internalStatus() - 1000 ) );
	else
		Contact::setOnlineStatus( status );
	
	if( status.status() == Kopete::OnlineStatus::Offline ) 
		removeProperty( ((YahooProtocol*)(m_account->protocol()))->awayMessage);
}

void YahooContact::setStealthed( bool stealthed )
{
	m_stealthed = stealthed;
	setOnlineStatus( onlineStatus() );
}

bool YahooContact::stealthed()
{
	return m_stealthed;
}

void YahooContact::serialize(QMap<QString, QString> &serializedData, QMap<QString, QString> &addressBookData)
{
	//kdDebug(YAHOO_GEN_DEBUG) << k_funcinfo << endl;

	Kopete::Contact::serialize(serializedData, addressBookData);
}

void YahooContact::syncToServer()
{
	kdDebug(YAHOO_GEN_DEBUG) << k_funcinfo  << endl;
	if(!m_account->isConnected()) return;

	if ( !m_account->isOnServer(m_userId) && !metaContact()->isTemporary() )
	{	kdDebug(YAHOO_GEN_DEBUG) << "Contact " << m_userId << " doesn't exist on server-side. Adding..." << endl;

		Kopete::GroupList groupList = metaContact()->groups();
		foreach(Kopete::Group *g, groupList)
			m_account->yahooSession()->addBuddy(m_userId, g->displayName() );
	}
}

void YahooContact::sync(unsigned int flags)
{
	kdDebug(YAHOO_GEN_DEBUG) << k_funcinfo  << endl;
	if ( !m_account->isConnected() )
		return;

	if ( !m_account->isOnServer( contactId() ) )
	{
		//TODO: Share this code with the above function
		kdDebug(YAHOO_GEN_DEBUG) << k_funcinfo << "Contact isn't on the server. Adding..." << endl;
		Kopete::GroupList groupList = metaContact()->groups();
		foreach(Kopete::Group *g, groupList)
			m_account->yahooSession()->addBuddy(m_userId, g->displayName() );
	}
	else
	{
		QString newGroup = metaContact()->groups().first()->displayName();
		if ( flags & Kopete::Contact::MovedBetweenGroup )
		{
			kdDebug(YAHOO_GEN_DEBUG) << k_funcinfo << "contact changed groups. moving on server" << endl;
			m_account->yahooSession()->moveBuddy( contactId(), m_groupName, newGroup );
			m_groupName = newGroup;
		}
	}
}


bool YahooContact::isOnline() const
{
	//kdDebug(YAHOO_GEN_DEBUG) << k_funcinfo << endl;
	return onlineStatus().status() != Kopete::OnlineStatus::Offline && onlineStatus().status() != Kopete::OnlineStatus::Unknown;
}

bool YahooContact::isReachable()
{
	//kdDebug(YAHOO_GEN_DEBUG) << k_funcinfo << endl;
	if ( m_account->isConnected() )
		return true;
	else
		return false;
}

Kopete::ChatSession *YahooContact::manager( Kopete::Contact::CanCreateFlags canCreate )
{
	if( !m_manager && canCreate)
	{
		Kopete::ContactPtrList m_them;
		m_them.append( this );
		m_manager = new YahooChatSession( protocol(), account()->myself(), m_them  );
		connect( m_manager, SIGNAL( destroyed() ), this, SLOT( slotChatSessionDestroyed() ) );
		connect( m_manager, SIGNAL( messageSent ( Kopete::Message&, Kopete::ChatSession* ) ), this, SLOT( slotSendMessage( Kopete::Message& ) ) );
		connect( m_manager, SIGNAL( myselfTyping( bool) ), this, SLOT( slotTyping( bool ) ) );
		connect( m_account, SIGNAL( receivedTypingMsg( const QString &, bool ) ), m_manager, SLOT( receivedTypingMsg( const QString&, bool ) ) );
		connect( this, SIGNAL( signalWebcamInviteAccepted() ), this, SLOT( requestWebcam() ) );
		connect( this, SIGNAL(displayPictureChanged()), m_manager, SLOT(slotDisplayPictureChanged()));
	}

	return m_manager;
}

const QString &YahooContact::prepareMessage( QString messageText )
{	
	// Yahoo does not understand XML/HTML message data, so send plain text
	// instead.  (Yahoo has its own format for "rich text".)
	QRegExp regExp;
	int pos = 0;
	regExp.setMinimal( true );
	
	// find and replace Bold-formattings
	regExp.setPattern( "<span([^>]*)font-weight:600([^>]*)>(.*)</span>" );
	pos = 0;
	while ( pos >= 0 ) {
		pos = regExp.search( messageText, pos );
		if ( pos >= 0 ) {
			pos += regExp.matchedLength();
		messageText.replace( regExp, QString::fromLatin1("<span\\1font-weight:600\\2>\033[1m\\3\033[x1m</span>" ) );
		}
	}
	
	// find and replace Underline-formattings
	regExp.setPattern( "<span([^>]*)text-decoration:underline([^>]*)>(.*)</span>" );
	pos = 0;
	while ( pos >= 0 ) {
		pos = regExp.search( messageText, pos );
		if ( pos >= 0 ) {
			pos += regExp.matchedLength();
		messageText.replace( regExp, QString::fromLatin1("<span\\1text-decoration:underline\\2>\033[4m\\3\033[x4m</span>" ) );
		}
	}
	
	// find and replace Italic-formattings
	regExp.setPattern( "<span([^>]*)font-style:italic([^>]*)>(.*)</span>" );
	pos = 0;
	while ( pos >= 0 ) {
		pos = regExp.search( messageText, pos );
		if ( pos >= 0 ) {
			pos += regExp.matchedLength();
		messageText.replace( regExp, QString::fromLatin1("<span\\1font-style:italic\\2>\033[2m\\3\033[x2m</span>" ) );
		}
	}
	
	// find and replace Color-formattings
	regExp.setPattern( "<span([^>]*)color:#([0-9a-zA-Z]*)([^>]*)>(.*)</span>" );
	pos = 0;
	while ( pos >= 0 ) {
		pos = regExp.search( messageText, pos );
		if ( pos >= 0 ) {
			pos += regExp.matchedLength();
			messageText.replace( regExp, QString::fromLatin1("<span\\1\\3>\033[#\\2m\\4\033[#000000m</span>" ) );
		}
	}
	
	// find and replace Font-formattings
	regExp.setPattern( "<span([^>]*)font-family:([^;\"]*)([^>]*)>(.*)</span>" );
	pos = 0;
	while ( pos >= 0 ) {
		pos = regExp.search( messageText, pos );
		if ( pos >= 0 ) {
			pos += regExp.matchedLength();
			messageText.replace( regExp, QString::fromLatin1("<span\\1\\3><font face=\"\\2\">\\4</span>" ) );
		}
	}
	
	// find and replace Size-formattings
	regExp.setPattern( "<span([^>]*)font-size:([0-9]*)pt([^>]*)>(.*)</span>" );
	pos = 0;
	while ( pos >= 0 ) {
		pos = regExp.search( messageText, pos );
		if ( pos >= 0 ) {
			pos += regExp.matchedLength();
			messageText.replace( regExp, QString::fromLatin1("<span\\1\\3><font size=\"\\2\">\\4</span>" ) );
		}
	}
	
	// remove span-tags
	regExp.setPattern( "<span([^>]*)>(.*)</span>" );
	pos = 0;
	while ( pos >= 0 ) {
		pos = regExp.search( messageText, pos );
		if ( pos >= 0 ) {
			pos += regExp.matchedLength();
			messageText.replace( regExp, QString::fromLatin1("\\2") );
		}
	}
	
	// convert escaped chars
	messageText.replace( QString::fromLatin1( "&gt;" ), QString::fromLatin1( ">" ) );
	messageText.replace( QString::fromLatin1( "&lt;" ), QString::fromLatin1( "<" ) );
	messageText.replace( QString::fromLatin1( "&quot;" ), QString::fromLatin1( "\"" ) );
	messageText.replace( QString::fromLatin1( "&nbsp;" ), QString::fromLatin1( " " ) );
	messageText.replace( QString::fromLatin1( "&amp;" ), QString::fromLatin1( "&" ) );
	messageText.replace( QString::fromLatin1( "<br/>" ), QString::fromLatin1( "\r" ) );
	
	return messageText;
}

void YahooContact::slotSendMessage( Kopete::Message &message )
{
	kdDebug(YAHOO_GEN_DEBUG) << k_funcinfo << endl;
	
	QString messageText = message.escapedBody();
	kdDebug(YAHOO_GEN_DEBUG) << "Original message: " << messageText << endl;
	messageText = prepareMessage( messageText );
	kdDebug(YAHOO_GEN_DEBUG) << "Converted message: " << messageText << endl;
	
	Kopete::ContactPtrList m_them = manager(Kopete::Contact::CanCreate)->members();
	Kopete::Contact *target = m_them.first();

	m_account->yahooSession()->sendMessage( static_cast<YahooContact *>(target)->m_userId, messageText );
	
	// append message to window
	manager(Kopete::Contact::CanCreate)->appendMessage(message);
	manager(Kopete::Contact::CanCreate)->messageSucceeded();
}

void YahooContact::sendFile( const KURL &sourceURL, const QString &/*fileName*/, uint fileSize )
{
	QString file;
	if( sourceURL.isValid() )
		file = sourceURL.path();
	else
	{
		file = KFileDialog::getOpenFileName( QString::null, "*", 0, i18n("Kopete File Transfer") );
		if( !file.isEmpty() )
		{
			fileSize = QFile( file ).size();
		}
		else
			return;
	}
	
	//m_account->yahooSession()->sendFile( m_userId, QString(), file, fileSize );
}

void YahooContact::slotTyping(bool isTyping_ )
{
	Kopete::ContactPtrList m_them = manager(Kopete::Contact::CanCreate)->members();
	Kopete::Contact *target = m_them.first();


	m_account->yahooSession()->sendTyping( static_cast<YahooContact*>(target)->m_userId, isTyping_ );
}

void YahooContact::slotChatSessionDestroyed()
{
	m_manager = 0L;
}

QList<KAction*> *YahooContact::customContextMenuActions()
{
	QList<KAction*> *actionCollection = new QList<KAction*>();
	if ( !m_webcamAction )
	{
		m_webcamAction = new KAction( i18n( "View &Webcam" ), "camera_unmount", KShortcut(),
		                              this, SLOT( requestWebcam() ), 0, "view_webcam" );
	}
	if ( isReachable() )
		m_webcamAction->setEnabled( true );
	else
		m_webcamAction->setEnabled( false );
	actionCollection->append( m_webcamAction );
	
	if( !m_inviteWebcamAction )
	{
		m_inviteWebcamAction = new KAction( i18n( "Invite to view your Webcam" ), "camera_unmount", KShortcut(),
		                                    this, SLOT( inviteWebcam() ), 0, "invite_webcam" );
	}
	if ( isReachable() )
		m_inviteWebcamAction->setEnabled( true );
	else
		m_inviteWebcamAction->setEnabled( false );
	actionCollection->append( m_inviteWebcamAction );
	
	if ( !m_buzzAction )
	{
		m_buzzAction = new KAction( i18n( "&Buzz Contact" ), "bell", KShortcut(), this, SLOT( buzzContact() ), 0, "buzz_contact");
	}
	if ( isReachable() )
		m_buzzAction->setEnabled( true );
	else
		m_buzzAction->setEnabled( false );
	actionCollection->append( m_buzzAction );

	if ( !m_stealthAction )
	{
		m_stealthAction = new KAction( i18n( "&Stealth Setting" ), "yahoo_stealthed", KShortcut(), this, SLOT( stealthContact() ), 0, "stealth_contact");
	}
	if ( isReachable() )
		m_stealthAction->setEnabled( true );
	else
		m_stealthAction->setEnabled( false );
	actionCollection->append( m_stealthAction );
	
	if ( !m_inviteConferenceAction )
	{
		m_inviteConferenceAction = new KAction( i18n( "&Invite to Conference" ), "kontact_contacts", KShortcut(), this, SLOT( inviteConference() ), 0, "invite_conference");
	}
	if ( isReachable() )
		m_inviteConferenceAction->setEnabled( true );
	else
		m_inviteConferenceAction->setEnabled( false );
	actionCollection->append( m_inviteConferenceAction );
	
	return actionCollection;
	
	//return 0L;
}

void YahooContact::slotUserInfo()
{
	kdDebug(YAHOO_GEN_DEBUG) << k_funcinfo << endl;
	/*if( m_account->yahooSession() )
		m_account->yahooSession()->getUserInfo( m_userId );
	else
		KMessageBox::information( Kopete::UI::Global::mainWidget(), i18n("You need to connect to the service in order to use this feature."),
		                         i18n("Not connected") );*/
}

void YahooContact::slotSendFile()
{
	kdDebug(YAHOO_GEN_DEBUG) << k_funcinfo << endl;
}

void YahooContact::stealthContact()
{
	kdDebug(YAHOO_GEN_DEBUG) << k_funcinfo << endl;

	KDialogBase *stealthSettingDialog = new KDialogBase( Kopete::UI::Global::mainWidget(), "stealthSettingDialog", "true",
				i18n("Stealth Setting"), KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Ok, true );
	YahooStealthSetting *stealthWidget = new YahooStealthSetting( stealthSettingDialog, "stealthSettingWidget" );
	stealthSettingDialog->setMainWidget( stealthWidget );

	if ( stealthSettingDialog->exec() == QDialog::Rejected )
		return;
	
	if ( stealthWidget->radioOnline->isChecked() )
		m_account->yahooSession()->stealthContact( m_userId, Yahoo::NotStealthed );
	else
		m_account->yahooSession()->stealthContact( m_userId, Yahoo::Stealthed );

	stealthSettingDialog->delayedDestruct();
}

void YahooContact::buzzContact()
{
	Kopete::ContactPtrList m_them = manager(Kopete::Contact::CanCreate)->members();
	Kopete::Contact *target = m_them.first();
	
	m_account->yahooSession()->sendBuzz( static_cast<YahooContact*>(target)->m_userId );

	KopeteView *view = manager(Kopete::Contact::CannotCreate)->view(false);
	if ( view )
	{
		Kopete::Message msg = Kopete::Message( manager(Kopete::Contact::CannotCreate)->myself() ,
									manager(Kopete::Contact::CannotCreate)->members(), i18n("Buzzz!!!"),
									Kopete::Message::Internal, Kopete::Message::PlainText );
		view->appendMessage( msg );
	}
}

void YahooContact::sendBuddyIconChecksum( int checksum )
{
	kdDebug(YAHOO_GEN_DEBUG) << k_funcinfo << endl;
	m_account->yahooSession()->sendPictureChecksum( checksum, m_userId );
	
}

void YahooContact::sendBuddyIconInfo( const QString &url, int checksum )
{
	kdDebug(YAHOO_GEN_DEBUG) << k_funcinfo << endl;
	m_account->yahooSession()->sendPictureInformation( m_userId, url, checksum );
}

void YahooContact::sendBuddyIconUpdate( int type )
{
	kdDebug(YAHOO_GEN_DEBUG) << k_funcinfo << endl;
	m_account->yahooSession()->sendPictureStatusUpdate( m_userId, type );
}

void YahooContact::setDisplayPicture(KTempFile *f, int checksum)
{
	kdDebug(YAHOO_GEN_DEBUG) << k_funcinfo << endl;
	if( !f )
		return;
	// stolen from msncontact.cpp ;)
	QString newlocation=locateLocal( "appdata", "yahoopictures/"+ contactId().lower().replace(QRegExp("[./~]"),"-")  +".png"  ) ;
	setProperty( YahooProtocol::protocol()->iconCheckSum, checksum );
	
	KIO::Job *j=KIO::file_move( KURL::fromPathOrURL( f->name() ) , KURL::fromPathOrURL( newlocation ) , -1, true /*overwrite*/ , false /*resume*/ , false /*showProgressInfo*/ );
	
	f->setAutoDelete(false);
	delete f;
	
	//let the time to KIO to copy the file
	connect(j, SIGNAL(result(KIO::Job *)) , this, SLOT(slotEmitDisplayPictureChanged() ));
}

void YahooContact::slotEmitDisplayPictureChanged()
{
	kdDebug(YAHOO_GEN_DEBUG) << k_funcinfo << endl;
	QString newlocation=locateLocal( "appdata", "yahoopictures/"+ contactId().lower().replace(QRegExp("[./~]"),"-")  +".png"  ) ;
	setProperty( Kopete::Global::Properties::self()->photo(), QString() );
	setProperty( Kopete::Global::Properties::self()->photo() , newlocation );
	emit displayPictureChanged();
}

void YahooContact::inviteConference()
{
	m_account->prepareConference( m_userId );
}

void YahooContact::inviteWebcam()
{
	m_account->yahooSession()->sendWebcamInvite( m_userId );
}

void YahooContact::receivedWebcamImage( const QPixmap& image )
{
	if( !m_webcamDialog )
		initWebcamViewer();
	m_receivingWebcam = true;
	emit signalReceivedWebcamImage( image );
}

void YahooContact::webcamClosed( int reason )
{
	m_receivingWebcam = false;
	emit signalWebcamClosed( reason );
}

void YahooContact::webcamPaused()
{
	emit signalWebcamPaused();
}

void YahooContact::initWebcamViewer()
{
	//KImageIO::registerFormats();
	
	if ( !m_webcamDialog )
	{
		m_webcamDialog = new YahooWebcamDialog( userId(), Kopete::UI::Global::mainWidget() );
// 		QObject::connect( m_webcamDialog, SIGNAL( closeClicked() ), this, SLOT( closeWebcamDialog() ) );
	
		QObject::connect( this, SIGNAL( signalWebcamClosed( int ) ),
		                  m_webcamDialog, SLOT( webcamClosed( int ) ) );
		
		QObject::connect( this, SIGNAL( signalWebcamPaused() ),
		                  m_webcamDialog, SLOT( webcamPaused() ) );
		
		QObject::connect( this, SIGNAL ( signalReceivedWebcamImage( const QPixmap& ) ),
				m_webcamDialog, SLOT( newImage( const QPixmap& ) ) );
		
		QObject::connect( m_webcamDialog, SIGNAL ( closingWebcamDialog ( ) ),
				this, SLOT ( closeWebcamDialog ( ) ) );
	}
	m_webcamDialog->show();
}

void YahooContact::requestWebcam()
{
	if ( KStandardDirs::findExe("jasper").isEmpty() )
	{
		KMessageBox::queuedMessageBox(                            		Kopete::UI::Global::mainWidget(),
			KMessageBox::Error, i18n("I cannot find the jasper image convert program.\njasper is required to render the yahoo webcam images.\nPlease go to http://kopete.kde.org/yahoo/jasper.html for instructions.")
                );
		return;
	}
	
	if( !m_webcamDialog )
		initWebcamViewer();
	m_account->yahooSession()->requestWebcam( contactId() );
}

void YahooContact::closeWebcamDialog()
{
	QObject::disconnect( this, SIGNAL( signalWebcamClosed( int ) ),
	                  m_webcamDialog, SLOT( webcamClosed( int ) ) );
	
	QObject::disconnect( this, SIGNAL( signalWebcamPaused() ),
	                  m_webcamDialog, SLOT( webcamPaused( int ) ) );
	
	QObject::disconnect( this, SIGNAL ( signalReceivedWebcamImage( const QPixmap& ) ),
	                  m_webcamDialog, SLOT( newImage( const QPixmap& ) ) );
	
	QObject::disconnect( m_webcamDialog, SIGNAL ( closingWebcamDialog ( ) ),
	                  this, SLOT ( closeWebcamDialog ( ) ) );
	if( m_receivingWebcam )
		m_account->yahooSession()->closeWebcam( contactId() );
	m_webcamDialog->delayedDestruct();
	m_webcamDialog = 0L;
}

void YahooContact::deleteContact()
{
	kdDebug(YAHOO_GEN_DEBUG) << k_funcinfo << endl;
	
	if( !m_account->isOnServer( contactId() ) )
	{
		kdDebug(YAHOO_GEN_DEBUG) << k_funcinfo << "Contact does not exist on server-side. Not removing..." << endl;		
	}
	else
	{
		kdDebug(YAHOO_GEN_DEBUG) << k_funcinfo << "Contact is getting remove from server side contactlist...." << endl;
		m_account->yahooSession()->removeBuddy( contactId(), m_groupName );
	}
	Kopete::Contact::deleteContact();
}
#include "yahoocontact.moc"

// vim: set noet ts=4 sts=4 sw=4:
//kate: space-indent off; replace-tabs off; indent-mode csands;

