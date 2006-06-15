/*
 Kopete Oscar Protocol
 icquserinfowidget.h - Display ICQ user info

 Copyright (c) 2005 Matt Rogers <mattr@kde.org>

 Kopete (c) 2002-2005 by the Kopete developers <kopete-devel@kde.org>

 *************************************************************************
 *                                                                       *
 * This library is free software; you can redistribute it and/or         *
 * modify it under the terms of the GNU Lesser General Public            *
 * License as published by the Free Software Foundation; either          *
 * version 2 of the License, or (at your option) any later version.      *
 *                                                                       *
 *************************************************************************
*/

#ifndef _ICQUSERINFOWIDGET_H_
#define _ICQUSERINFOWIDGET_H_

#include <kpagedialog.h>
#include <icquserinfo.h>

namespace Ui
{
	class ICQGeneralInfoWidget;
	class ICQWorkInfoWidget;
	class ICQOtherInfoWidget;
	class ICQInterestInfoWidget;
}
class ICQContact;

class ICQUserInfoWidget : public KPageDialog
{
Q_OBJECT
public:
	ICQUserInfoWidget( QWidget* parent = 0 );
	~ICQUserInfoWidget();
	void setContact( ICQContact* contact );
	
public slots:
	void fillBasicInfo( const ICQGeneralUserInfo& );
	void fillWorkInfo( const ICQWorkUserInfo& );
	void fillEmailInfo( const ICQEmailInfo& );
	void fillMoreInfo( const ICQMoreUserInfo& );
	void fillInterestInfo( const ICQInterestInfo& );

private:
	Ui::ICQGeneralInfoWidget* m_genInfoWidget;
	Ui::ICQWorkInfoWidget* m_workInfoWidget;
	Ui::ICQOtherInfoWidget* m_otherInfoWidget;
	Ui::ICQInterestInfoWidget * m_interestInfoWidget;
	ICQContact* m_contact;
};

#endif

//kate: indent-mode csands; tab-width 4; space-indent off; replace-tabs off;
