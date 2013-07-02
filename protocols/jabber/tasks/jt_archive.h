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

/**
 * This is an XMPP extension protocol 0136 implementation (Message Archiving)
 * task for Kopete.
 * See http://xmpp.org/extensions/xep-0136.html for further details.
 */

#ifndef JT_ARCHIVE_H
#define JT_ARCHIVE_H

#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QPair>
#include <QtCore/QVariant>
#include <QtCore/QList>
#include <QtCore/QMetaEnum>
#include <QtCore/QMetaObject>
#include <QtXml/QDomElement>

#include <KDateTime>

#include "xmpp_task.h"
#include "xmpp_xdata.h"

/**
 * @brief The JT_Archive class is an Iris task for the XMPP extension #0136, Message archiving.
 * @author Roman Nazarenko
 *
 * One can create an instance to either check or modify archiving preferences.
 * Since all XEP-0136 preferences are stored on the server, we need no resident
 * module for this extension.
 *
 * Because of this, I marked 'static' as much methods as possible -- minimal
 * on-create overhead.
 *
 * Since XEP-0136 is multipurposal, XMPP::Task::onGo() is not implemented. Instead,
 * we have several methods performing appropriate tasks.
 *
 * This class uses Qt meta system for enum-to-string conversions, so, enumerations'
 * naming is sensitive and one ought to change anything carefuly. At first, we used
 * to handle those conversions manually, but dozens of 'toSomeCrap' and 'fromSomeCrap'
 * methods always make me sick.
 *
 * As any other Iris task, this class must be initialized with a root task pointer,
 * but you'd probably use the object provided by jabber client, such as:
 * @example
 *      JT_Archive *archiving = account()->client()->archivingManager();
 *
 * For concurrency support, this class uses Qt's signal-slot mechanism to receive
 * remote server's answeres. For example, let's request our preferences using the archiving
 * class pointer allocated above:
 * @example
 *      connect(archiving, SIGNAL(automaticArchivingEnable(bool,JT_Archive::AutoScope)),
 *              SLOT(someSlot(bool)));
 *      archiving->requestPrefs();
 *
 * Now, when server answers, you can find out whether archiving management is enabled for
 * this contact on this server using someSlot():
 * @example
 *      void someSlot(bool isEnabled) {
 *          m_archivingEnabled = isEnabled;
 *      }
 *
 * Some parameters were implemented for standard compliance only and are ignored, read signals
 * and/or request methods comments for more information.
 *
 * You may notice enumerations are named strangely, but there is a cause. I'm using QMetaEnum
 * for conversions between XML string literals and C++ enumerations. At first I used to
 * handle this via capitalization: AutoScopeGlobal, for example. But I found out some servers
 * (Prosody, for example, with its old mod_archive extension) are not fully standard-compliant,
 * allowing us to add capitalized attributes to users' preferences, such as
 * <method use=Concede type=Local/>, which is forbidden by the current standard draft and leads
 * to methods duplication and unavailability of proper preferences. So I decided to move
 * to the underlined enumerations, and I have been having no troubles with those enums since.
 *
 * WARNING: One should not leave this object to be created longer than needed, or ever create one
 * if client's archivingManager is available. since Iris tasks are made like hell of a crap,
 * and only first task presented will receive an incoming stanza.
 *
 * WARNING OTR negotiation is not supported, so I'm not really sure whether using this class
 * with OTR is safe, some investigation and testing is needed.
 */
class JT_Archive : public XMPP::Task
{
    Q_OBJECT

    /// Qt meta system macros. Those make enumerations available through Qt's
    /// QMetaEnum, allowing us to extract string names from enumerational values
    /// and vice versa.
    Q_ENUMS( TagNames )
    Q_ENUMS( AutoScope )
    Q_ENUMS( DefaultSave )
    Q_ENUMS( DefaultOtr )
    Q_ENUMS( MethodType )
    Q_ENUMS( MethodUse )
public:
    /**
     * @brief ArchivingNS is an XML namespace title for XEP-0136.
     *
     * This variable is used in take() method to check if DOM element received
     * must be handled by this class. This is also used in uniformArchivingNS()
     * for proper outgoing stanzas creation.
     *
     * One is going to extend this class should use this variable in
     * all the outgoing stanzas, according to the current draft, as, for example:
     *      <iq type='set'>
     *          <pref xmlns=#ArchivingNS>
     *              <method type=local use=concede/>
     *          </pref>
     *      </iq>
     */
    static const QString ArchivingNS; // urn:xmpp:archive

    /**
     * @brief ResultSetManagementNS keeps XEP-0059 XML namespace.
     *
     * Use cases are obvious: tag <set/> with this namespace will be converted
     * into JT_Archive::RSMInfo structure, letting programmers to handle
     * resources offsets and counting.
     *
     * This method would be used like:
     *      <iq type='get'>
     *          <list with="some@contact.from">
     *              <set xmlns=#ResultSetManagementNS>
     *                  <max>30</max>
     *              </set>
     *          </list>
     *      </iq>
     */
    static const QString ResultSetManagementNS; // http://jabber.org/protocol/rsm

    /**
     * WARNING: Only element itself is checked, child ones are not.
     */
    static bool hasArchivingNS(const QDomElement&);

    /// Possible tag names for XEP-0136 preferences.
    enum TagNames {
        Auto,
        Default,
        Method,
        Item,
        Session
    };

    /// Scope of archiving setting: forever or for a current stream only.
    enum AutoScope {
        AutoScope_global,
        AutoScope_stream
    };
    /// It's set to AutoScope_global by default.
    static const AutoScope defaultScope;

    /// Which part of the messaging stream should we store?
    enum DefaultSave {
        DefaultSave_false,   // Nothing
        DefaultSave_body,    // <body/>
        DefaultSave_message, // <message/>
        DefaultSave_stream   // The whole stream, byte-to-byte
    };

    /// How do we proceed Off-the-Record?
    /// This task does not support OTR now, mainly because I
    /// know nothing about it.
    /// TODO: Implement OTR support.
    enum DefaultOtr {
        DefaultOtr_concede,
        DefaultOtr_prefer,
        DefaultOtr_forbid,
        DefaultOtr_approve,
        DefaultOtr_oppose,
        DefaultOtr_require
    };

    /// Where and how hard does user prefer to store archive?
    /// This task doesn't support Manual storing.
    /// TODO: Implement manual storing
    enum MethodType {
        MethodType_auto,     // Server saves everything itself
        MethodType_local,    // Everything goes to the local storage, nothing to a server
        MethodType_manual    // Client will send messages to archive manually.
    };
    enum MethodUse {
        MethodUse_concede,   // Use if no other options available
        MethodUse_prefer,
        MethodUse_forbid
    };
    /// Special way to say 'forever'?
    static const uint defaultExpiration = (uint)-1;

    /// Enumerations meta information. We use those variables for str-to-enum
    /// conversions. Changing their names is not permitted, since they're
    /// related to enumerations' names via macroses (see jt_archive.cpp).
    static const QMetaEnum s_TagNamesEnum;
    static const QMetaEnum s_AutoScopeEnum;
    static const QMetaEnum s_DefaultSaveEnum;
    static const QMetaEnum s_DefaultOtrEnum;
    static const QMetaEnum s_MethodTypeEnum;
    static const QMetaEnum s_MethodUseEnum;

    /**
     * @brief The CollectionsRequest struct keeps request information, which is
     * used in XML conversion.
     *
     * Not all values are mandatory, the only mandatory one for collections
     * request is .with, which stores interlocutor's JID. You might also be
     * interested in setting .start and .end to specify timing. Nevertheless,
     * if .end attribute is skipped, server may return all the collections until
     * now, so, be careful.
     *
     * @example
     *      KDateTime start; start.fromString("1992-06-05T12:05:15Z");
     *      CollectionsRequest request("test@contact.org", start);
     *      archiveManager->requestCollections(request);
     *
     * This struct is used for both collections and chats requests.
     * WARNING: you MUST specify the exact time, received via collections
     * request, in chats request, or server won't find this collection.
     */
    struct CollectionsRequest {
        CollectionsRequest() {}
        CollectionsRequest(const QString &_with,
                           const KDateTime &_start = KDateTime(),
                           const KDateTime &_end = KDateTime(),
                           const QString &_after = QString())
            : with(_with),
              start(_start),
              end(_end),
              after(_after) {}

        QString with;
        KDateTime start, end;
        QString after;
    };

    /**
     * @brief The RSMInfo struct keeps Result Set Management (XEP-0059) information
     * received.
     *
     * When we request a collection (or collections), we specify maximum amount
     * of entries we're ready to receive. This maximum is built-in and programmer
     * should not take care of it. When server answers, it shows us number of
     * items sent and their boundaries in the whole answer. For example, we can
     * receive by 2 items, server has 4 items on it's side, then first answer
     * would contain an RSM entry like:
     *      RSMInfo { first => 0, last => 1, count => 4 }
     * and a second one would contain one like:
     *      RSMInfo { first => 2, last => 3, count => 4 }.
     *
     * Please, take note, first and last are the server-defined string literals and
     * should not always be numbers, so, any conversion on them is prohibited.
     */
    struct RSMInfo {
        RSMInfo() : count(0){}
        RSMInfo(const QString &_first, const QString &_last, uint _count)
            : first(_first),
              last(_last),
              count(_count) {}

        QString first, last;
        uint count;
    };

    /**
     * @brief The ChatInfo struct keeps collection data.
     *
     * When we want to get some stored messages, we must first request a collections
     * list. Collections are identified by both interlocutor's name and conversation
     * starting time.
     *
     * A list of those structs will be received after requestCollections() call.
     *
     * Server answers to our collections request by a list of collections, which are
     * converted into those structs by this class. Programmer should use those credentials
     * to request a collection choosen via requestCollection().
     */
    struct ChatInfo {
        ChatInfo() {}
        ChatInfo(const QString &_with, const KDateTime &_time)
            : with(_with),
              time(_time) {}
        QString with;
        KDateTime time;
    };

    /**
     * @brief The ChatItem struct keeps chat messages, one-by-one.
     *
     * When one had been received a list of collections, he would probably request
     * one particular collection via requestCollection() call. If so,
     * server would answer with a list of chat messages, which are converted
     * to those structs by this class.
     *
     * Every item contains message body, time and boolean value describing if
     * this message is received or sent by the current account.
     *
     * WARNING: Since XEP-0136 standard draft is not yet final, there might be
     * some troubles with .time field.
     */
    struct ChatItem {
        ChatItem() : isIncoming(true) {}
        ChatItem(KDateTime _offset, const QString &_body, bool _isIncoming)
            : time(_offset),
              body(_body),
              isIncoming(_isIncoming) {}
        QString body;
        KDateTime time;
        bool isIncoming;
    };

    /**
     * @param parent is a root Iris task.
     *
     * One creating this class should always use proper root task, since all the
     * facilities, such as stanzas sending and receiving, are related to this
     * root task's parametes.
     *
     * You should use Kopete's JabberAccount instance to get proper root task.
     * Something like account()->rootTask(), or client()->account()->rootTask(),
     * as done in protocols/jabber/ui/jabbereditaccountwidget.cpp.
     */
    JT_Archive(Task *const parent);

    /**
     * @brief requestPrefs sends an empty <pref/> tag to find out stored archiving
     * preferences.
     *
     * Results will be returned via signal-slot mechanism, take a look at signals
     * section of this file for further details.
     */
    virtual QString requestPrefs();

    /**
     * @brief requestCollections depending on request parameters set.
     * @return outgoing stanza's ID. Programmer may compare this to the one
     * received via signal-slot mechanism to determine whether info received
     * is one he had requested.
     *
     * You can find a bit more information in request/result structures, such as:
     * @see CollectionsRequest
     * @see ChatInfo
     * @see RSMInfo
     */
    virtual QString requestCollections(const CollectionsRequest&);

    /**
     * @brief requestCollection requests chats which are belong to the collection specified.
     *
     * To find more about collections specification, @see CollectionsRequest.
     *
     * @return outgoing stanza's ID. Programmer may compare this to the one
     * received via signal-slot mechanism to determine whether info received
     * is one he had requested.
     *
     * Also take a look at those resulting structures:
     * @see ChatItem
     * @see RSMInfo
     */
    virtual QString requestCollection(const CollectionsRequest&);

    /**
     * @brief take is an overloaded XMPP::Task's method.
     * @return true if stanza received is supposed to be handled by XEP-0136 task and
     * is well-formed, false otherwise.
     *
     * If it's not obvious if the stanza received is well-formed or not, it returns
     * false so any class further in an inheritance tree could check this out.
     *
     * This method checks if stanza received should be handled by this class. Look at
     * writePref() and handle*Tag() methods below for further details.
     * @see writePref
     * @see handleAutoTag
     */
    virtual bool take(const QDomElement &);

    virtual void updateDefault(const DefaultSave, const DefaultOtr, const uint expiration);
    virtual void updateAuto(bool isEnabled, const AutoScope = (AutoScope)-1);
    virtual void updateStorage(const MethodType, const MethodUse);

protected:

    /**
     * @brief writePrefs extracts every subtag from the QDomElement given and
     * calls writePref() with them.
     * @return true if every tag is recognized and parsed properly, false otherwise.
     */
    bool writePrefs(const QDomElement&, const QString &id);

    /**
     * @brief writePref parses tag name and calls proper handler from the handle{#name}Tag.
     * @return true if everything is parsed and recognized, false otherwise. If tag
     * name is correct, it returns handle*Tag function's result.
     */
    bool writePref(const QDomElement&, const QString &id);

    bool collectionsListReceived(const QDomElement&, const QString &id);
    bool chatReceived(const QDomElement &, const QString &id);

    /**
     * These functions are tags handlers. For every tag, writePref chooses proper
     * handler fron this list and calls it.
     *
     * Handlers check tag's integrity and standard compliance. If everything is right,
     * they emit an appropriate signal from the list below and return true. If optional
     * XML attribute is not set, they will set it to the default value. If optional
     * attribute is set, but invalid, signal will not be emitted and false will be
     * returned.
     */
    bool handleAutoTag(const QDomElement&, const QString &id);
    bool handleDefaultTag(const QDomElement&, const QString &id);
    bool handleItemTag(const QDomElement&, const QString &id);
    bool handleSessionTag(const QDomElement&, const QString &id);
    bool handleMethodTag(const QDomElement&, const QString &id);

    RSMInfo parseRSM(const QDomElement &);
    QList<ChatInfo> parseChatsInfo(const QDomElement &);

    /**
     * @brief uniformArchivingNS creates DOM element <tagName xmlns=NS></tagName>.
     * This function uses static NS element of the archiving class.
     * Result can be used to embed any XEP-0136 data.
     */
    QDomElement uniformArchivingNS(const QString &tagName);
    QDomElement uniformPrefsRequest();
    QDomElement uniformAutoTag(bool, AutoScope);
    QDomElement uniformDefaultTag(DefaultSave, DefaultOtr, uint expiration);
    QDomElement uniformMethodTag(MethodType, MethodUse);
    QDomElement uniformUpdate(const QDomElement&);
    QDomElement uniformCollectionsRequest(const CollectionsRequest &params);
    QDomElement uniformChatsRequest(const CollectionsRequest &params);
    QDomElement uniformRsmNS(const QString &tagName);
    QDomElement uniformRsmMax(uint max);
    QDomElement uniformSkeletonCollectionsRequest(const QString &with);
    QDomElement uniformSkeletonChatsRequest(const QString &with);
    QDomElement uniformRsm(const QString &after = QString());

signals:
    /**
     * <auto save='isEnabled' scope='scope'/>
     * Scope is an optional parameter, if not received, will be set to AutoScopeGlobal.
     */
    void automaticArchivingEnable(bool isEnabled, const QString &id, JT_Archive::AutoScope scope);

    /**
     * <default save='saveMode' otr='otr' expire='expire'/>
     * Expire is an optional parameter. If not received, will be set to infinity.
     */
    void defaultPreferenceChanged(JT_Archive::DefaultSave saveMode,JT_Archive::DefaultOtr otr, const QString &id,uint expire);

    /**
     * <method type='method' use='use'/>
     * This setting shows archiving priorities. Server usually sends three of <method/> tags,
     * so receiver should be ready to get three continuous signals.
     */
    void archivingMethodChanged(JT_Archive::MethodType method,JT_Archive::MethodUse use, const QString &id);

    void collectionsReceived(QList<JT_Archive::ChatInfo>, JT_Archive::RSMInfo, const QString &id);

    void chatReceived(QList<JT_Archive::ChatItem>, JT_Archive::RSMInfo, const QString &id);

protected:
    typedef bool (JT_Archive::*AnswerHandler)(const QDomElement&, const QDomElement&, const QString&);
    AnswerHandler chooseHandler(const QDomElement&);

private:
    bool handleSet(const QDomElement&, const QDomElement&, const QString&);
    bool handleGet(const QDomElement&, const QDomElement&, const QString&);
    bool handleResult(const QDomElement&, const QDomElement&, const QString&);
    bool handleError(const QDomElement&, const QDomElement&, const QString&);
    bool skip(const QDomElement&, const QDomElement&, const QString&) { return false; }
    bool acknowledge(const QDomElement&, const QDomElement&, const QString&) { return true; }
};

#endif
