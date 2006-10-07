/*
    kopetemetacontact.cpp - Kopete Meta Contact

    Copyright (c) 2002-2003 by Martijn Klingens       <klingens@kde.org>
    Copyright (c) 2002-2005 by Olivier Goffart        <ogoffart@ kde.org>
    Copyright (c) 2002-2004 by Duncan Mac-Vicar Prett <duncan@kde.org>
    Copyright (c) 2005      by Michaël Larouche       <larouche@kde.org>

    Kopete    (c) 2002-2006 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include "kopetemetacontact.h"
#include "kopetemetacontact_p.h"

#include <kapplication.h>

#include <kabc/addressbook.h>
#include <kabc/addressee.h>

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdeversion.h>

#include "kabcpersistence.h"
#include "kopetecontactlist.h"
#include "kopetecontact.h"
#include "kopeteaccountmanager.h"
#include "kopeteprotocol.h"
#include "kopeteaccount.h"
#include "kopetepluginmanager.h"
#include "kopetegroup.h"
#include "kopeteglobal.h"
#include "kopeteuiglobal.h"

namespace Kopete {

MetaContact::MetaContact()
	: ContactListElement( ContactList::self() )
{
	d = new Private;

	connect( this, SIGNAL( pluginDataChanged() ), SIGNAL( persistentDataChanged() ) );
	connect( this, SIGNAL( iconChanged( Kopete::ContactListElement::IconState, const QString & ) ), SIGNAL( persistentDataChanged() ) );
	connect( this, SIGNAL( useCustomIconChanged( bool ) ), SIGNAL( persistentDataChanged() ) );
	connect( this, SIGNAL( displayNameChanged( const QString &, const QString & ) ), SIGNAL( persistentDataChanged() ) );
	connect( this, SIGNAL( movedToGroup( Kopete::MetaContact *, Kopete::Group *, Kopete::Group * ) ), SIGNAL( persistentDataChanged() ) );
	connect( this, SIGNAL( removedFromGroup( Kopete::MetaContact *, Kopete::Group * ) ), SIGNAL( persistentDataChanged() ) );
	connect( this, SIGNAL( addedToGroup( Kopete::MetaContact *, Kopete::Group * ) ), SIGNAL( persistentDataChanged() ) );
	connect( this, SIGNAL( contactAdded( Kopete::Contact * ) ), SIGNAL( persistentDataChanged() ) );
	connect( this, SIGNAL( contactRemoved( Kopete::Contact * ) ), SIGNAL( persistentDataChanged() ) );

	// Update the KABC picture when the KDE Address book change.
	connect(KABCPersistence::self()->addressBook(), SIGNAL(addressBookChanged(AddressBook *)), this, SLOT(slotUpdateAddressBookPicture()));

	// make sure MetaContact is at least in one group
	addToGroup( Group::topLevel() );
			 // I'm not sure this is correct -Olivier
			 // we probably should do the check in groups() instead
}

MetaContact::~MetaContact()
{
	delete d;
}

void MetaContact::addContact( Contact *c )
{
	if( d->contacts.contains( c ) )
	{
		kWarning(14010) << "Ignoring attempt to add duplicate contact " << c->contactId() << "!" << endl;
	}
	else
	{
		d->contacts.append( c );

		connect( c, SIGNAL( onlineStatusChanged( Kopete::Contact *, const Kopete::OnlineStatus &, const Kopete::OnlineStatus & ) ),
			SLOT( slotContactStatusChanged( Kopete::Contact *, const Kopete::OnlineStatus &, const Kopete::OnlineStatus & ) ) );

		connect( c, SIGNAL( propertyChanged( Kopete::Contact *, const QString &, const QVariant &, const QVariant & ) ),
			this, SLOT( slotPropertyChanged( Kopete::Contact *, const QString &, const QVariant &, const QVariant & ) ) ) ;

		connect( c, SIGNAL( contactDestroyed( Kopete::Contact * ) ),
			this, SLOT( slotContactDestroyed( Kopete::Contact * ) ) );

		connect( c, SIGNAL( idleStateChanged( Kopete::Contact * ) ),
			this, SIGNAL( contactIdleStateChanged( Kopete::Contact * ) ) );

		emit contactAdded(c);

		updateOnlineStatus();

		// if this is the first contact, probbaly was created by a protocol
		// so it has empty custom properties, then set sources to the contact
		if ( d->contacts.count() == 1 )
		{
			if ( displayName().isEmpty() )
			{
				setDisplayNameSourceContact(c);
				setDisplayNameSource(SourceContact);
			}
			if ( picture().isNull() )
			{
				setPhotoSourceContact(c);
				setPhotoSource(SourceContact);
			}
		}
	}
}

void MetaContact::updateOnlineStatus()
{
	Kopete::OnlineStatus::StatusType newStatus = Kopete::OnlineStatus::Unknown;
	Kopete::OnlineStatus mostSignificantStatus;

	QListIterator<Contact *> it(d->contacts);
	while ( it.hasNext() )
	{
		Contact *c = it.next();
		// find most significant status
		if ( c->onlineStatus() > mostSignificantStatus )
			mostSignificantStatus = c->onlineStatus();
	}

	newStatus = mostSignificantStatus.status();

	if( newStatus != d->onlineStatus )
	{
		d->onlineStatus = newStatus;
		emit onlineStatusChanged( this, d->onlineStatus );
	}
}


void MetaContact::removeContact(Contact *c, bool deleted)
{
	if( !d->contacts.contains( c ) )
	{
		kDebug(14010) << k_funcinfo << " Contact is not in this metaContact " << endl;
	}
	else
	{
		// must check before removing, or will always be false
		bool wasTrackingName = ( !displayNameSourceContact() && (displayNameSource() == SourceContact) );
		bool wasTrackingPhoto = ( !photoSourceContact() && (photoSource() == SourceContact) );
		// save for later use
		QString currDisplayName = displayName();

		d->contacts.removeAll( c );
		
		// if the contact was a source of property data, clean
		if (displayNameSourceContact() == c)
			setDisplayNameSourceContact(0L);
		if (photoSourceContact() == c)
			setPhotoSourceContact(0L);


		if ( wasTrackingName )
		{
			// Oh! this contact was the source for the metacontact's name
			// lets do something
			// is this the only contact?
			if ( d->contacts.isEmpty() )
			{
				// fallback to a custom name as we don't have
				// more contacts to chose as source.
				setDisplayNameSource(SourceCustom);
				// perhaps the custom display name was empty
				// no problems baby, I saved the old one.
				setDisplayName(currDisplayName);
			}
			else
			{
				// we didn't fallback to SourceCustom above so lets use the next
				// contact as source
				setDisplayNameSourceContact( d->contacts.first() );
			}
		}

		if ( wasTrackingPhoto )
		{
			// Oh! this contact was the source for the metacontact's photo
			// lets do something
			// is this the only contact?
			if ( d->contacts.isEmpty() )
			{
				// fallback to a custom photo as we don't have
				// more contacts to chose as source.
				setPhotoSource(SourceCustom);
				// FIXME set the custom photo
			}
			else
			{
				// we didn't fallback to SourceCustom above so lets use the next
				// contact as source
				setPhotoSourceContact( d->contacts.first() );
			}
		}

		if(!deleted)
		{  //If this function is tell by slotContactRemoved, c is maybe just a QObject
			disconnect( c, SIGNAL( onlineStatusChanged( Kopete::Contact *, const Kopete::OnlineStatus &, const Kopete::OnlineStatus & ) ),
				this, SLOT( slotContactStatusChanged( Kopete::Contact *, const Kopete::OnlineStatus &, const Kopete::OnlineStatus & ) ) );
			disconnect( c, SIGNAL( propertyChanged( Kopete::Contact *, const QString &, const QVariant &, const QVariant & ) ),
				this, SLOT( slotPropertyChanged( Kopete::Contact *, const QString &, const QVariant &, const QVariant & ) ) ) ;
			disconnect( c, SIGNAL( contactDestroyed( Kopete::Contact * ) ),
				this, SLOT( slotContactDestroyed( Kopete::Contact * ) ) );
			disconnect( c, SIGNAL( idleStateChanged( Kopete::Contact * ) ),
				this, SIGNAL( contactIdleStateChanged( Kopete::Contact *) ) );

			kDebug( 14010 ) << k_funcinfo << "Contact disconnected" << endl;

			KABCPersistence::self()->write( this );
		}

		// Reparent the contact
		c->setParent( 0 );

		emit contactRemoved( c );
	}
	updateOnlineStatus();
}

Contact *MetaContact::findContact( const QString &protocolId, const QString &accountId, const QString &contactId )
{
	//kDebug( 14010 ) << k_funcinfo << "Num contacts: " << d->contacts.count() << endl;
	QListIterator<Contact *> it( d->contacts );
	while ( it.hasNext() )
	{
		Contact *c = it.next();
		//kDebug( 14010 ) << k_funcinfo << "Trying " << it.current()->contactId() << ", proto "
		//<< it.current()->protocol()->pluginId() << ", account " << it.current()->accountId() << endl;
		if( ( c->contactId() == contactId ) && ( c->protocol()->pluginId() == protocolId || protocolId.isNull() ) )
		{
			if ( accountId.isNull() )
				return c;

			if(c->account())
			{
				if(c->account()->accountId() == accountId)
					return c;
			}
		}
	}

	// Contact not found
	return 0L;
}

void MetaContact::setDisplayNameSource(PropertySource source)
{
	QString oldName = displayName();
	d->displayNameSource = source;
	QString newName = displayName();
	if ( oldName != newName)
		emit displayNameChanged( oldName, newName );
}

void MetaContact::setDisplayNameSource( const QString &nameSourcePID, const QString &nameSourceAID, const QString &nameSourceCID )
{
	d->nameSourcePID = nameSourcePID;
	d->nameSourceAID = nameSourceAID;
	d->nameSourceCID = nameSourceCID;
}

MetaContact::PropertySource MetaContact::displayNameSource() const
{
	return d->displayNameSource;
}

void MetaContact::setPhotoSource(PropertySource source)
{
	PropertySource oldSource = photoSource();
	d->photoSource = source;
	if ( source != oldSource )
	{
		emit photoChanged();
	}
}

void MetaContact::setPhotoSource( const QString &photoSourcePID, const QString &photoSourceAID, const QString &photoSourceCID )
{
	d->photoSourcePID = photoSourcePID;
	d->photoSourceAID = photoSourceAID;
	d->photoSourceCID = photoSourceCID;
}

MetaContact::PropertySource MetaContact::photoSource() const
{
	return d->photoSource;
}


Contact *MetaContact::sendMessage()
{
	Contact *c = preferredContact();

	if( !c )
	{
		KMessageBox::queuedMessageBox( UI::Global::mainWidget(), KMessageBox::Sorry,
			i18n( "This user is not reachable at the moment. Please make sure you are connected and using a protocol that supports offline sending, or wait "
			"until this user comes online." ), i18n( "User is Not Reachable" ) );
	}
	else
	{
		c->sendMessage();
		return c;
	}
	return 0L;
}

Contact *MetaContact::startChat()
{
	Contact *c = preferredContact();

	if( !c )
	{
		KMessageBox::queuedMessageBox( UI::Global::mainWidget(), KMessageBox::Sorry,
			i18n( "This user is not reachable at the moment. Please make sure you are connected and using a protocol that supports offline sending, or wait "
			"until this user comes online." ), i18n( "User is Not Reachable" ) );
	}
	else
	{
		c->startChat();
		return c;
	}
	return 0L;
}

Contact *MetaContact::preferredContact()
{
	/*
		This function will determine what contact will be used to reach the contact.

		The preferred contact is choose with the following criterias:  (in that order)
		1) If a contact was an open chatwindow already, we will use that one.
		2) The contact with the better online status is used. But if that
		    contact is not reachable, we prefer return no contact.
		3) If all the criterias aboxe still gives ex-eaquo, we use the preffered
		    account as selected in the account preferances (with the arrows)
	*/

	Contact *contact = 0;
	bool hasOpenView=false; //has the selected contact already an open chatwindow
	QListIterator<Contact *> it(d->contacts);
	while ( it.hasNext() )
	{
		Contact *c=it.next();

		//Does the contact an open chatwindow?
		if( c->manager( Contact::CannotCreate ) )
		{ //no need to check the view. having a manager is enough
			if( !hasOpenView )
			{
				contact=c;
				hasOpenView=true;
				if( c->isReachable() )
					continue;
			} //else, several contact might have an open view, uses following criterias
		}
		else if( hasOpenView && contact->isReachable() )
			continue; //This contact has not open view, but the selected contact has, and is reachable

		// FIXME: The isConnected call should be handled in Contact::isReachable
		//        after KDE 3.2 - Martijn
		if ( !c->account() || !c->account()->isConnected() || !c->isReachable() )
			continue; //if this contact is not reachable, we ignore it.

		if ( !contact )
		{  //this is the first contact.
			contact= c;
			continue;
		}

		if( c->onlineStatus().status() > contact->onlineStatus().status()  )
			contact=c; //this contact has a better status
		else if ( c->onlineStatus().status() == contact->onlineStatus().status() )
		{
			if( c->account()->priority() > contact->account()->priority() )
				contact=c;
			else if(  c->account()->priority() == contact->account()->priority()
					&& c->onlineStatus().weight() > contact->onlineStatus().weight() )
				contact = c;  //the weight is not supposed to follow the same scale for each protocol
		}
	}
	return contact;
}

Contact *MetaContact::execute()
{
	Contact *c = preferredContact();

	if( !c )
	{
		KMessageBox::queuedMessageBox( UI::Global::mainWidget(), KMessageBox::Sorry,
			i18n( "This user is not reachable at the moment. Please make sure you are connected and using a protocol that supports offline sending, or wait "
			"until this user comes online." ), i18n( "User is Not Reachable" ) );
	}
	else
	{
		c->execute();
		return c;
	}

	return 0L;
}

unsigned long int MetaContact::idleTime() const
{
	unsigned long int time = 0;
	QListIterator<Contact *> it( d->contacts );
	while ( it.hasNext() )
	{
		Contact *c = it.next();
		unsigned long int i = c->idleTime();
		if( c->isOnline() && i < time || time == 0 )
		{
			time = i;
		}
	}
	return time;
}

QString MetaContact::statusIcon() const
{
	switch( status() )
	{
		case OnlineStatus::Online:
			if( useCustomIcon() )
				return icon( ContactListElement::Online );
			else
				return QString::fromUtf8( "metacontact_online" );
		case OnlineStatus::Away:
			if( useCustomIcon() )
				return icon( ContactListElement::Away );
			else
				return QString::fromUtf8( "metacontact_away" );

		case OnlineStatus::Unknown:
			if( useCustomIcon() )
				return icon( ContactListElement::Unknown );
			if ( d->contacts.isEmpty() )
				return QString::fromUtf8( "metacontact_unknown" );
			else
				return QString::fromUtf8( "metacontact_offline" );

		case OnlineStatus::Offline:
		default:
			if( useCustomIcon() )
				return icon( ContactListElement::Offline );
			else
				return QString::fromUtf8( "metacontact_offline" );
	}
}

QString MetaContact::statusString() const
{
	switch( status() )
	{
		case OnlineStatus::Online:
			return i18n( "Online" );
		case OnlineStatus::Away:
			return i18n( "Away" );
		case OnlineStatus::Offline:
			return i18n( "Offline" );
		case OnlineStatus::Unknown:
		default:
			return i18n( "Status not available" );
	}
}

OnlineStatus::StatusType MetaContact::status() const
{
	return d->onlineStatus;
}

bool MetaContact::isOnline() const
{
	QListIterator<Contact *> it( d->contacts );
	while ( it.hasNext() )
	{
		if( it.next()->isOnline() )
			return true;
	}
	return false;
}

bool MetaContact::isReachable() const
{
	if ( isOnline() )
		return true;

	QListIterator<Contact *> it( d->contacts );
	while ( it.hasNext() )
	{
		Contact *c = it.next();
		if ( c->account()->isConnected() && c->isReachable() )
			return true;
	}
	return false;
}

//Determine if we are capable of accepting file transfers
bool MetaContact::canAcceptFiles() const
{
	if( !isOnline() )
		return false;

	QListIterator<Contact *> it( d->contacts );
	while ( it.hasNext() )
	{
		if( it.next()->canAcceptFiles() )
			return true;
	}
	return false;
}

//Slot for sending files
void MetaContact::sendFile( const KUrl &sourceURL, const QString &altFileName, unsigned long fileSize )
{
	//If we can't send any files then exit
	if( d->contacts.isEmpty() || !canAcceptFiles() )
		return;

	//Find the highest ranked protocol that can accept files
	Contact *contact = d->contacts.first();
	QListIterator<Contact *> it( d->contacts );
	while ( it.hasNext() )
	{
		Contact *curr = it.next();
		if( (curr)->onlineStatus() > contact->onlineStatus() && (curr)->canAcceptFiles() )
			contact = curr;
	}

	//Call the sendFile slot of this protocol
	contact->sendFile( sourceURL, altFileName, fileSize );
}

void MetaContact::emitAboutToSave()
{
	emit aboutToSave( this );
}

void MetaContact::slotContactStatusChanged( Contact * c, const OnlineStatus &status, const OnlineStatus &/*oldstatus*/  )
{
	updateOnlineStatus();
	emit contactStatusChanged( c, status );
}

void MetaContact::setDisplayName( const QString &name )
{
	/*kDebug( 14010 ) << k_funcinfo << "Change displayName from " << d->displayName <<
		" to " << name  << ", d->trackChildNameChanges=" << d->trackChildNameChanges << endl;
	kDebug(14010) << kBacktrace(6) << endl;*/

	if( name == d->displayName )
		return;

	if ( loading() )
	{
		d->displayName = name;
	}
	else
	{
		const QString old = d->displayName;
		d->displayName = name;

		emit displayNameChanged( old , name );
		QListIterator<Kopete::Contact *> it( d->contacts );
		while (  it.hasNext() )
			( it.next() )->sync(Contact::DisplayNameChanged);
	}

}

QString MetaContact::customDisplayName() const
{
	return d->displayName;
}

QString MetaContact::displayName() const
{
	PropertySource source = displayNameSource();
	if ( source == SourceKABC )
	{
		// kabc source, try to get from addressbook
		// if the metacontact has a kabc association
		if ( !metaContactId().isEmpty() )
			return nameFromKABC(metaContactId());
	}
	else if ( source == SourceContact )
	{
		if ( d->displayNameSourceContact==0 )
		{
			if( d->contacts.count() >= 1 )
			{// don't call setDisplayNameSource , or there will probably be an infinite loop
				d->displayNameSourceContact=d->contacts.first();
//				kDebug( 14010 ) << k_funcinfo << " setting displayname source for " << metaContactId()  << endl;
			}
		}
		if ( displayNameSourceContact() != 0L )
		{
			return nameFromContact(displayNameSourceContact());
		}
		else
		{
//			kDebug( 14010 ) << k_funcinfo << " source == SourceContact , but there is no displayNameSourceContact for contact " << metaContactId() << endl;
		}
	}
	return d->displayName;
}

QString nameFromKABC( const QString &id ) /*const*/
{
	KABC::AddressBook* ab = KABCPersistence::self()->addressBook();
	if ( ! id.isEmpty() && !id.contains(':') )
	{
		KABC::Addressee theAddressee = ab->findByUid(id);
		if ( theAddressee.isEmpty() )
		{
			kDebug( 14010 ) << k_funcinfo << "no KABC::Addressee found for ( " << id << " ) " << " in current address book" << endl;
		}
		else
		{
			return theAddressee.formattedName();
		}
	}
	// no kabc association, return null image
	return QString();
}

QString nameFromContact( Kopete::Contact *c) /*const*/
{
	if ( !c )
		return QString();

	QString contactName;
	if ( c->hasProperty( Kopete::Global::Properties::self()->nickName().key() ) )
		contactName = c->property( Global::Properties::self()->nickName()).value().toString();

				//the replace is there to workaround the Bug 95444
	return contactName.isEmpty() ? c->contactId() : contactName.replace('\n',QString::fromUtf8(""));
}

KUrl MetaContact::customPhoto() const
{
	return KUrl(d->customPicture.path());
}

void MetaContact::setPhoto( const KUrl &url )
{
	d->photoUrl = url;
	d->customPicture.setPicture(url.path());

	if ( photoSource() == SourceCustom )
	{
		emit photoChanged();
	}
}

QImage MetaContact::photo() const
{
	return picture().image();
}

Picture &MetaContact::picture() const
{
	if ( photoSource() == SourceKABC )
	{
		return d->kabcPicture;
	}
	else if ( photoSource() == SourceContact )
	{
		return d->contactPicture;
	}

	return d->customPicture;
}

QImage MetaContact::photoFromCustom() const
{
	return d->customPicture.image();
}

QImage photoFromContact( Kopete::Contact *contact) /*const*/
{

	// screw it, crashes now. 
	// FIXME investigate later
	
	return QImage();

	if ( contact == 0L )
		return QImage();

	QVariant photoProp;
	if ( contact->hasProperty( Kopete::Global::Properties::self()->photo().key() ) )
		photoProp = contact->property( Kopete::Global::Properties::self()->photo().key() ).value();

	QImage img;
	if(photoProp.canConvert( QVariant::Image ))
		img= photoProp.value<QImage>();
	else if(photoProp.canConvert( QVariant::Pixmap ))
		img=photoProp.value<QPixmap>().toImage();
	else if(!photoProp.toString().isEmpty())
	{
		img=QPixmap( photoProp.toString() ).toImage();
	}
	return img;
}

QImage photoFromKABC( const QString &id ) /*const*/
{
	KABC::AddressBook* ab = KABCPersistence::self()->addressBook();
	if ( ! id.isEmpty() && !id.contains(':') )
	{
		KABC::Addressee theAddressee = ab->findByUid(id);
		if ( theAddressee.isEmpty() )
		{
			kDebug( 14010 ) << k_funcinfo << "no KABC::Addressee found for ( " << id << " ) " << " in current address book" << endl;
		}
		else
		{
			KABC::Picture pic = theAddressee.photo();
			if ( pic.data().isNull() && pic.url().isEmpty() )
				pic = theAddressee.logo();

			if ( pic.isIntern())
			{
				return pic.data();
			}
			else
			{
				return QPixmap( pic.url() ).toImage();
			}
		}
	}
	// no kabc association, return null image
	return QImage();
}

Contact *MetaContact::displayNameSourceContact() const
{
	return d->displayNameSourceContact;
}

Contact *MetaContact::photoSourceContact() const
{
	return d->photoSourceContact;
}

void MetaContact::setDisplayNameSourceContact( Contact *contact )
{
	Contact *old = d->displayNameSourceContact;
	d->displayNameSourceContact = contact;
	if ( displayNameSource() == SourceContact )
	{
		emit displayNameChanged( nameFromContact(old), nameFromContact(contact));
	}
}

void MetaContact::setPhotoSourceContact( Contact *contact )
{
	d->photoSourceContact = contact;

	// Create a cache for the contact photo.
	if(d->photoSourceContact != 0L)
	{
		QVariant photoProp;
		if ( contact->hasProperty( Kopete::Global::Properties::self()->photo().key() ) )
		{
			photoProp = contact->property( Kopete::Global::Properties::self()->photo().key() ).value();
		
			if(photoProp.canConvert( QVariant::Image ))
			{
				d->contactPicture.setPicture( photoProp.value<QImage>() );
			}
			else if(photoProp.canConvert( QVariant::Pixmap ))
			{
				d->contactPicture.setPicture( photoProp.value<QPixmap>().toImage() );
			}
			else if(!photoProp.toString().isEmpty())
			{
				d->contactPicture.setPicture(photoProp.toString());
			}
		}
	}

	if ( photoSource() == SourceContact )
	{
		emit photoChanged();
	}
}

void MetaContact::slotPropertyChanged( Contact* subcontact, const QString &key,
		const QVariant &oldValue, const QVariant &newValue  )
{
	if ( displayNameSource() == SourceContact )
	{
		if( key == Global::Properties::self()->nickName().key() )
		{
			if (displayNameSourceContact() == subcontact)
			{
				emit displayNameChanged( oldValue.toString(), newValue.toString());
			}
			else
			{
				// HACK the displayName that changed is not from the contact we are tracking, but
				// as the current one is null, lets use this new one
				if (displayName().isEmpty())
					setDisplayNameSourceContact(subcontact);
			}
		}
	}

	if (photoSource() == SourceContact)
	{
		if ( key == Global::Properties::self()->photo().key() )
		{
			if (photoSourceContact() != subcontact)
			{
				// HACK the displayName that changed is not from the contact we are tracking, but
				// as the current one is null, lets use this new one
				if (picture().isNull())
					setPhotoSourceContact(subcontact);
					
			}
			else if(photoSourceContact() == subcontact)
			{
				if(d->photoSyncedWithKABC)
					setPhotoSyncedWithKABC(true);
					
				setPhotoSourceContact(subcontact);
			}
		}
	}
}

void MetaContact::moveToGroup( Group *from, Group *to )
{
	if ( !from || !groups().contains( from )  )
	{
		// We're adding, not moving, because 'from' is illegal
		addToGroup( to );
		return;
	}

	if ( !to || groups().contains( to )  )
	{
		// We're removing, not moving, because 'to' is illegal
		removeFromGroup( from );
		return;
	}

	if ( isTemporary() && to->type() != Group::Temporary )
		return;


	//kDebug( 14010 ) << k_funcinfo << from->displayName() << " => " << to->displayName() << endl;

	d->groups.removeAll( from );
	d->groups.append( to );

	QListIterator<Contact *> it(d->contacts);
	while( it.hasNext() )
		it.next()->sync(Contact::MovedBetweenGroup);

	emit movedToGroup( this, from, to );
}

void MetaContact::removeFromGroup( Group *group )
{
	if ( !group || !groups().contains( group ) || ( isTemporary() && group->type() == Group::Temporary ) )
	{
		return;
	}

	d->groups.removeAll( group );

	// make sure MetaContact is at least in one group
	if ( d->groups.isEmpty() )
	{
		d->groups.append( Group::topLevel() );
		emit addedToGroup( this, Group::topLevel() );
	}

	QListIterator<Contact *> it(d->contacts);
	while( it.hasNext() )
		it.next()->sync(Contact::MovedBetweenGroup);

	emit removedFromGroup( this, group );
}

void MetaContact::addToGroup( Group *to )
{
	if ( !to || groups().contains( to )  )
		return;

	if ( d->temporary && to->type() != Group::Temporary )
		return;

	if ( d->groups.contains( Group::topLevel() ) )
	{
		d->groups.removeAll( Group::topLevel() );
		emit removedFromGroup( this, Group::topLevel() );
	}

	d->groups.append( to );

	QListIterator<Contact *> it(d->contacts);
	while( it.hasNext() )
		it.next()->sync(Contact::MovedBetweenGroup);

	emit addedToGroup( this, to );
}

QList<Group *> MetaContact::groups() const
{
	return d->groups;
}

void MetaContact::slotContactDestroyed( Contact *contact )
{
	removeContact(contact,true);
}

QString MetaContact::addressBookField( Kopete::Plugin * /* p */, const QString &app, const QString & key ) const
{
	return d->addressBook[ app ][ key ];
}

void Kopete::MetaContact::setAddressBookField( Kopete::Plugin * /* p */, const QString &app, const QString &key, const QString &value )
{
	d->addressBook[ app ][ key ] = value;
}

void MetaContact::slotPluginLoaded( Plugin *p )
{
	if( !p )
		return;

	QMap<QString, QString> map= pluginData( p );
	if(!map.isEmpty())
	{
		p->deserialize(this,map);
	}
}

void MetaContact::slotAllPluginsLoaded()
{
	// Now that the plugins and subcontacts are loaded, set the source contact.
	setDisplayNameSourceContact( findContact( d->nameSourcePID, d->nameSourceAID, d->nameSourceCID) );
	setPhotoSourceContact( findContact( d->photoSourcePID, d->photoSourceAID, d->photoSourceCID) );
}

void MetaContact::slotUpdateAddressBookPicture()
{
	KABC::AddressBook* ab = KABCPersistence::self()->addressBook();
	QString id = metaContactId();
	if ( !id.isEmpty() && !id.contains(':') )
	{
		KABC::Addressee theAddressee = ab->findByUid(id);
		if ( theAddressee.isEmpty() )
		{
			kDebug( 14010 ) << k_funcinfo << "no KABC::Addressee found for ( " << id << " ) " << " in current address book" << endl;
		}
		else
		{
			KABC::Picture pic = theAddressee.photo();
			if ( pic.data().isNull() && pic.url().isEmpty() )
				pic = theAddressee.logo();

			d->kabcPicture.setPicture(pic);
		}
	}
}

bool MetaContact::isTemporary() const
{
	return d->temporary;
}

void MetaContact::setTemporary( bool isTemporary, Group *group )
{
	d->temporary = isTemporary;
	Group *temporaryGroup = Group::temporary();
	if ( d->temporary )
	{
		addToGroup (temporaryGroup);
		QListIterator<Group *> it(d->groups);
		while ( it.hasNext() )
		{
			Group *g = it.next();
			if(g != temporaryGroup)
				removeFromGroup(g);
		}
	}
	else
		moveToGroup(temporaryGroup, group ? group : Group::topLevel());
}

QString MetaContact::metaContactId() const
{
	if(d->metaContactId.isEmpty())
	{
		if(d->contacts.isEmpty())
			return QString();
		Contact *c=d->contacts.first();
		if(!c)
			return QString();
		return c->protocol()->pluginId()+QString::fromUtf8(":")+c->account()->accountId()+QString::fromUtf8(":") + c->contactId() ;
	}
	return d->metaContactId;
}

void MetaContact::setMetaContactId( const QString& newMetaContactId )
{
	if(newMetaContactId == d->metaContactId)
		return;

	// 1) Check the Id is not already used by another contact
	// 2) cause a kabc write ( only in response to metacontactLVIProps calling this, or will
	//      write be called twice when creating a brand new MC? )
	// 3) What about changing from one valid kabc to another, are kabc fields removed?
	// 4) May be called with Null to remove an invalid kabc uid by KMC::toKABC()
	// 5) Is called when reading the saved contact list

	// Don't remove IM addresses from kabc if we are changing contacts;
	// other programs may have written that data and depend on it
	d->metaContactId = newMetaContactId;
	if ( loading() )
	{
		slotUpdateAddressBookPicture();
	}
	else
	{
		KABCPersistence::self()->write( this );
		emit onlineStatusChanged( this, d->onlineStatus );
		emit persistentDataChanged();
	}
}

bool MetaContact::isPhotoSyncedWithKABC() const
{
	return d->photoSyncedWithKABC;
}

void MetaContact::setPhotoSyncedWithKABC(bool b)
{
	d->photoSyncedWithKABC=b;
	if( b && !loading() )
	{
		QVariant newValue;

		switch( photoSource() )
		{
			case SourceContact:
			{
				Contact *source = photoSourceContact();
				if(source != 0L)
					newValue = source->property( Kopete::Global::Properties::self()->photo() ).value();
				break;
			}
			case SourceCustom:
			{
				if( !d->customPicture.isNull() )
					newValue = d->customPicture.path();
				break;
			}
			// Don't sync the photo with KABC if the source is KABC !
			default:
				return;
		}

		if ( !d->metaContactId.isEmpty() && !newValue.isNull())
		{
			KABC::Addressee theAddressee = KABCPersistence::self()->addressBook()->findByUid( metaContactId() );

			if ( !theAddressee.isEmpty() )
			{
				QImage img;
				if(newValue.canConvert( QVariant::Image ))
					img=newValue.value<QImage>();
				else if(newValue.canConvert( QVariant::Pixmap ))
					img=newValue.value<QPixmap>().toImage();

				if(img.isNull())
				{
					// Some protocols like MSN save the photo as a url in
					// contact properties, we should not use this url
					// to sync with kabc but try first to embed the
					// photo data in the kabc addressee, because it could
					// be remote resource and the local url makes no sense
					QImage fallBackImage = QImage(newValue.toString());
					if(fallBackImage.isNull())
						theAddressee.setPhoto(newValue.toString());
					else
						theAddressee.setPhoto(fallBackImage);
				}
				else
					theAddressee.setPhoto(img);

				KABCPersistence::self()->addressBook()->insertAddressee(theAddressee);
				KABCPersistence::self()->writeAddressBook( theAddressee.resource() );
			}

		}
	}
}

QList<Contact *> MetaContact::contacts() const
{
	return d->contacts;
}
} //END namespace Kopete

#include "kopetemetacontact.moc"

// vim: set noet ts=4 sts=4 sw=4:
