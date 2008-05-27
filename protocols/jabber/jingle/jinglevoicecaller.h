#define PsiAccount JabberAccount

#ifndef JINGLEVOICECALLER_H
#define JINGLEVOICECALLER_H

#include <qmap.h>

#include "im.h"
#include "voicecaller.h"

using namespace XMPP;


class PsiAccount;

namespace cricket {
	class BasicPortAllocator;
	class SessionManager;
	class PhoneSessionClient;
	class Call;
}

namespace talk_base{
	class Thread;
	class SocketAddress;
	class NetworkManager;
	class SocketServer;
}

class JingleClientSlots;


class JingleVoiceCaller : public VoiceCaller
{
	Q_OBJECT

	friend class JingleClientSlots;

public:
	JingleVoiceCaller(PsiAccount* account);
	~JingleVoiceCaller();
	
	virtual bool calling(const Jid&);
	
	virtual void initialize();
	virtual void deinitialize();

	virtual void call(const Jid&);
	virtual void accept(const Jid&);
	virtual void reject(const Jid&);
	virtual void terminate(const Jid&);

protected:
	void sendStanza(const char*);
	void registerCall(const Jid&, cricket::Call*);
	void removeCall(const Jid&);

protected slots:
	void receiveStanza(const QString&);

private:
	bool initialized_;
	static talk_base::SocketServer *socket_server_;
	static talk_base::Thread *thread_;
	static talk_base::NetworkManager *network_manager_;
	static cricket::BasicPortAllocator *port_allocator_;
	static talk_base::SocketAddress *stun_addr_;
	cricket::SessionManager *session_manager_;
//	cricket::SessionManagerTask *session_manager_task_;
	cricket::PhoneSessionClient *phone_client_;
	JingleClientSlots *slots_;
	QMap<QString,cricket::Call*> calls_;
};

#endif
