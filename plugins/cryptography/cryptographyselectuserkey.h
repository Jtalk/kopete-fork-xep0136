/***************************************************************************
                          cryptographyselectuserkey.h  -  description
                             -------------------
    begin                : dim nov 17 2002
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

#ifndef CRYPTOGRAPHYSELECTUSERKEY_H
#define CRYPTOGRAPHYSELECTUSERKEY_H

#include <kdialogbase.h>

class KopeteMetaContact;
class CryptographyUserKey_ui;

/**
  *@author OlivierGoffart
  */

class CryptographySelectUserKey : public KDialogBase {
	Q_OBJECT
public: 
	CryptographySelectUserKey(const QString &key, KopeteMetaContact *mc);
	~CryptographySelectUserKey();

	
  QString publicKey();

private slots:
	void keySelected(QString &,bool,bool,bool,bool);
	void slotSelectPressed();
  /** No descriptions */
  void slotRemovePressed();

private:
	CryptographyUserKey_ui *view;
	KopeteMetaContact *m_metaContact;

};

#endif
