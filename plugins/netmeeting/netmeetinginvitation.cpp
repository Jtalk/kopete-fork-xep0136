/*
    msninvitation.cpp

    Copyright (c) 2003 by Olivier Goffart        <ogoffart@tiscalinet.be>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "netmeetinginvitation.h"


#include "msnmessagemanager.h"
#include "msnswitchboardsocket.h"
#include "msncontact.h"

#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>

#include <qtimer.h>
#include <qprocess.h>

NetMeetingInvitation::NetMeetingInvitation(bool incomming, MSNContact *c, QObject *parent)
 : QObject(parent) , MSNInvitation( incomming, NetMeetingInvitation::applicationID() , i18n("NetMeeting") )
{
	m_contact=c;
	oki=false;
}


NetMeetingInvitation::~NetMeetingInvitation()
{
}


QString NetMeetingInvitation::invitationHead()
{
	QTimer::singleShot( 10*60000, this, SLOT( slotTimeout() ) ); //send TIMEOUT in 10 minute if the invitation has not been accepted/refused
	return QString( MSNInvitation::invitationHead()+
				"Session-Protocol: SM1\r\n"
  				"Session-ID: {6672F94C-45BF-11D7-B4AE-00010A1008DF}\r\n" //FIXME i dont know what is the session id
				"\r\n").utf8();
}

void NetMeetingInvitation::parseInvitation(const QString& msg)
{
	QRegExp rx("Invitation-Command: ([A-Z]*)");
	rx.search(msg);
	QString command=rx.cap(1);
	if( msg.contains("Invitation-Command: INVITE") )
	{
		MSNInvitation::parseInvitation(msg); //for the cookie

		unsigned int result = KMessageBox::questionYesNo( 0,
					i18n("%1 want to start a chat with Gnome-meeting. Do you want to accept it? " ).arg(m_contact->displayName()),
					i18n("MSN Plugin - Kopete") , i18n("Accept"),i18n("Refuse"));

		MSNMessageManager* manager=dynamic_cast<MSNMessageManager*>(m_contact->manager());

		if(manager && manager->service())
		{
			if(result==3) // Yes == 3
			{
				QCString message=QString(
					"MIME-Version: 1.0\r\n"
					"Content-Type: text/x-msmsgsinvite; charset=UTF-8\r\n"
					"\r\n"
					"Invitation-Command: ACCEPT\r\n"
					"Invitation-Cookie: " + QString::number(cookie()) + "\r\n"
					"Session-ID: {6672F94C-45BF-11D7-B4AE-00010A1008DF}\r\n" //FIXME
					"Session-Protocol: SM1\r\n"
					"Launch-Application: TRUE\r\n"
					"Request-Data: IP-Address:\r\n"
					"IP-Address: " + manager->service()->getLocalIP()+ "\r\n"
					"\r\n" ).utf8();


				manager->service()->sendCommand( "MSG" , "N", true, message );
				oki=false;
				QTimer::singleShot( 10* 60000, this, SLOT( slotTimeout() ) ); //TIMOUT afte 10 min
			}
			else //No
			{
				manager->service()->sendCommand( "MSG" , "N", true, rejectMessage() );
				emit done(this);
			}
		}
	}
	else if( msg.contains("Invitation-Command: ACCEPT") )
	{
		if( ! incoming() )
		{
			MSNMessageManager* manager=dynamic_cast<MSNMessageManager*>(m_contact->manager());
			if(manager && manager->service())
			{
				QCString message=QString(
					"MIME-Version: 1.0\r\n"
					"Content-Type: text/x-msmsgsinvite; charset=UTF-8\r\n"
					"\r\n"
					"Invitation-Command: ACCEPT\r\n"
					"Invitation-Cookie: " + QString::number(cookie()) + "\r\n"
					"Session-ID: {6672F94C-45BF-11D7-B4AE-00010A1008DF}\r\n" //FIXME: what is session id?
					"Session-Protocol: SM1\r\n"
					"Launch-Application: TRUE\r\n"
					"Request-Data: IP-Address:\r\n"
					"IP-Address: " + manager->service()->getLocalIP() + "\r\n"
					"\r\n" ).utf8();
				manager->service()->sendCommand( "MSG" , "N", true, message );
			}
			rx=QRegExp("IP-Address: ([0-9\\:\\.]*)");
			rx.search(msg);
			QString ip_adress = rx.cap(1);
	    	startMeeting(ip_adress);
			kdDebug() << k_funcinfo << ip_adress << endl ;
		}
		else
		{
			rx=QRegExp("IP-Address: ([0-9\\:\\.]*)");
			rx.search(msg);
			QString ip_adress = rx.cap(1);

			startMeeting(ip_adress);
		}
	}
	else //CANCEL
	{
		emit done(this);
	}
}

void NetMeetingInvitation::slotTimeout()
{
	if(oki)
		return;

 	MSNMessageManager* manager=dynamic_cast<MSNMessageManager*>(m_contact->manager());

	if(manager && manager->service())
	{
		manager->service()->sendCommand( "MSG" , "N", true, rejectMessage("TIMEOUT") );
	}
	emit done(this);

}


void NetMeetingInvitation::startMeeting(const QString & ip_adress)
{
	kdDebug() << k_funcinfo << endl ;

	//TODO: use KProcess

  QProcess process;
  process.addArgument("gnomemeeting");
  process.addArgument("-c");
  process.addArgument(  "callto://" + ip_adress );
  process.start();
}

#include "netmeetinginvitation.moc"




