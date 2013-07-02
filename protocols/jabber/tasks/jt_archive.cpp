/*
   Copyright (c) 2013 by Roman Nazarenko  <me@jtalk.me>

   Kopete    (c) 2008 by the Kopete developers <kopete-devel@kde.org>

   *************************************************************************
   *                                                                       *
   * This program is free software; you can redistribute it and/or modify  *
   * it under the terms of the GNU General Public License as published by  *
   * the Free Software Foundation; either version 2 of the License, or     *
   * (at your option) any later version.                                   *
   *                                                                       *
   *************************************************************************
*/

#include "jt_archive.h"

#include "xmpp_xmlcommon.h"
#include "xmpp_client.h"

const uint RSM_MAX = 100;

// This 'magic' just extracts QMetaEnum from an object's metainformation; looks awful, works great though.
const QMetaEnum JT_Archive::s_TagNamesEnum = JT_Archive::staticMetaObject.enumerator( JT_Archive::staticMetaObject.indexOfEnumerator( "TagNames" ) );
const QMetaEnum JT_Archive::s_AutoScopeEnum = JT_Archive::staticMetaObject.enumerator( JT_Archive::staticMetaObject.indexOfEnumerator( "AutoScope" ) );
const QMetaEnum JT_Archive::s_DefaultSaveEnum = JT_Archive::staticMetaObject.enumerator( JT_Archive::staticMetaObject.indexOfEnumerator( "DefaultSave" ) );
const QMetaEnum JT_Archive::s_DefaultOtrEnum = JT_Archive::staticMetaObject.enumerator( JT_Archive::staticMetaObject.indexOfEnumerator( "DefaultOtr" ) );
const QMetaEnum JT_Archive::s_MethodTypeEnum = JT_Archive::staticMetaObject.enumerator( JT_Archive::staticMetaObject.indexOfEnumerator( "MethodType" ) );
const QMetaEnum JT_Archive::s_MethodUseEnum = JT_Archive::staticMetaObject.enumerator( JT_Archive::staticMetaObject.indexOfEnumerator( "MethodUse" ) );

// Why have I set this to 'global'? Who knows, this thing just doesn't go anywhere right now, we ignore it.
const JT_Archive::AutoScope JT_Archive::defaultScope = JT_Archive::AutoScope_global;

// XMPP NS field for Archiving and Result Set Management stanzas
const QString JT_Archive::ArchivingNS = "urn:xmpp:archive";
const QString JT_Archive::ResultSetManagementNS = "http://jabber.org/protocol/rsm";

bool JT_Archive::hasArchivingNS(const QDomElement &e)
{
    return e.attribute("xmlns") == ArchivingNS;
}

JT_Archive::JT_Archive(Task *const parent)
    : Task(parent)
{
}

QString JT_Archive::requestPrefs()
{
    // We must request our server-side stored settings
    QDomElement request = uniformPrefsRequest();
    send(request);
    return request.attribute("id");
}

QString JT_Archive::requestCollections(const CollectionsRequest &params)
{
    QDomElement requestIq = uniformCollectionsRequest(params);
    send(requestIq);
    return requestIq.attribute("id");
}

QString JT_Archive::requestCollection(const JT_Archive::CollectionsRequest &params)
{
    QDomElement requestIq = uniformChatsRequest(params);
    send(requestIq);
    return requestIq.attribute("id");
}

QDomElement JT_Archive::uniformUpdate(const QDomElement &tag)
{
    QDomElement prefsUpdate = createIQ(doc(), "set", "", client()->genUniqueId());
    QDomElement prefTag = uniformArchivingNS("pref");
    prefTag.appendChild(tag);
    prefsUpdate.appendChild(prefTag);
    return prefsUpdate;
}

QDomElement JT_Archive::uniformArchivingNS(const QString &tagName)
{
    //return doc()->createElementNS(ArchivingNS, tagName);
    QDomElement tag = doc()->createElement(tagName);
    tag.setAttribute("xmlns", ArchivingNS);
    return tag;

}

QDomElement JT_Archive::uniformPrefsRequest()
{
    QDomElement prefsRequest = createIQ(doc(), "get", "", client()->genUniqueId());
    prefsRequest.appendChild( uniformArchivingNS("pref") );
    return prefsRequest;
}

static inline bool isPref(const QDomElement &elem)
{
    return elem.tagName() == "pref";
}

bool JT_Archive::handleSet(const QDomElement &wholeElement, const QDomElement &noIq, const QString &sessionID)
{
    Q_UNUSED(wholeElement)
    // Server pushes us new preferences, let's save them!
    // Server must send us 'set' requests only for new settings
    // pushing, so, if there's no <pref> tag inside <iq>, the
    // stanza is incorrect and should not be further processed.
    if (isPref(noIq)) {
        return writePrefs(noIq, sessionID);
    } else {
        return false;
    }
}

bool JT_Archive::handleGet(const QDomElement &wholeElement, const QDomElement &noIq, const QString &sessionID)
{
    Q_UNUSED(wholeElement)
    Q_UNUSED(sessionID)
    Q_UNUSED(noIq)
    // That's weird. Server is not supposed to send GET IQs
    return false;
}

static inline bool isList(const QDomElement &elem)
{
    return elem.tagName() == "list";
}

static inline bool isChat(const QDomElement &elem)
{
    return elem.tagName() == "chat";
}

bool JT_Archive::handleResult(const QDomElement &wholeElement, const QDomElement &noIq, const QString &sessionID)
{
    Q_UNUSED(wholeElement)
    if (isPref(noIq)) {
        return writePrefs(noIq, sessionID);
    } else if (isList(noIq)) {
        return collectionsListReceived(noIq, sessionID);
    } else if (isChat(noIq)) {
        return chatReceived(noIq, sessionID);
    } else return false;
}

bool JT_Archive::handleError(const QDomElement &wholeElement, const QDomElement &noIq, const QString &sessionID)
{
    Q_UNUSED(sessionID)
    Q_UNUSED(wholeElement)
    Q_UNUSED(noIq)
    return true;
}

static inline bool isIq(const QDomElement &e)
{
    return e.tagName() == "iq";
}

/**
 * Converting text from XML to enumerations.
 */
JT_Archive::AnswerHandler JT_Archive::chooseHandler(const QDomElement &e)
{
    // TODO: Fix this ugly mess somehow
    if (isIq(e)) {
        if (e.childNodes().isEmpty() && e.attribute("type") == "result") {
            return &JT_Archive::acknowledge;
        } else if (e.attribute("type") == "get") {
            return &JT_Archive::skip;
        } else if (e.attribute("type") == "set") {
            return &JT_Archive::handleSet;
        } else if (e.attribute("type") == "result") {
            return &JT_Archive::handleResult;
        } else if (e.attribute("type") == "error") {
            return &JT_Archive::handleError;
        }
    }
    return &JT_Archive::skip;
}

bool JT_Archive::take(const QDomElement &e)
{
    // TODO: Should we look through all tags instead?
    QDomElement internalTag = e.firstChild().toElement();
    QString id = e.attribute("id");
    if (hasArchivingNS(internalTag)) {
        // TODO: If we should do something on acknowledgement package receiving?
        return (this->*chooseHandler(e))(e, internalTag, id);
    } else return false;
}

static QString DateTimeToXMPP(const KDateTime &dt)
{
    //return TS2stamp(dt.dateTime());
    return dt.toString(KDateTime::RFC3339Date);
}

static KDateTime XMPPtoDateTime(const QString &dt)
{
    //return KDateTime(stamp2TS(dt));
    return KDateTime::fromString(dt, KDateTime::RFC3339Date);
}

QDomElement& JT_Archive::fillCollectionRequest(const CollectionsRequest &params, QDomElement &tag)
{
    if (params.start.isValid()) {
        tag.setAttribute("start", DateTimeToXMPP(params.start));
    }
    if (params.end.isValid()) {
        tag.setAttribute("end", DateTimeToXMPP(params.end));
    }
    tag.appendChild( uniformRsm(params.after) );
    return tag;
}

QDomElement JT_Archive::uniformCollectionsRequest(const CollectionsRequest &params)
{
    QDomElement listTag = uniformSkeletonCollectionsRequest(params.with);
    QDomElement &filledListTag = fillCollectionRequest(params, listTag);
    QDomElement iq = createIQ(doc(), "get", "", client()->genUniqueId());
    iq.appendChild(filledListTag);
    return iq;
}

QDomElement JT_Archive::uniformChatsRequest(const CollectionsRequest &params)
{
    QDomElement retrieveTag = uniformSkeletonChatsRequest(params.with);
    QDomElement &filledRetrieveTag = fillCollectionRequest(params, retrieveTag);
    QDomElement iq = createIQ(doc(), "get", "", client()->genUniqueId());
    iq.appendChild(filledRetrieveTag);
    return iq;
}

QDomElement JT_Archive::uniformRsmNS(const QString &tagName)
{
    //return doc()->createElementNS(ResultSetManagementNS, tagName);
    QDomElement tag = doc()->createElement(tagName);
    tag.setAttribute("xmlns", ResultSetManagementNS);
    return tag;
}

QDomElement JT_Archive::uniformRsmMax(uint max)
{
    QDomElement maxTag = doc()->createElement("max");
    QDomText internals = doc()->createTextNode(QString::number(max));
    maxTag.appendChild(internals);
    return maxTag;
}

QDomElement JT_Archive::uniformSkeletonCollectionsRequest(const QString &with)
{
    QDomElement skeleton = uniformArchivingNS("list");
    if (!with.isEmpty()) {
        skeleton.setAttribute("with", with);
    }
    return skeleton;
}

QDomElement JT_Archive::uniformSkeletonChatsRequest(const QString &with)
{
    QDomElement skeleton = uniformArchivingNS("retrieve");
    if (!with.isEmpty()) {
        skeleton.setAttribute("with", with);
    }
    return skeleton;
}

QDomElement JT_Archive::uniformRsm(const QString &after)
{
    QDomElement resultSetManagement = uniformRsmNS("set");
    resultSetManagement.appendChild( uniformRsmMax(RSM_MAX) );
    if (!after.isEmpty()) {
        QDomElement afterTag = doc()->createElement("after");
        afterTag.appendChild( doc()->createTextNode(after) );
        resultSetManagement.appendChild(afterTag);
    }
    return resultSetManagement;
}

static inline QString capitalize(const QString &str)
{
    QString lowerStr = str.toLower();
    lowerStr[0] = lowerStr[0].toUpper();
    return lowerStr;
}

static inline bool verifyAutoTag(const QDomElement &autoTag)
{
    return autoTag.childNodes().isEmpty()
            && autoTag.attributes().contains("save");
}

static inline bool hasScope(const QDomElement &autoTag)
{
    return autoTag.attributes().contains("scope");
}

#define STR(x) # x
#define STRCAT(x,y) QString(QString(x) + "_" + QString(y))
#define EXTRACT_TAG(type, tag, name) (type)s_ ## type ## Enum.keyToValue(STRCAT(STR(type) \
    , tag.attribute(name)).toAscii())

bool JT_Archive::handleAutoTag(const QDomElement &autoTag, const QString &id)
{
    // This will return either received scope or static defaultScope variable,
    // if no correct scope is received;
    AutoScope scope = EXTRACT_TAG(AutoScope, autoTag, "scope");
    if (scope == -1) {
        scope = defaultScope;
    }
    // <auto> Must be an empty tag and must contain 'save' attribute
    // according to the current standard draft.
    // If there was no scope, 'scope' variable would be defaultScope, which is
    // valid.
    if (verifyAutoTag(autoTag)) {
        bool isAutoArchivingEnabled = QVariant(autoTag.attribute("save")).toBool();
        emit automaticArchivingEnable(isAutoArchivingEnabled, id, scope);
        return true;
    } else {
        return false;
    }
}

static inline bool verifyDefaultTag(const QDomElement &defaultTag)
{
    return defaultTag.childNodes().isEmpty()
            && defaultTag.attributes().contains("save")
            && defaultTag.attributes().contains("otr");
}

bool JT_Archive::handleDefaultTag(const QDomElement &defaultTag, const QString &id)
{
    DefaultSave saveMode = EXTRACT_TAG(DefaultSave, defaultTag, "save");
    DefaultOtr otrMode = EXTRACT_TAG(DefaultOtr, defaultTag, "otr");
    // <default> Must be an empty tag and must contain 'save' and 'otr' attributes
    // according to the current standard draft.
    if (verifyDefaultTag(defaultTag) && saveMode != -1 && otrMode != -1) {
        uint expire = defaultTag.attributes().contains("expire")
                ? QVariant(defaultTag.attribute("scope")).toUInt()
                : defaultExpiration;
        emit defaultPreferenceChanged(saveMode, otrMode, id, expire);
        return true;
    } else {
        return false;
    }
}

bool JT_Archive::handleItemTag(const QDomElement &, const QString &id)
{
    Q_UNUSED(id)
    // TODO: Implement per-user storing settings
    return true;
}

bool JT_Archive::handleSessionTag(const QDomElement &, const QString &id)
{
    Q_UNUSED(id)
    // TODO: Implement per-session storing settings
    return true;
}

static inline bool verifyMethodTag(const QDomElement &tag) {
    return tag.childNodes().isEmpty()
            && tag.attributes().contains("type")
            && tag.attributes().contains("use");
}

bool JT_Archive::handleMethodTag(const QDomElement &methodTag, const QString &id)
{
    MethodType method = EXTRACT_TAG(MethodType, methodTag, "type");
    MethodUse use = EXTRACT_TAG(MethodUse, methodTag, "use");
    if (verifyMethodTag(methodTag) && method != -1 && use != -1) {
        emit archivingMethodChanged(method, use, id);
        return true;
    } else return false;
}

static QDomElement findRSMTag(const QDomElement &tag, bool *found)
{
    QDomElement rsmTag = tag.tagName() == "set" ? tag : findSubTag(tag, "set", 0);
    if (found) {
        *found = rsmTag.attribute("xmlns") == JT_Archive::ResultSetManagementNS;
    }
    return rsmTag;
}

JT_Archive::RSMInfo JT_Archive::parseRSM(const QDomElement &elem)
{
    bool found = false;
    QDomElement rsmTag = findRSMTag(elem, &found);
    if (found) {
        RSMInfo info;
        info.count = findSubTag(rsmTag, "count", 0).text().toUInt();
        info.first = findSubTag(rsmTag, "first", 0).text();
        info.last = findSubTag(rsmTag, "last", 0).text();
        return info;
    } else {
        return RSMInfo();
    }
}

#define DOM_FOREACH(var, domElement) for(QDomNode var = domElement.firstChild(); !var.isNull(); var = var.nextSibling())

static QList<JT_Archive::ChatInfo> parseChatsInfo(const QDomElement &tags)
{
    QList<JT_Archive::ChatInfo> list;
    DOM_FOREACH(chatTag, tags) {
        QDomElement currentElem = chatTag.toElement();
        if (currentElem.tagName() == "chat") {
            JT_Archive::ChatInfo info;
            info.with = currentElem.attribute("with");
            info.time = XMPPtoDateTime( currentElem.attribute("start") );
            list.append(info);
        } else continue;
    }
    return list;
}

bool JT_Archive::writePrefs(const QDomElement &tags, const QString &id)
{
    bool isSuccessful = true;
    DOM_FOREACH(tag, tags) {
        bool isCurrentSuccessful = writePref(tag.toElement(), id);
        isSuccessful = isSuccessful && isCurrentSuccessful;
    }
    return isSuccessful;
}

bool JT_Archive::writePref(const QDomElement &elem, const QString &id)
{
    TagNames tagName = (TagNames)s_TagNamesEnum.keyToValue(capitalize(elem.tagName()).toAscii());
    switch (tagName) {
    case Auto: return handleAutoTag(elem, id);
    case Default: return handleDefaultTag(elem, id);
    case Item: return handleItemTag(elem, id);
    case Session: return handleSessionTag(elem, id);
    case Method: return handleMethodTag(elem, id);
    default: return false;
    }
}

bool JT_Archive::collectionsListReceived(const QDomElement &listTag, const QString &id)
{
    RSMInfo rsmInfo = parseRSM(listTag);
    QList<ChatInfo> list = parseChatsInfo(listTag);
    emit collectionsReceived(list, rsmInfo, id);
    return true;
}

static inline bool isChatTag(const QDomElement &tag)
{
    return tag.tagName() == "chat";
}

static inline KDateTime fromSeconds(quint64 seconds)
{
    return KDateTime(QDateTime::fromMSecsSinceEpoch(seconds * 1000));
}

static QList<JT_Archive::ChatItem> parseChatItems(const QDomElement &tags)
{
    QList<JT_Archive::ChatItem> list;
    list.reserve(tags.childNodes().count());
    DOM_FOREACH(tag, tags) {
        QDomElement currentTag = tag.toElement();
        if (currentTag.tagName() == "to" || currentTag.tagName() == "from") {
            JT_Archive::ChatItem item;
            item.isIncoming = currentTag.tagName() != "to";
            item.time = fromSeconds(currentTag.attribute("utc_secs").toULongLong());
            item.body = currentTag.text();
            list.append(item);
        }
    }
    return list;
}

bool JT_Archive::chatReceived(const QDomElement &retrieveTag, const QString &id)
{
    RSMInfo rsmInfo = parseRSM(retrieveTag);
    QList<ChatItem> list = parseChatItems(retrieveTag);
    emit chatReceived(list, rsmInfo, id);
    return true;
}

#define VALUE(type, value) QString( s_ ## type ## Enum.valueToKey(value) ).remove(QLatin1String(# type) + "_")
#define INSERT_TAG(type, tag, name, value) tag.setAttribute(name, VALUE(type, value))

void JT_Archive::updateDefault(const JT_Archive::DefaultSave save,
                               const JT_Archive::DefaultOtr otr,
                               const uint expiration)
{
    Q_UNUSED(expiration)
    QDomElement tag = doc()->createElement("default");
    INSERT_TAG(DefaultSave, tag, "save", save);
    INSERT_TAG(DefaultOtr, tag, "otr", otr);
    QDomElement update = uniformUpdate(tag);
    send(update);
}

void JT_Archive::updateAuto(bool isEnabled, const JT_Archive::AutoScope scope)
{
    QDomElement tag = doc()->createElement("auto");
    tag.setAttribute("save", QVariant(isEnabled).toString());
    if (scope != -1) {
        INSERT_TAG(AutoScope, tag, "scope", scope);
    }
    QDomElement update = uniformUpdate(tag);
    send(update);
}

void JT_Archive::updateStorage(const JT_Archive::MethodType method, const JT_Archive::MethodUse use)
{
    QDomElement tag = doc()->createElement("method");
    INSERT_TAG(MethodType, tag, "type", method);
    INSERT_TAG(MethodUse, tag, "use", use);
    QDomElement update = uniformUpdate(tag);
    send(update);
}

