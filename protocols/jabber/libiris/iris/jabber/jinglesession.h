/*
 * This class defines a Jingle Session which contains all informations about the session.
 * This is here that the state machine is and where almost everything is done for a session.
 */
#ifndef JINGLE_SESSION
#define JINGLE_SESSION

#include <QObject>
#include <QString>
#include <QDomElement>

#include "im.h"
//#include "xmpp_client.h"
//#include "xmpp_jid.h"
#include "jingletasks.h"

namespace XMPP
{
	/*
	 * This class defines a jingle reason used when sending
	 * a session-terminate jingle action.
	 */
	class IRIS_EXPORT JingleReason
	{
	public:
		/*
		 * Default constructor : create a No Reason reason with no text.
		 */
		JingleReason();
		enum Type {
			Decline = 0,
			Busy,
			NoReason
		};
		/*
		 * Creates a reason with a type and a text reason.
		 */
		JingleReason(JingleReason::Type, const QString& text = QString());
		~JingleReason();
		
		//static Type stringToType(const QString&);

		void setType(Type);
		void setText(const QString&);
		Type type() const;
		QString text() const;
	private:
		class Private;
		Private *d;
	};

	class JingleContent;
	class JT_JingleSession;
	class JT_PushJingleSession;

	class IRIS_EXPORT JingleSession : public QObject
	{
		Q_OBJECT
	public:
		JingleSession();
		JingleSession(Task*, const Jid&);
		~JingleSession();

		/*
		 * Adds a content to the session.
		 * Currently, the content is just added in the contents list.
		 * TODO: addContent should add a content even when the session
		 * is in ACTIVE state so the session is modified with a content-add action.
		 */
		void addContent(JingleContent*);

		/*
		 * Same as above but the content is in a QDomElement form.
		 * For convenience.
		 */
		void addContent(const QDomElement&);

		/*
		 * Adds multiple contents to the session. It is advised
		 * to use this method instead of addContent() even for
		 * one content.
		 */
		void addContents(const QList<JingleContent*>&);

		/* 
		 * Adds session information to the session
		 * (used to inform the session that a "received"
		 * informational message has been received for eg.)
		 * Argument is a QDomElement containing the child(s -- TODO)
		 * of the jingle tag in a jingle stanza.
		 */
		void addSessionInfo(const QDomElement&);

		/*
		 * Adds transport info to the session.
		 * Mostly, it adds a candidate to the session
		 * and the session starts to try to connect to it.
		 * Argument is a QDomElement containing the child(s -- TODO)
		 * of the jingle tag in a jingle stanza.
		 */
		void addTransportInfo(const QDomElement&);

		/*
		 * Sends a content-accept jingle action.
		 * Not used yet, may be removed.
		 */
		void acceptContent();

		/*
		 * Sends a session-accept jingle action.
		 * Not used yet, may be removed.
		 */
		void acceptSession();

		/*
		 * Sends a remove-content jingle action with the content
		 * name given as an argument.
		 */
		void removeContent(const QString&);

		/*
		 * Sends a remove-content jingle action with the contents
		 * name given as an argument.
		 * Prefer this method instead of removeContent(const QString&);
		 */
		void removeContent(const QStringList&);

		/*
		 * Sends a session-terminate jingle action with the reason r.
		 * Once the responder sends the acknowledgement stanza, the
		 * signal terminated() is emitted.
		 */
		void terminate(const JingleReason& r = JingleReason());

		/*
		 * Sends a ringing informational message.
		 * FIXME:Would be better to use the sessionInfo() method.
		 */
		void ring();
		
		/*
		 * Returns the Jid of the other peer with whom the session is established.
		 */
		Jid to() const;

		/*
		 * Returns the contents of this session.
		 * In Pending state, it should return contents sent by the other peer.
		 * In Active state, it should return contents being used.
		 * This is right as we know which contents we do support.
		 */
		QList<JingleContent*> contents() const;

		/*
		 * Starts the session by sending a session-initiate jingle action.
		 * if a SID has been set, it will be overwritten by a new generated one.
		 */
		void start();
		
		/* This method sets the SID.
		 * For an incoming session, the sid must be set and not
		 * generated randomly.
		 * Calling the start method after this one will change the SID
		 */
		void setSid(const QString&);

		/*
		 * Sets peer's Jid.
		 */
		void setTo(const Jid&);

		/*
		 * Sets the initiator Jid.
		 * This can be already set if a session is redirected.
		 * Session redirection is NOT supported yet.
		 */
		void setInitiator(const QString&); //Or const Jid& ??

		/*
		 * Return initiator Jid.
		 */
		QString initiator() const;
		
		/*
		 * Start negotiation.
		 * This function is called after receiving a session initiate.
		 * This will start negotiating a connection depending on the transport.
		 */
		void startNegotiation();
		
		/*
		 * Returns a pointer to the first JingleContent with the name n.
		 * Each content must have a unique name so returning the first
		 * one returns the only one.
		 */
		JingleContent *contentWithName(const QString& n);
		
		/*
		 * Returns the sid of this session.
		 */
		QString sid() const;

		// Jingle actions
		enum JingleAction {
			SessionInitiate = 0,
			SessionTerminate,
			SessionAccept,
			SessionInfo,
			ContentAdd,
			ContentRemove,
			ContentModify,
			TransportReplace,
			TransportAccept,
			TransportInfo,
			NoAction
		};
		
		// Session states
		enum State {
			Pending = 0,
			Active,
			Ended
		};
		
		/*
		 * Returns the current state of the session.
		 */
		State state() const;

	signals:
		
		/*
		 * Emitted once a session-terminate has been acknowledged
		 */
		void terminated();
		
		/* 
		 * needData() is emitted once.
		 * Once it has been emitted, streaming must start on this socket until stopSending is emitted.
		 */
		void needData(XMPP::JingleContent*);
	public slots:
		
		/*
		 * This slot is called when a content-remove has been acked.
		 */
		void slotRemoveAcked();
		
		/*
		 * This slot is called when a session-terminate has been acked.
		 */
		void slotSessTerminated();

		/*
		 * This slot is called when data is received on the raw udp socket.
		 */
		void slotRawUdpDataReady();

	private:
		class Private;
		Private *d;
		
		/*
		 * Sends ice udp cadidates
		 */
		void sendIceUdpCandidates();
		
		/*
		 * Starts a raw udp connection for this JingleContent.
		 * (Create socket, ask to start sending data on it)
		 */
		void startRawUdpConnection(JingleContent*);
	};
}

#endif
