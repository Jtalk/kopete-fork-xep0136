//
//
// C++ Interface: h
//
// Description:
//
//
// Author: Will Stephenson (C) 2003
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef AIMCONTACT_H
#define AIMCONTACT_H

#include "oscarcontact.h"

class AIMAccount;
class AIMProtocol;
class KopeteMessageManager;
class AIMUserInfoDialog;

class AIMContact : public OscarContact
{
	Q_OBJECT

	public:
		AIMContact (const QString name, const QString displayName, AIMAccount *account, KopeteMetaContact *parent);
		virtual ~AIMContact();

		bool isReachable();
		KActionCollection *customContextMenuActions();

		/*
		 * Reimplemented because AIM handles start/stop of typing
		 */
		KopeteMessageManager* manager( bool canCreate = false );

		virtual void setStatus(const unsigned int newStatus);

		const QString &userProfile() { return mUserProfile; }
		const UserInfo &userInfo() { return mUserInfo; }
		const QString &awayMessage() { return mAwayMessage; }

		/*
		 * Only usable for the myself() contact
		 */
		void setOwnProfile(const QString &profile);

		virtual void gotIM(OscarSocket::OscarMessageType type, const QString &message);

	protected:
		/**
		* parses HTML AIM-Clients send to us and
		* strips off most of it
		*/
		KopeteMessage parseAIMHTML ( QString m );

		AIMProtocol* mProtocol;

		/**
		* The time of the last autoresponse,
		* used to determine when to send an
		* autoresponse again.
		*/
		long mLastAutoResponseTime;

	signals:
		void updatedProfile();

	private slots:
		/*
		 * Called when we get a minityping notification
		 */
		void slotGotMiniType(const QString &screenName, int type);
		void slotTyping(bool typing);
		/*
		 * Called when a buddy has changed status
		 */
		void slotContactChanged(const UserInfo &);

		/*
		 * Called when a buddy is going offline
		 */
		void slotOffgoingBuddy(QString sn);

		/*
		 * Called when we want to send a message
		 */
		void slotSendMsg(KopeteMessage&, KopeteMessageManager *);

		/*
		 * Called when the user requests a contact's user info
		 */
		void slotUserInfo();
		/*
		 * Warn the user
		 */
		void slotWarn();

		void slotGotProfile(const UserInfo &user, const QString &profile, const QString &away);

		void slotCloseUserInfoDialog();

	private:
		QString mUserProfile;
		UserInfo mUserInfo;
		QString mAwayMessage;
		AIMUserInfoDialog *infoDialog;
};
#endif
