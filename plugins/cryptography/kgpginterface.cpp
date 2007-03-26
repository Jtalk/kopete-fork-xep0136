#include "cryptographyplugin.h"  //(for the cached passphrase)
//Code from KGPG

/***************************************************************************
                          kgpginterface.cpp  -  description
                             -------------------
    begin                : Mon Jul 8 2002
    copyright            : (C) 2002 by y0k0
    email                : bj@altern.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include <klocale.h>
#include <KDE/KPasswordDialog>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <qfile.h>
//Added by qt3to4:
#include <QByteArray>

#include <k3procio.h>

//#include "kdetailedconsole.h"

#include "kgpginterface.h"

KgpgInterface::KgpgInterface()
{}

KgpgInterface::~KgpgInterface()
{}

QString KgpgInterface::KgpgEncryptText(QString text,QString userIDs, QString Options)
{
	FILE *fp;
	QString dests,encResult;
	char buffer[200];
	
	userIDs=userIDs.trimmed();
	userIDs=userIDs.simplified();
	Options=Options.trimmed();
	
	int ct=userIDs.indexOf(" ");
	while (ct!=-1)  // if multiple keys...
	{
		dests+=" --recipient "+userIDs.section(' ',0,0);
		userIDs.remove(0,ct+1);
		ct=userIDs.indexOf(" ");
	}
	dests+=" --recipient "+userIDs;
	
	QByteArray gpgcmd = "echo -n ";
	gpgcmd += K3ShellProcess::quote( text ).toUtf8();
	gpgcmd += " | gpg --no-secmem-warning --no-tty ";
	gpgcmd += Options.toLocal8Bit();
	gpgcmd += " -e ";
	gpgcmd += dests.toLocal8Bit();
	
	//////////   encode with untrusted keys or armor if checked by user
	fp = popen( gpgcmd, "r");
	while ( fgets( buffer, sizeof(buffer), fp))
		encResult+=buffer;
	pclose(fp);
	
	if( !encResult.isEmpty() )
		return encResult;
	else
		return QString();
}

QString KgpgInterface::KgpgDecryptText(QString text,QString userID)
{
	FILE *fp,*pass;
	QString encResult;
	
	char buffer[200];
	int counter=0,ppass[2];
	QByteArray password = CryptographyPlugin::cachedPass();
	bool passphraseHandling=CryptographyPlugin::passphraseHandling();
	
	while ((counter<3) && (encResult.isEmpty()))
	{
		counter++;
		if(passphraseHandling && password.isNull())
		{
			/// pipe for passphrase
			//userID=QString::fromUtf8(userID);
			userID.replace('<',"&lt;");
			QString passdlg=i18n("Enter passphrase for <b>%1</b>:", userID);
			if (counter>1)
				passdlg.prepend(i18n("<b>Bad passphrase</b><br> You have %1 tries left.<br>", 4-counter));
	
			/// pipe for passphrase
			KPasswordDialog dlg( 0L, KPasswordDialog::NoFlags );
			dlg.setPrompt( passdlg );
			if( !dlg.exec() )
				return QString(); //the user canceled
			CryptographyPlugin::setCachedPass(dlg.password().toLocal8Bit());
		}
	
		if(passphraseHandling)
		{
			pipe(ppass);
			pass = fdopen(ppass[1], "w");
			fwrite(password, sizeof(char), strlen(password), pass);
			//        fwrite("\n", sizeof(char), 1, pass);
			fclose(pass);
		}
	
		QByteArray gpgcmd="echo ";
		gpgcmd += K3ShellProcess::quote(text).toUtf8();
		gpgcmd += " | gpg --no-secmem-warning --no-tty ";
		if(passphraseHandling)
			gpgcmd += "--passphrase-fd " + QString::number(ppass[0]).toLocal8Bit();
		gpgcmd += " -d ";
		
		//////////   encode with untrusted keys or armor if checked by user
		fp = popen(gpgcmd, "r");
		while ( fgets( buffer, sizeof(buffer), fp))
			encResult += QString::fromUtf8(buffer);
		
		pclose(fp);
		password = QByteArray();
	}
	
	if( !encResult.isEmpty() )
		return encResult;
	else
		return QString();
}

QString KgpgInterface::checkForUtf8(QString txt)
{

        //    code borrowed from gpa
        const char *s;

        /* Make sure the encoding is UTF-8.
         * Test structure suggested by Werner Koch */
        if (txt.isEmpty())
                return QString();

        for (s = txt.toAscii(); *s && !(*s & 0x80); s++)
                ;
        if (*s && !strchr (txt.toAscii(), 0xc3) && (txt.indexOf("\\x")==-1))
                return txt;

        /* The string is not in UTF-8 */
        //if (strchr (txt.toAscii(), 0xc3)) return (txt+" +++");
        if (txt.indexOf("\\x")==-1)
                return QString::fromUtf8(txt.toAscii());
        //        if (!strchr (txt.toAscii(), 0xc3) || (txt.find("\\x")!=-1)) {
        for ( int idx = 0 ; (idx = txt.indexOf( "\\x", idx )) >= 0 ; ++idx ) {
                char str[2] = "x";
                str[0] = (char) QString( txt.mid( idx + 2, 2 ) ).toShort( 0, 16 );
                txt.replace( idx, 4, str );
        }
	if (!strchr (txt.toAscii(), 0xc3))
                return QString::fromUtf8(txt.toAscii());
        else
                return QString::fromUtf8(QString::fromUtf8(txt.toAscii()).toAscii());  // perform Utf8 twice, or some keys display badly
}




#include "kgpginterface.moc"
