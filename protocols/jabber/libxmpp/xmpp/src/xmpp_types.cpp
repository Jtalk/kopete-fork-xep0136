/*
 * types.h - classes for handling Jabber datatypes
 * Copyright (C) 2001, 2002  Justin Karneges
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include"xmpp_types.h"

#include<qdom.h>
#include<qobject.h>

// used in FeatureName class
#include<qmap.h>
#include<qapplication.h>

#include"xmpp_xmlcommon.h"

using namespace Jabber;


//---------------------------------------------------------------------------
// Subscription
//---------------------------------------------------------------------------
Subscription::Subscription(SubType type)
{
	value = type;
}

int Subscription::type() const
{
	return value;
}

QString Subscription::toString() const
{
	switch(value) {
		case Remove:
			return "remove";
		case Both:
			return "both";
		case From:
			return "from";
		case To:
			return "to";
		case None:
		default:
			return "none";
	}
}

bool Subscription::fromString(const QString &s)
{
	if(s == "remove")
		value = Remove;
	else if(s == "both")
		value = Both;
	else if(s == "from")
		value = From;
	else if(s == "to")
		value = To;
	else if(s == "none")
		value = None;
	else
		return false;

	return true;
}


//---------------------------------------------------------------------------
// Status
//---------------------------------------------------------------------------
Status::Status(const QString &show, const QString &status, int priority, bool available)
{
	v_isAvailable = available;
	v_show = show;
	v_status = status;
	v_priority = priority;
	v_timeStamp = QDateTime::currentDateTime();
	v_isInvisible = false;
	ecode = -1;
}

Status::~Status()
{
}

bool Status::hasError() const
{
	return (ecode != -1);
}

void Status::setError(int code, const QString &str)
{
	ecode = code;
	estr = str;
}

void Status::setIsAvailable(bool available)
{
	v_isAvailable = available;
}

void Status::setIsInvisible(bool invisible)
{
	v_isInvisible = invisible;
}

void Status::setPriority(int x)
{
	v_priority = x;
}

void Status::setShow(const QString & _show)
{
	v_show = _show;
}

void Status::setStatus(const QString & _status)
{
	v_status = _status;
}

void Status::setTimeStamp(const QDateTime & _timestamp)
{
	v_timeStamp = _timestamp;
}

void Status::setKeyID(const QString &key)
{
	v_key = key;
}

void Status::setXSigned(const QString &s)
{
	v_xsigned = s;
}

void Status::setSongTitle(const QString & _songtitle)
{
	v_songTitle = _songtitle;
}

bool Status::isAvailable() const
{
	return v_isAvailable;
}

bool Status::isAway() const
{
	if(v_show == "away" || v_show == "xa" || v_show == "dnd")
		return true;

	return false;
}

bool Status::isInvisible() const
{
	return v_isInvisible;
}

int Status::priority() const
{
	return v_priority;
}

const QString & Status::show() const
{
	return v_show;
}
const QString & Status::status() const
{
	return v_status;
}

QDateTime Status::timeStamp() const
{
	return v_timeStamp;
}

const QString & Status::keyID() const
{
	return v_key;
}

const QString & Status::xsigned() const
{
	return v_xsigned;
}

const QString & Status::songTitle() const
{
	return v_songTitle;
}

int Status::errorCode() const
{
	return ecode;
}

const QString & Status::errorString() const
{
	return estr;
}


//---------------------------------------------------------------------------
// Resource
//---------------------------------------------------------------------------
Resource::Resource(const QString &name, const Status &stat)
{
	v_name = name;
	v_status = stat;
}

Resource::~Resource()
{
}

const QString & Resource::name() const
{
	return v_name;
}

int Resource::priority() const
{
	return v_status.priority();
}

const Status & Resource::status() const
{
	return v_status;
}

void Resource::setName(const QString & _name)
{
	v_name = _name;
}

void Resource::setStatus(const Status & _status)
{
	v_status = _status;
}


//---------------------------------------------------------------------------
// ResourceList
//---------------------------------------------------------------------------
ResourceList::ResourceList()
:QValueList<Resource>()
{
}

ResourceList::~ResourceList()
{
}

ResourceList::Iterator ResourceList::find(const QString & _find)
{
	for(ResourceList::Iterator it = begin(); it != end(); ++it) {
		if((*it).name() == _find)
			return it;
	}

	return end();
}

ResourceList::Iterator ResourceList::priority()
{
	ResourceList::Iterator highest = end();

	for(ResourceList::Iterator it = begin(); it != end(); ++it) {
		if(highest == end() || (*it).priority() > (*highest).priority())
			highest = it;
	}

	return highest;
}

ResourceList::ConstIterator ResourceList::find(const QString & _find) const
{
	for(ResourceList::ConstIterator it = begin(); it != end(); ++it) {
		if((*it).name() == _find)
			return it;
	}

	return end();
}

ResourceList::ConstIterator ResourceList::priority() const
{
	ResourceList::ConstIterator highest = end();

	for(ResourceList::ConstIterator it = begin(); it != end(); ++it) {
		if(highest == end() || (*it).priority() > (*highest).priority())
			highest = it;
	}

	return highest;
}


//---------------------------------------------------------------------------
// RosterItem
//---------------------------------------------------------------------------
RosterItem::RosterItem(const Jid &_jid)
{
	v_jid = _jid;
}

RosterItem::~RosterItem()
{
}

const Jid & RosterItem::jid() const
{
	return v_jid;
}

const QString & RosterItem::name() const
{
	return v_name;
}

const QStringList & RosterItem::groups() const
{
	return v_groups;
}

const Subscription & RosterItem::subscription() const
{
	return v_subscription;
}

const QString & RosterItem::ask() const
{
	return v_ask;
}

bool RosterItem::isPush() const
{
	return v_push;
}

bool RosterItem::inGroup(const QString &g) const
{
	for(QStringList::ConstIterator it = v_groups.begin(); it != v_groups.end(); ++it) {
		if(*it == g)
			return true;
	}
	return false;
}

void RosterItem::setJid(const Jid &_jid)
{
	v_jid = _jid;
}

void RosterItem::setName(const QString &_name)
{
	v_name = _name;
}

void RosterItem::setGroups(const QStringList &_groups)
{
	v_groups = _groups;
}

void RosterItem::setSubscription(const Subscription &type)
{
	v_subscription = type;
}

void RosterItem::setAsk(const QString &_ask)
{
	v_ask = _ask;
}

void RosterItem::setIsPush(bool b)
{
	v_push = b;
}

bool RosterItem::addGroup(const QString &g)
{
	if(inGroup(g))
		return false;

	v_groups += g;
	return true;
}

bool RosterItem::removeGroup(const QString &g)
{
	for(QStringList::Iterator it = v_groups.begin(); it != v_groups.end(); ++it) {
		if(*it == g) {
			v_groups.remove(it);
			return true;
		}
	}

	return false;
}

QDomElement RosterItem::toXml(QDomDocument *doc) const
{
	QDomElement item = doc->createElement("item");
	item.setAttribute("jid", v_jid.full());
	item.setAttribute("name", v_name);
	item.setAttribute("subscription", v_subscription.toString());
	if(!v_ask.isEmpty())
		item.setAttribute("ask", v_ask);
	for(QStringList::ConstIterator it = v_groups.begin(); it != v_groups.end(); ++it)
		item.appendChild(textTag(doc, "group", *it));

	return item;
}

bool RosterItem::fromXml(const QDomElement &item)
{
	if(item.tagName() != "item")
		return false;
	Jid j(item.attribute("jid"));
	if(!j.isValid())
		return false;
	QString na = item.attribute("name");
	Subscription s;
	if(!s.fromString(item.attribute("subscription")) )
		return false;
	QStringList g;
	for(QDomNode n = item.firstChild(); !n.isNull(); n = n.nextSibling()) {
		QDomElement i = n.toElement();
		if(i.isNull())
			continue;
		if(i.tagName() == "group")
			g += tagContent(i);
	}
	QString a = item.attribute("ask");

	v_jid = j;
	v_name = na;
	v_subscription = s;
	v_groups = g;
	v_ask = a;

	return true;
}


//---------------------------------------------------------------------------
// Roster
//---------------------------------------------------------------------------
Roster::Roster()
:QValueList<RosterItem>()
{
}

Roster::~Roster()
{
}

Roster::Iterator Roster::find(const Jid &j)
{
	for(Roster::Iterator it = begin(); it != end(); ++it) {
		if((*it).jid().compare(j))
			return it;
	}

	return end();
}

Roster::ConstIterator Roster::find(const Jid &j) const
{
	for(Roster::ConstIterator it = begin(); it != end(); ++it) {
		if((*it).jid().compare(j))
			return it;
	}

	return end();
}


//---------------------------------------------------------------------------
// FormField
//---------------------------------------------------------------------------
FormField::FormField(const QString &type, const QString &value)
{
	v_type = misc;
	if(!type.isEmpty()) {
		int x = tagNameToType(type);
		if(x != -1)
			v_type = x;
	}
	v_value = value;
}

FormField::~FormField()
{
}

int FormField::type() const
{
	return v_type;
}

QString FormField::realName() const
{
	return typeToTagName(v_type);
}

QString FormField::fieldName() const
{
	switch(v_type) {
		case username:  return QObject::tr("Username");
		case nick:      return QObject::tr("Nickname");
		case password:  return QObject::tr("Password");
		case name:      return QObject::tr("Name");
		case first:     return QObject::tr("First Name");
		case last:      return QObject::tr("Last Name");
		case email:     return QObject::tr("E-mail");
		case address:   return QObject::tr("Address");
		case city:      return QObject::tr("City");
		case state:     return QObject::tr("State");
		case zip:       return QObject::tr("Zipcode");
		case phone:     return QObject::tr("Phone");
		case url:       return QObject::tr("URL");
		case date:      return QObject::tr("Date");
		case misc:      return QObject::tr("Misc");
		default:        return "";
	};
}

bool FormField::isSecret() const
{
	return (type() == password);
}

const QString & FormField::value() const
{
	return v_value;
}

void FormField::setType(int x)
{
	v_type = x;
}

bool FormField::setType(const QString &in)
{
	int x = tagNameToType(in);
	if(x == -1)
		return false;

	v_type = x;
	return true;
}

void FormField::setValue(const QString &in)
{
	v_value = in;
}

int FormField::tagNameToType(const QString &in) const
{
	if(!in.compare("username")) return username;
	if(!in.compare("nick"))     return nick;
	if(!in.compare("password")) return password;
	if(!in.compare("name"))     return name;
	if(!in.compare("first"))    return first;
	if(!in.compare("last"))     return last;
	if(!in.compare("email"))    return email;
	if(!in.compare("address"))  return address;
	if(!in.compare("city"))     return city;
	if(!in.compare("state"))    return state;
	if(!in.compare("zip"))      return zip;
	if(!in.compare("phone"))    return phone;
	if(!in.compare("url"))      return url;
	if(!in.compare("date"))     return date;
	if(!in.compare("misc"))     return misc;

	return -1;
}

QString FormField::typeToTagName(int type) const
{
	switch(type) {
		case username:  return "username";
		case nick:      return "nick";
		case password:  return "password";
		case name:      return "name";
		case first:     return "first";
		case last:      return "last";
		case email:     return "email";
		case address:   return "address";
		case city:      return "city";
		case state:     return "state";
		case zip:       return "zipcode";
		case phone:     return "phone";
		case url:       return "url";
		case date:      return "date";
		case misc:      return "misc";
		default:        return "";
	};
}


//---------------------------------------------------------------------------
// Form
//---------------------------------------------------------------------------
Form::Form(const Jid &j)
:QValueList<FormField>()
{
	setJid(j);
}

Form::~Form()
{
}

Jid Form::jid() const
{
	return v_jid;
}

QString Form::instructions() const
{
	return v_instructions;
}

QString Form::key() const
{
	return v_key;
}

void Form::setJid(const Jid &j)
{
	v_jid = j;
}

void Form::setInstructions(const QString &s)
{
	v_instructions = s;
}

void Form::setKey(const QString &s)
{
	v_key = s;
}


//---------------------------------------------------------------------------
// SearchResult
//---------------------------------------------------------------------------
SearchResult::SearchResult(const Jid &jid)
{
	setJid(jid);
}

SearchResult::~SearchResult()
{
}

const Jid & SearchResult::jid() const
{
	return v_jid;
}

const QString & SearchResult::nick() const
{
	return v_nick;
}

const QString & SearchResult::first() const
{
	return v_first;
}

const QString & SearchResult::last() const
{
	return v_last;
}

const QString & SearchResult::email() const
{
	return v_email;
}

void SearchResult::setJid(const Jid &jid)
{
	v_jid = jid;
}

void SearchResult::setNick(const QString &nick)
{
	v_nick = nick;
}

void SearchResult::setFirst(const QString &first)
{
	v_first = first;
}

void SearchResult::setLast(const QString &last)
{
	v_last = last;
}

void SearchResult::setEmail(const QString &email)
{
	v_email = email;
}

//---------------------------------------------------------------------------
// Features
//---------------------------------------------------------------------------

Features::Features()
{
}

Features::Features(const QStringList &l)
{
	setList(l);
}

Features::Features(const QString &str)
{
	QStringList l;
	l << str;

	setList(l);
}

Features::~Features()
{
}

QStringList Features::list() const
{
	return _list;
}

void Features::setList(const QStringList &l)
{
	_list = l;
}

bool Features::test(const QStringList &ns) const
{
	QStringList::ConstIterator it = ns.begin();
	for ( ; it != ns.end(); ++it)
		if ( _list.find( *it ) != _list.end() )
			return true;

	return false;
}

#define FID_REGISTER "jabber:iq:register"
bool Features::canRegister() const
{
	QStringList ns;
	ns << FID_REGISTER;

	return test(ns);
}

#define FID_SEARCH "jabber:iq:search"
bool Features::canSearch() const
{
	QStringList ns;
	ns << FID_SEARCH;

	return test(ns);
}

#define FID_GROUPCHAT "jabber:iq:conference"
bool Features::canGroupchat() const
{
	QStringList ns;
	ns << "http://jabber.org/protocol/muc";
	ns << FID_GROUPCHAT;

	return test(ns);
}

#define FID_GATEWAY "jabber:iq:gateway"
bool Features::isGateway() const
{
	QStringList ns;
	ns << FID_GATEWAY;

	return test(ns);
}

#define FID_DISCO "http://jabber.org/protocol/disco"
bool Features::canDisco() const
{
	QStringList ns;
	ns << FID_DISCO;
	ns << "http://jabber.org/protocol/disco#info";
	ns << "http://jabber.org/protocol/disco#items";

	return test(ns);
}

#define FID_VCARD "vcard-temp"
bool Features::haveVCard() const
{
	QStringList ns;
	ns << FID_VCARD;

	return test(ns);
}

class Features::FeatureName : public QObject
{
	Q_OBJECT
public:
	FeatureName()
	: QObject(qApp)
	{
		id2s[FID_Invalid]	= tr("ERROR: Incorrect usage of Features class");
		id2s[FID_None]		= tr("None");
		id2s[FID_Register]	= tr("Register");
		id2s[FID_Search]	= tr("Search");
		id2s[FID_Groupchat]	= tr("Groupchat");
		id2s[FID_Gateway]	= tr("Gateway");
		id2s[FID_Disco]		= tr("Service Discovery");
		id2s[FID_VCard]		= tr("VCard");

		// compute reverse map
		//QMap<QString, long>::Iterator it = id2s.begin();
		//for ( ; it != id2s.end(); ++it)
		//	s2id[it.data()] = it.key();

		id2f[FID_Register]	= FID_REGISTER;
		id2f[FID_Search]	= FID_SEARCH;
		id2f[FID_Groupchat]	= FID_GROUPCHAT;
		id2f[FID_Gateway]	= FID_GATEWAY;
		id2f[FID_Disco]		= FID_DISCO;
		id2f[FID_VCard]		= FID_VCARD;
	}

	//QMap<QString, long> s2id;
	QMap<long, QString> id2s;
	QMap<long, QString> id2f;
};

static Features::FeatureName *featureName = 0;

long Features::id() const
{
	if ( _list.count() > 1 )
		return FID_Invalid;
	else if ( canRegister() )
		return FID_Register;
	else if ( canSearch() )
		return FID_Search;
	else if ( canGroupchat() )
		return FID_Groupchat;
	else if ( isGateway() )
		return FID_Gateway;
	else if ( canDisco() )
		return FID_Disco;
	else if ( haveVCard() )
		return FID_VCard;

	return FID_None;
}

long Features::id(const QString &feature)
{
	Features f(feature);
	return f.id();
}

QString Features::feature(long id)
{
	if ( !featureName )
		featureName = new FeatureName();

	return featureName->id2f[id];
}

QString Features::name(long id)
{
	if ( !featureName )
		featureName = new FeatureName();

	return featureName->id2s[id];
}

QString Features::name() const
{
	return name(id());
}

QString Features::name(const QString &feature)
{
	Features f(feature);
	return f.name(f.id());
}

//---------------------------------------------------------------------------
// DiscoItem
//---------------------------------------------------------------------------
class DiscoItem::Private
{
public:
	Private()
	{
		action = None;
	}

	Jid jid;
	QString name;
	QString node;
	Action action;

	Features features;
	Identities identities;
};

DiscoItem::DiscoItem()
{
	d = new Private;
}

DiscoItem::DiscoItem(const DiscoItem &from)
{
	d = new Private;
	*this = from;
}

DiscoItem & DiscoItem::operator= (const DiscoItem &from)
{
	d->jid = from.d->jid;
	d->name = from.d->name;
	d->node = from.d->node;
	d->action = from.d->action;
	d->features = from.d->features;
	d->identities = from.d->identities;

	return *this;
}

DiscoItem::~DiscoItem()
{
	delete d;
}

AgentItem DiscoItem::toAgentItem() const
{
	AgentItem ai;

	ai.setJid( jid() );
	ai.setName( name() );

	Identity id;
	if ( !identities().isEmpty() )
		id = identities().first();

	ai.setCategory( id.category );
	ai.setType( id.type );

	ai.setFeatures( d->features );

	return ai;
}

void DiscoItem::fromAgentItem(const AgentItem &ai)
{
	setJid( ai.jid() );
	setName( ai.name() );

	Identity id;
	id.category = ai.category();
	id.type = ai.type();
	id.name = ai.name();

	Identities idList;
	idList << id;

	setIdentities( idList );

	setFeatures( ai.features() );
}

const Jid &DiscoItem::jid() const
{
	return d->jid;
}

void DiscoItem::setJid(const Jid &j)
{
	d->jid = j;
}

const QString &DiscoItem::name() const
{
	return d->name;
}

void DiscoItem::setName(const QString &n)
{
	d->name = n;
}

const QString &DiscoItem::node() const
{
	return d->node;
}

void DiscoItem::setNode(const QString &n)
{
	d->node = n;
}

DiscoItem::Action DiscoItem::action() const
{
	return d->action;
}

void DiscoItem::setAction(Action a)
{
	d->action = a;
}

const Features &DiscoItem::features() const
{
	return d->features;
}

void DiscoItem::setFeatures(const Features &f)
{
	d->features = f;
}

const DiscoItem::Identities &DiscoItem::identities() const
{
	return d->identities;
}

void DiscoItem::setIdentities(const Identities &i)
{
	d->identities = i;

	if ( name().isEmpty() && i.count() )
		setName( i.first().name );
}


DiscoItem::Action DiscoItem::string2action(QString s)
{
	Action a;

	if ( s == "update" )
		a = Update;
	else if ( s == "remove" )
		a = Remove;
	else
		a = None;

	return a;
}

QString DiscoItem::action2string(Action a)
{
	QString s;

	if ( a == Update )
		s = "update";
	else if ( a == Remove )
		s = "remove";
	else
		s = QString::null;

	return s;
}

#include "xmpp_types.moc"
