#ifndef JABBER_JINGLE_CONTENT
#define JABBER_JINGLE_CONTENT

#include <QObject>
#include <QString>
#include <QDomElement>

namespace XMPP
{
	class JingleContent;
	class JingleSession;
}
class JabberJingleSession;
class JingleMediaManager;
class JingleMediaSession;
class JingleRtpSession;

class JabberJingleContent : public QObject
{
	Q_OBJECT
public:
	JabberJingleContent(JabberJingleSession* parent = 0, XMPP::JingleContent* c = 0);
	~JabberJingleContent();

	void setContent(XMPP::JingleContent*);
	void startWritingRtpData();
	QString contentName();
	QString elementToSdp(const QDomElement&);
	void prepareRtpInSession();
	void prepareRtpOutSession();

public slots:
	void slotSendRtpData();
	void slotIncomingData(const QByteArray&);
	void slotReadyRead(int);

private:
	XMPP::JingleContent *m_content;
	XMPP::JingleSession *m_jingleSession;
	JingleMediaManager *m_mediaManager;
	JingleMediaSession *m_mediaSession;
	JingleRtpSession *m_rtpInSession;
	JingleRtpSession *m_rtpOutSession;
	JabberJingleSession *m_jabberSession;
};

#endif
