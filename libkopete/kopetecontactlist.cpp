/*
    kopetecontactlist.cpp - Kopete's Contact List backend

    Copyright (c) 2002-2003 by Martijn Klingens       <klingens@kde.org>
    Copyright (c) 2002-2003 by Olivier Goffart        <ogoffart@tiscalinet.be>
    Copyright (c) 2002      by Duncan Mac-Vicar Prett <duncan@kde.org>

    Copyright (c) 2002      by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include "kopetecontactlist.h"

#include <qdir.h>
#include <qregexp.h>

#include <kapplication.h>
#include <kdebug.h>
#include <ksavefile.h>
#include <kstandarddirs.h>
#include "kopetemetacontact.h"
#include "kopeteprotocol.h"
#include "kopeteaccount.h"
#include "kopeteaccountmanager.h"

KopeteContactList *KopeteContactList::s_contactList = 0L;

KopeteContactList *KopeteContactList::contactList()
{
	if( !s_contactList )
		s_contactList = new KopeteContactList;

	return s_contactList;
}

KopeteContactList::KopeteContactList()
: QObject( kapp, "KopeteContactList" )
{
	//no contactlist loaded yet, don't save them
	m_loaded=false;
}

KopeteContactList::~KopeteContactList()
{
// save is currently called in ~kopete (before the deletion of plugins)
//	save();
}

KopeteMetaContact *KopeteContactList::findContact( const QString &protocolId,
	const QString &accountId, const QString &contactId )
{
	KopeteAccount *i=KopeteAccountManager::manager()->findAccount(protocolId,accountId);
	if(!i)
	{
		kdDebug( 14010 ) << k_funcinfo << "Account not found" << endl;
		return 0L;
	}
	KopeteContact *c=i->contacts()[contactId];
	if(c)
		return c->metaContact();

	/*QPtrListIterator<KopeteMetaContact> it( m_contacts );
	for( ; it.current(); ++it )
	{
		if( it.current()->findContact( protocolId, accountId, contactId ) )
			return it.current();
	}*/
	return 0L;
}

void KopeteContactList::addMetaContact( KopeteMetaContact *mc )
{
	m_contacts.append( mc );

/*	connect( mc,
		SIGNAL( removedFromGroup( KopeteMetaContact *, const QString & ) ),
		SLOT( slotRemovedFromGroup( KopeteMetaContact *, const QString & ) ) );*/

	emit metaContactAdded( mc );
}

/*void KopeteContactList::slotRemovedFromGroup( KopeteMetaContact *mc,
	const QString &  )
{
	if( mc->groups().isEmpty() )
	{
		kdDebug(14010) << "KopeteContactList::slotRemovedFromGroup: "
			<< "contact removed from all groups: now toplevel." << endl;
		//m_contacts.remove( mc );
		//mc->deleteLater();
	}
}                  */

void KopeteContactList::loadXML()
{
	addGroup( KopeteGroup::toplevel );

	QString filename = locateLocal( "appdata", QString::fromLatin1( "contactlist.xml" ) );
	if( filename.isEmpty() )
	{
		m_loaded=true;
		return ;
	}

	QDomDocument contactList( QString::fromLatin1( "kopete-contact-list" ) );

	QFile contactListFile( filename );
	contactListFile.open( IO_ReadOnly );
	contactList.setContent( &contactListFile );

	QDomElement list = contactList.documentElement();

	QString versionString = list.attribute( QString::fromLatin1( "version" ), QString::null );
	uint version = 0;
	if( QRegExp( QString::fromLatin1( "[0-9]+\\.[0-9]" ) ).exactMatch( versionString ) )
		version = versionString.replace( QRegExp( QString::fromLatin1( "\\." ) ), QString::null ).toUInt();

	if( version < ContactListVersion )
	{
		// The version string is invalid, or we're using an older version.
		// Convert first and reparse the file afterwards
		kdDebug( 14010 ) << k_funcinfo << "Contact list version " << version
			<< " is older than current version " << ContactListVersion
			<< ". Converting first." << endl;

		contactListFile.close();

		convertContactList( filename, version, ContactListVersion );

		contactList = QDomDocument ( QString::fromLatin1( "kopete-contact-list" ) );

		contactListFile.open( IO_ReadOnly );
		contactList.setContent( &contactListFile );

		list = contactList.documentElement();
	}

	QDomElement element = list.firstChild().toElement();
	while( !element.isNull() )
	{
		if( element.tagName() == QString::fromLatin1("meta-contact") )
		{
			//TODO: id isn't used
			//QString id = element.attribute( "id", QString::null );
			KopeteMetaContact *metaContact = new KopeteMetaContact();
			if ( !metaContact->fromXML( element ) )
			{
				delete metaContact;
				metaContact = 0;
			}
			else
			{
				KopeteContactList::contactList()->addMetaContact(
					metaContact );
			}
		}
		else if( element.tagName() == QString::fromLatin1("kopete-group") )
		{
			KopeteGroup *group = new KopeteGroup();
			if( !group->fromXML( element ) )
			{
				delete group;
				group = 0;
			}
			else
			{
				KopeteContactList::contactList()->addGroup( group );
			}
		}
		else
		{
			kdWarning(14010) << "KopeteContactList::loadXML: "
				<< "Unknown element '" << element.tagName()
				<< "' in contact list!" << endl;
		}
		element = element.nextSibling().toElement();
	}
	contactListFile.close();
	m_loaded=true;
}

void KopeteContactList::convertContactList( const QString &fileName, uint /* fromVersion */, uint /* toVersion */ )
{
	// For now, ignore fromVersion and toVersion. These are meant for future
	// changes to allow incremental (multi-pass) conversion so we don't have
	// to rewrite the whole conversion code for each change.

	QDomDocument contactList( QString::fromLatin1( "messaging-contact-list" ) );
	QFile contactListFile( fileName );
	contactListFile.open( IO_ReadOnly );
	contactList.setContent( &contactListFile );

	QDomElement oldList = contactList.documentElement();

	QDomDocument newList( QString::fromLatin1( "kopete-contact-list" ) );
	newList.appendChild( newList.createProcessingInstruction( QString::fromLatin1( "xml" ), QString::fromLatin1( "version=\"1.0\"" ) ) );

	QDomElement newRoot = newList.createElement( QString::fromLatin1( "kopete-contact-list" ) );
	newList.appendChild( newRoot );
	newRoot.setAttribute( QString::fromLatin1( "version" ), QString::fromLatin1( "1.0" ) );

	QDomNode oldNode = oldList.firstChild();
	while( !oldNode.isNull() )
	{
		QDomElement oldElement = oldNode.toElement();
		if( !oldElement.isNull() )
		{
			if( oldElement.tagName() == QString::fromLatin1("meta-contact") )
			{
				// Ignore ID, it is not used in the current list anyway
				QDomElement newMetaContact = newList.createElement( QString::fromLatin1( "meta-contact" ) );
				newRoot.appendChild( newMetaContact );

				// Plugin data is stored completely different, and requires
				// some bookkeeping to convert properly
				QMap<QString, QDomElement> pluginData;
				QStringList icqData;
				QStringList gaduData;

				// ICQ and Gadu can only be converted properly if the address book fields
				// are already parsed. Therefore, scan for those first and add the rest
				// afterwards
				QDomNode oldContactNode = oldNode.firstChild();
				while( !oldContactNode.isNull() )
				{
					QDomElement oldContactElement = oldContactNode.toElement();
					if( !oldContactElement.isNull() && oldContactElement.tagName() == QString::fromLatin1("address-book-field") )
					{
						// Convert address book fields.
						// Jabber will be called "xmpp", Aim/Toc and Aim/Oscar both will
						// be called "aim". MSN, AIM, IRC, Oscar and SMS don't use address
						// book fields yet; Gadu and ICQ can be converted as-is.
						// As Yahoo is unfinished we won't try to convert at all.
						QString id   = oldContactElement.attribute( QString::fromLatin1( "id" ), QString::null );
						QString data = oldContactElement.text();

						QString app, key, val;
						QString separator = QString::fromLatin1( "," );
						if( id == QString::fromLatin1( "messaging/gadu" ) )
							separator = QString::fromLatin1( "\n" );
						else if( id == QString::fromLatin1( "messaging/icq" ) )
							separator = QString::fromLatin1( ";" );
						else if( id == QString::fromLatin1( "messaging/jabber" ) )
							id = QString::fromLatin1( "messaging/xmpp" );

						if( id == QString::fromLatin1("messaging/gadu") || id == QString::fromLatin1("messaging/icq") ||
							id == QString::fromLatin1("messaging/winpopup") || id == QString::fromLatin1("messaging/xmpp") )
						{
							app = id;
							key = QString::fromLatin1("All");
							val = data.replace( QRegExp( separator ), QChar( 0xE000 ) );
						}

						if( !app.isEmpty() )
						{
							QDomElement addressBookField = newList.createElement( QString::fromLatin1( "address-book-field" ) );
							newMetaContact.appendChild( addressBookField );

							addressBookField.setAttribute( QString::fromLatin1( "app" ), app );
							addressBookField.setAttribute( QString::fromLatin1( "key" ), key );

							addressBookField.appendChild( newList.createTextNode( val ) );

							// ICQ didn't store the contactId locally, only in the address
							// book fields, so we need to be able to access it later
							if( id == QString::fromLatin1( "messaging/icq" ) )
								icqData = QStringList::split( QChar( 0xE000 ), val );
							else if( id == QString::fromLatin1("messaging/gadu") )
								gaduData = QStringList::split( QChar( 0xE000 ), val );
						}
					}
					oldContactNode = oldContactNode.nextSibling();
				}

				// Now, convert the other elements
				oldContactNode = oldNode.firstChild();
				while( !oldContactNode.isNull() )
				{
					QDomElement oldContactElement = oldContactNode.toElement();
					if( !oldContactElement.isNull() )
					{
						if( oldContactElement.tagName() == QString::fromLatin1("display-name") )
						{
							QDomElement displayName = newList.createElement( QString::fromLatin1( "display-name" ) );
							displayName.appendChild( newList.createTextNode( oldContactElement.text() ) );
							newMetaContact.appendChild( displayName );
						}
						else if( oldContactElement.tagName() == QString::fromLatin1("groups") )
						{
							QDomElement groups = newList.createElement( QString::fromLatin1( "groups" ) );
							newMetaContact.appendChild( groups );

							QDomNode oldGroup = oldContactElement.firstChild();
							while( !oldGroup.isNull() )
							{
								QDomElement oldGroupElement = oldGroup.toElement();
								if ( oldGroupElement.tagName() == QString::fromLatin1("group") )
								{
									QDomElement group = newList.createElement( QString::fromLatin1( "group" ) );
									group.appendChild( newList.createTextNode( oldGroupElement.text() ) );
									groups.appendChild( group );
								}
								else if ( oldGroupElement.tagName() == QString::fromLatin1("top-level") )
								{
									QDomElement group = newList.createElement( QString::fromLatin1( "top-level" ) );
									groups.appendChild( group );
								}

								oldGroup = oldGroup.nextSibling();
							}
						}
						else if( oldContactElement.tagName() == QString::fromLatin1( "plugin-data" ) )
						{
							// Convert the plugin data
							QString id   = oldContactElement.attribute( QString::fromLatin1( "plugin-id" ), QString::null );
							QString data = oldContactElement.text();

							bool convertOldAim = false;
							uint fieldCount = 1;
							QString addressBookLabel;
							if( id == QString::fromLatin1("MSNProtocol") )
							{
								fieldCount = 3;
								addressBookLabel = "msn";
							}
							else if( id == QString::fromLatin1("IRCProtocol") )
							{
								fieldCount = 3;
								addressBookLabel = "irc";
							}
							else if( id == QString::fromLatin1("OscarProtocol") )
							{
								fieldCount = 2;
								addressBookLabel = "aim";
							}
							else if( id == QString::fromLatin1("AIMProtocol") )
							{
								id = QString::fromLatin1("OscarProtocol");
								convertOldAim = true;
								addressBookLabel = "aim";
							}
							else if( id == QString::fromLatin1("ICQProtocol") || id == QString::fromLatin1("WPProtocol") || id == QString::fromLatin1("GaduProtocol") )
							{
								fieldCount = 1;
							}
							else if( id == QString::fromLatin1("JabberProtocol") )
							{
								fieldCount = 4;
							}
							else if( id == QString::fromLatin1("SMSProtocol") )
							{
								// SMS used a variable serializing using a dot as delimiter.
								// The minimal count is three though (id, name, delimiter).
								fieldCount = 2;
								addressBookLabel = QString::fromLatin1("sms");
							}

							if( pluginData[ id ].isNull() )
							{
								pluginData[ id ] = newList.createElement( QString::fromLatin1( "plugin-data" ) );
								pluginData[ id ].setAttribute( QString::fromLatin1( "plugin-id" ), id );
								newMetaContact.appendChild( pluginData[ id ] );
							}

							// Do the actual conversion
							if( id == QString::fromLatin1("MSNProtocol") || id == QString::fromLatin1("OscarProtocol") || id == QString::fromLatin1("AIMProtocol") || id == QString::fromLatin1("IRCProtocol") ||
								id == QString::fromLatin1("ICQProtocol") || id == QString::fromLatin1("JabberProtocol") || id == QString::fromLatin1("SMSProtocol") || id == QString::fromLatin1("WPProtocol") ||
								id == QString::fromLatin1("GaduProtocol") )
							{
								QStringList strList = QStringList::split( QString::fromLatin1( "||" ), data );
								// Unescape '||'
								for( QStringList::iterator it = strList.begin(); it != strList.end(); ++it )
									( *it ).replace( QRegExp( QString::fromLatin1( "\\\\\\|;" ) ),
									QString::fromLatin1( "|" ) ).replace( QRegExp( QString::fromLatin1( "\\\\\\\\" ) ),
									QString::fromLatin1( "\\" ) );

								uint idx = 0;
								while( idx < strList.size() )
								{
									QDomElement dataField;

									dataField = newList.createElement( QString::fromLatin1( "plugin-data-field" ) );
									pluginData[ id ].appendChild( dataField );
									dataField.setAttribute( QString::fromLatin1( "key" ), QString::fromLatin1( "contactId" ) );
									if( id == QString::fromLatin1("ICQProtocol") )
										dataField.appendChild( newList.createTextNode( icqData[ idx ] ) );
									else if( id == QString::fromLatin1("GaduProtocol") )
										dataField.appendChild( newList.createTextNode( gaduData[ idx ] ) );
									else if( id == QString::fromLatin1("JabberProtocol") )
										dataField.appendChild( newList.createTextNode( strList[ idx + 1 ] ) );
									else
										dataField.appendChild( newList.createTextNode( strList[ idx ] ) );

									dataField = newList.createElement( QString::fromLatin1( "plugin-data-field" ) );
									pluginData[ id ].appendChild( dataField );
									dataField.setAttribute( QString::fromLatin1( "key" ), QString::fromLatin1( "displayName" ) );
									if( convertOldAim || id == QString::fromLatin1("ICQProtocol") || id == QString::fromLatin1("WPProtocol") || id == QString::fromLatin1("GaduProtocol") )
										dataField.appendChild( newList.createTextNode( strList[ idx ] ) );
									else if( id == QString::fromLatin1("JabberProtocol") )
										dataField.appendChild( newList.createTextNode( strList[ idx + 2 ] ) );
									else
										dataField.appendChild( newList.createTextNode( strList[ idx + 1 ] ) );

									if( id == QString::fromLatin1("MSNProtocol") )
									{
										dataField = newList.createElement( QString::fromLatin1( "plugin-data-field" ) );
										pluginData[ id ].appendChild( dataField );
										dataField.setAttribute( QString::fromLatin1( "key" ), QString::fromLatin1( "groups" ) );
										dataField.appendChild( newList.createTextNode( strList[ idx + 2 ] ) );
									}
									else if( id == QString::fromLatin1("IRCProtocol") )
									{
										dataField = newList.createElement( QString::fromLatin1( "plugin-data-field" ) );
										pluginData[ id ].appendChild( dataField );
										dataField.setAttribute( QString::fromLatin1( "key" ), QString::fromLatin1( "serverName" ) );
										dataField.appendChild( newList.createTextNode( strList[ idx + 2 ] ) );
									}
									else if( id == QString::fromLatin1("JabberProtocol") )
									{
										dataField = newList.createElement( QString::fromLatin1( "plugin-data-field" ) );
										pluginData[ id ].appendChild( dataField );
										dataField.setAttribute( QString::fromLatin1( "key" ), QString::fromLatin1( "accountId" ) );
										dataField.appendChild( newList.createTextNode( strList[ idx ] ) );

										dataField = newList.createElement( QString::fromLatin1( "plugin-data-field" ) );
										pluginData[ id ].appendChild( dataField );
										dataField.setAttribute( QString::fromLatin1( "key" ), QString::fromLatin1( "groups" ) );
										dataField.appendChild( newList.createTextNode( strList[ idx + 3 ] ) );
									}
									else if( id == QString::fromLatin1( "SMSProtocol" ) &&
										( idx + 2 < strList.size() ) && strList[ idx + 2 ] != QString::fromLatin1( "." ) )
									{
										dataField = newList.createElement( QString::fromLatin1( "plugin-data-field" ) );
										pluginData[ id ].appendChild( dataField );
										dataField.setAttribute( QString::fromLatin1( "key" ), QString::fromLatin1( "serviceName" ) );
										dataField.appendChild( newList.createTextNode( strList[ idx + 2 ] ) );

										dataField = newList.createElement( QString::fromLatin1( "plugin-data-field" ) );
										pluginData[ id ].appendChild( dataField );
										dataField.setAttribute( QString::fromLatin1( "key" ), QString::fromLatin1( "servicePrefs" ) );
										dataField.appendChild( newList.createTextNode( strList[ idx + 3 ] ) );

										// Add extra fields
										idx += 2;
									}

									// MSN, AIM, IRC, Oscar and SMS didn't store address book fields up
									// to now, so create one
									if( id != QString::fromLatin1("ICQProtocol") && id != QString::fromLatin1("JabberProtocol") && id != QString::fromLatin1("WPProtocol") && id != QString::fromLatin1("GaduProtocol") )
									{
										QDomElement addressBookField = newList.createElement( QString::fromLatin1( "address-book-field" ) );
										newMetaContact.appendChild( addressBookField );

										addressBookField.setAttribute( QString::fromLatin1( "app" ),
											QString::fromLatin1( "messaging/" ) + addressBookLabel );
										addressBookField.setAttribute( QString::fromLatin1( "key" ), QString::fromLatin1( "All" ) );
										addressBookField.appendChild( newList.createTextNode( strList[ idx ] ) );
									}

									idx += fieldCount;
								}
							}
							else if( id == QString::fromLatin1("ContactNotesPlugin") || id == QString::fromLatin1("CryptographyPlugin") || id == QString::fromLatin1("TranslatorPlugin") )
							{
								QDomElement dataField = newList.createElement( QString::fromLatin1( "plugin-data-field" ) );
								pluginData[ id ].appendChild( dataField );
								if( id == QString::fromLatin1("ContactNotesPlugin") )
									dataField.setAttribute( QString::fromLatin1( "key" ), QString::fromLatin1( "notes" ) );
								else if( id == QString::fromLatin1("CryptographyPlugin") )
									dataField.setAttribute( QString::fromLatin1( "key" ), QString::fromLatin1( "gpgKey" ) );
								else if( id == QString::fromLatin1("TranslatorPlugin") )
									dataField.setAttribute( QString::fromLatin1( "key" ), QString::fromLatin1( "languageKey" ) );

								dataField.appendChild( newList.createTextNode( data ) );
							}
						}
					}
					oldContactNode = oldContactNode.nextSibling();
				}
			}
			else if( oldElement.tagName() == QString::fromLatin1("kopete-group") )
			{
				QDomElement newGroup = newList.createElement( QString::fromLatin1( "kopete-group" ) );
				newRoot.appendChild( newGroup );

				QDomNode oldGroupNode = oldNode.firstChild();
				while( !oldGroupNode.isNull() )
				{
					QDomElement oldGroupElement = oldGroupNode.toElement();

					if( oldGroupElement.tagName() == QString::fromLatin1("display-name") )
					{
						QDomElement displayName = newList.createElement( QString::fromLatin1( "display-name" ) );
						displayName.appendChild( newList.createTextNode( oldGroupElement.text() ) );
						newGroup.appendChild( displayName );
					}
					if( oldGroupElement.tagName() == QString::fromLatin1("type") )
					{
						if( oldGroupElement.text() == QString::fromLatin1("Temporary") )
							newGroup.setAttribute( QString::fromLatin1( "type" ), QString::fromLatin1( "temporary" ) );
						else if( oldGroupElement.text() == QString::fromLatin1("TopLevel") )
							newGroup.setAttribute( QString::fromLatin1( "type" ), QString::fromLatin1( "top-level" ) );
						else
							newGroup.setAttribute( QString::fromLatin1( "type" ), QString::fromLatin1( "standard" ) );
					}
					if( oldGroupElement.tagName() == QString::fromLatin1("view") )
					{
						if( oldGroupElement.text() == QString::fromLatin1("collapsed") )
							newGroup.setAttribute( QString::fromLatin1( "view" ), QString::fromLatin1( "collapsed" ) );
						else
							newGroup.setAttribute( QString::fromLatin1( "view" ), QString::fromLatin1( "expanded" ) );
					}
					else if( oldGroupElement.tagName() == QString::fromLatin1("plugin-data") )
					{
						// Per-group plugin data
						// FIXME: This needs updating too, ideally, convert this in a later
						//        contactlist.xml version
						QDomElement groupPluginData = newList.createElement( QString::fromLatin1( "plugin-data" ) );
						newGroup.appendChild( groupPluginData );

						groupPluginData.setAttribute( QString::fromLatin1( "plugin-id" ),
							oldGroupElement.attribute( QString::fromLatin1( "plugin-id" ), QString::null ) );
						groupPluginData.appendChild( newList.createTextNode( oldGroupElement.text() ) );
					}

					oldGroupNode = oldGroupNode.nextSibling();
				}
			}
			else
			{
				kdWarning( 14010 ) << k_funcinfo << "Unknown element '" << oldElement.tagName()
					<< "' in contact list!" << endl;
			}
		}
		oldNode = oldNode.nextSibling();
	}

	// Close the file, and save the new file
	contactListFile.close();

	QDir().rename( fileName, fileName + QString::fromLatin1( ".bak" ) );

	// kdDebug( 14010 ) << k_funcinfo << "XML output:\n" << newList.toString( 2 ) << endl;

	contactListFile.open( IO_WriteOnly );
	QTextStream stream( &contactListFile );
	stream.setEncoding( QTextStream::UnicodeUTF8 );
#if QT_VERSION < 0x030100
	stream << newList.toString();
#else
	stream << newList.toString( 2 );
#endif
	contactListFile.flush();
	contactListFile.close();
}

void KopeteContactList::saveXML()
{
	if(!m_loaded)
	{
		kdDebug(14010) << "KopeteContactList::saveXML: contactlist not loaded, abort saving" << endl;
		return;
	}

	QString contactListFileName = locateLocal( "appdata", QString::fromLatin1( "contactlist.xml" ) );
	KSaveFile contactListFile( contactListFileName );
	if( contactListFile.status() == 0 )
	{
		QTextStream *stream = contactListFile.textStream();
		stream->setEncoding( QTextStream::UnicodeUTF8 );
		toXML().save( *stream, 4 );

		if ( !contactListFile.close() )
		{
			kdDebug(14010) << "KopeteContactList::saveXML: failed to write contactlist, error code is: " << contactListFile.status() << endl;
		}
	}
	else
	{
		kdWarning(14010) << "KopeteContactList::saveXML: Couldn't open contact list file "
			<< contactListFileName << ". Contact list not saved." << endl;
	}
}

const QDomDocument KopeteContactList::toXML()
{
	QDomDocument doc;
	doc.appendChild( doc.createElement( QString::fromLatin1("kopete-contact-list") ) );
	doc.documentElement().setAttribute( QString::fromLatin1("version"), QString::fromLatin1("1.0"));

	// Save group information. ie: Open/Closed, pehaps later icons? Who knows.
	for( KopeteGroup *g = m_groupList.first(); g; g = m_groupList.next() )
		doc.documentElement().appendChild( doc.importNode( g->toXML(), true ) );

	// Save metacontact information.
	for( KopeteMetaContact *m = m_contacts.first(); m; m = m_contacts.next() )
	{
		if( !m->isTemporary() )
			doc.documentElement().appendChild( doc.importNode( m->toXML(), true ) );
	}

	return doc;
}


QStringList KopeteContactList::contacts() const
{
	QStringList contacts;
	QPtrListIterator<KopeteMetaContact> it( m_contacts );
	for( ; it.current(); ++it )
	{
		contacts.append( it.current()->displayName() );
	}
	return contacts;
}

QStringList KopeteContactList::contactStatuses() const
{
	QStringList meta_contacts;
	QPtrListIterator<KopeteMetaContact> it( m_contacts );
	for( ; it.current(); ++it )
	{
		meta_contacts.append( QString::fromLatin1( "%1 (%2)" ).
#if QT_VERSION < 0x030200
			arg( it.current()->displayName() ).arg( it.current()->statusString() )
#else
			arg( it.current()->displayName(), it.current()->statusString() )
#endif
		);
	}
	return meta_contacts;
}

QStringList KopeteContactList::reachableContacts() const
{
	QStringList contacts;
	QPtrListIterator<KopeteMetaContact> it( m_contacts );
	for( ; it.current(); ++it )
	{
		if ( it.current()->isReachable() )
			contacts.append( it.current()->displayName() );
	}
	return contacts;
}

QPtrList<KopeteContact> KopeteContactList::onlineContacts() const
{
	QPtrList<KopeteContact> result;
	QPtrListIterator<KopeteMetaContact> it( m_contacts );
	for( ; it.current(); ++it )
	{
		if ( it.current()->isOnline() )
		{
			QPtrList<KopeteContact> contacts = it.current()->contacts();
			QPtrListIterator<KopeteContact> cit( contacts );
			for( ; cit.current(); ++cit )
			{
				if ( cit.current()->isOnline() )
					result.append( cit.current() );
			}
		}
	}
	return result;
}

QPtrList<KopeteMetaContact> KopeteContactList::onlineMetaContacts() const
{
	QPtrList<KopeteMetaContact> result;
	QPtrListIterator<KopeteMetaContact> it( m_contacts );
	for( ; it.current(); ++it )
	{
		if ( it.current()->isOnline() )
			result.append( it.current() );
	}
	return result;
}

QPtrList<KopeteMetaContact> KopeteContactList::onlineMetaContacts( const QString &protocolId ) const
{
	QPtrList<KopeteMetaContact> result;
	QPtrListIterator<KopeteMetaContact> it( m_contacts );
	for( ; it.current(); ++it )
	{
		// FIXME: This loop is not very efficient :(
		if ( it.current()->isOnline() )
		{
			QPtrList<KopeteContact> contacts = it.current()->contacts();
			QPtrListIterator<KopeteContact> cit( contacts );
			for( ; cit.current(); ++cit )
			{
				if( cit.current()->isOnline() && cit.current()->protocol()->pluginId() == protocolId )
					result.append( it.current() );
			}
		}
	}
	return result;
}

QPtrList<KopeteContact> KopeteContactList::onlineContacts( const QString &protocolId ) const
{
	QPtrList<KopeteContact> result;
	QPtrListIterator<KopeteMetaContact> it( m_contacts );
	for( ; it.current(); ++it )
	{
		// FIXME: This loop is not very efficient :(
		if ( it.current()->isOnline() )
		{
			QPtrList<KopeteContact> contacts = it.current()->contacts();
			QPtrListIterator<KopeteContact> cit( contacts );
			for( ; cit.current(); ++cit )
			{
				if( cit.current()->isOnline() && cit.current()->protocol()->pluginId() == protocolId )
					result.append( cit.current() );
			}
		}
	}
	return result;
}

QStringList KopeteContactList::fileTransferContacts() const
{
	QStringList contacts;
	QPtrListIterator<KopeteMetaContact> it( m_contacts );
	for( ; it.current(); ++it )
	{
		if ( it.current()->canAcceptFiles() )
			contacts.append( it.current()->displayName() );
	}
	return contacts;
}

void KopeteContactList::sendFile( const QString &displayName, const KURL &sourceURL,
	const QString &altFileName, const long unsigned int fileSize)
{
//	kdDebug(14010) << "Send To Display Name: " << displayName << "\n";

	KopeteMetaContact *c = findContactByDisplayName( displayName );
	if( c )
		c->sendFile( sourceURL, altFileName, fileSize );
}

void KopeteContactList::messageContact( const QString &displayName, const QString & /* messageText */ )
{
	KopeteMetaContact *c = findContactByDisplayName( displayName );
	if( c )
		c->execute();

	// TODO: Add the message text
}

KopeteMetaContact *KopeteContactList::findContactByDisplayName( const QString &displayName )
{
	QPtrListIterator<KopeteMetaContact> it( m_contacts );
	for( ; it.current(); ++it )
	{
//		kdDebug(14010) << "Display Name: " << it.current()->displayName() << "\n";
		if( it.current()->displayName() == displayName ) {
			return it.current();
		}
	}

	return 0L;
}

QStringList KopeteContactList::contactFileProtocols(const QString &displayName)
{
//	kdDebug(14010) << "Get contacts for: " << displayName << "\n";
	QStringList protocols;

	KopeteMetaContact *c = findContactByDisplayName( displayName );
	if( c )
	{
		QPtrList<KopeteContact> mContacts = c->contacts();
		kdDebug(14010) << mContacts.count() << endl;
		QPtrListIterator<KopeteContact> jt( mContacts );
		for ( ; jt.current(); ++jt )
		{
			kdDebug(14010) << "1" << jt.current()->protocol()->pluginId() << "\n";
			if( jt.current()->canAcceptFiles() ) {
				kdDebug(14010) << jt.current()->protocol()->pluginId() << "\n";
				protocols.append ( jt.current()->protocol()->pluginId() );
			}
		}
		return protocols;
	}
	return QStringList();
}


KopeteGroupList KopeteContactList::groups() const
{
	return m_groupList;
}

void KopeteContactList::removeMetaContact(KopeteMetaContact *m)
{
	QPtrList<KopeteContact> children = m->contacts();
	for( KopeteContact *c = children.first(); c; c = children.next() )
	{
/*		if( c->conversations() > 0 )
		{
			KopeteMetaContact *m = new KopeteMetaContact();
			m->setTemporary( true );
			c->setMetaContact( m );
		}
		else*/
			c->slotDeleteContact();
	}

	emit metaContactDeleted( m );
	m_contacts.remove( m );
	m->deleteLater();
}

QPtrList<KopeteMetaContact> KopeteContactList::metaContacts() const
{
	return m_contacts;
}

void KopeteContactList::addGroup( KopeteGroup * g)
{
	if(!m_groupList.contains(g) )
	{
		m_groupList.append( g );
		emit groupAdded( g );
		connect( g , SIGNAL ( renamed(KopeteGroup* , const QString & )) , this , SIGNAL ( groupRenamed(KopeteGroup* , const QString & )) ) ;
	}
}

void KopeteContactList::removeGroup( KopeteGroup *g)
{
	m_groupList.remove( g );
	emit groupRemoved( g );
	delete g;
}

KopeteGroup * KopeteContactList::getGroup(const QString& displayName, KopeteGroup::GroupType type)
{
	if( type == KopeteGroup::Temporary )
		return KopeteGroup::temporary;

	KopeteGroup *groupIterator;;
	for ( groupIterator = m_groupList.first(); groupIterator; groupIterator = m_groupList.next() )
	{
		if( groupIterator->type() == type && groupIterator->displayName() == displayName )
			return groupIterator;
	}

	KopeteGroup *newGroup = new KopeteGroup( displayName, type );
	addGroup( newGroup );
	return  newGroup;
}

KopeteGroup * KopeteContactList::getGroup(unsigned int groupId)
{
	KopeteGroup *groupIterator;
	for ( groupIterator = m_groupList.first(); groupIterator; groupIterator = m_groupList.next() )
	{
		if( groupIterator->groupId()==groupId )
			return groupIterator;
	}
	return 0L;
}


#include "kopetecontactlist.moc"

// vim: set noet ts=4 sts=4 sw=4:

