/*
    chatview.h - Chat View

    Copyright (c) 2002      by Olivier Goffart       <ogoffart@tiscalinet.be>

    Kopete    (c) 2002-2003 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef __CHATVIEW_H__
#define __CHATVIEW_H__

#include <qptrdict.h>
#include <qvaluelist.h>
#include <qpair.h>
#include <ktextedit.h>

#include <kopetecontact.h>
#include <kdockwidget.h>
#include <klistview.h>
#include <dom/html_element.h>

#include "kopeteview.h"

class QPixmap;
class KopeteTabWidget;
class QTimer;

class KHTMLPart;
class KHTMLView;
class KRootPixmap;

class KopeteChatWindow;
class KTabWidget;
class KopeteMessageManager;
class KCompletion;
class KURL;

using namespace DOM;

typedef QPtrList<KopeteContact> KopeteContactPtrList;

namespace KParts { struct URLArgs; class Part; }

class KopeteContactLVI : public QObject, public KListViewItem
{
	Q_OBJECT

	public:
		KopeteContactLVI( KopeteView *view, const KopeteContact *contact, KListView *parent );
		const KopeteContact *contact() const { return m_contact; }

	private:
		const KopeteContact *m_contact;
		KListView *m_parentView;
		KopeteView *m_view;

	private slots:
		void slotDisplayNameChanged(const QString &, const QString&);
		void slotStatusChanged();
		void slotExecute( QListViewItem* );
};

/**
 * @author Olivier Goffart
 */
class ChatView : public KDockMainWindow, public KopeteView
{
	Q_OBJECT
public:
	ChatView( KopeteMessageManager *manager, const char *name = 0 );
	~ChatView();

	/**
	 * Adds text into the edit area. Used when you select an emotiocon
	 * @param text The text to be inserted
	 */
	void addText( const QString &text );

	/**
	 * Saves window settings such as splitter positions
	 */
	void saveOptions();

	/**
	 * Sets the text to be displayed on the status label
	 * @param text The text to be displayed
	 */
	void setStatus( const QString &text );

	/**
	 * Tells this view if it is the active view
	 */
	void setActive( bool value );

	/**
	 * Clears the chat buffer
	 */
	virtual void clear();

	/**
	 * Sets the text to be displayed on tab label and window caption
	 */
	virtual void setCaption( const QString &text, bool modified );

	/**
	 * Changes the pointer to the chat widnow. Used to re-parent the view
	 * @param parent The new chat window
	 */
	void setMainWindow( KopeteChatWindow* parent );

	/**
	 * Sets the current message in the chat window
	 * @param parent The new chat window
	 */
	virtual void setCurrentMessage( const KopeteMessage &newMessage );

	void setTabBar( KTabWidget *tabBar );

	/**
	 * Sets the placement of the members contact list.
	 * DockLeft, DockRight, or DockNone.
	 * @param dp The dock position of the list
	 * @param width The % width the left hand widget should take up
	 */
	void placeMembersList( KDockWidget::DockPosition dp = KDockWidget::DockRight );

	/**
	 * Returns the message currently in the edit area
	 * @return The message
	 */
	virtual KopeteMessage currentMessage();

	/**
	 * Returns the chat window this view is in
	 * @return The chat window
	 */
	KopeteChatWindow *mainWindow() const { return m_mainWindow; }

	/**
	 * Returns the current position of the chat member slist
	 * @return The position of the chat members list
	 */
	const KDockWidget::DockPosition membersListPosition()  { return membersDockPosition; }

	/**
	 * Returns the HTML contents of the KHTML widget
	 * @return The contents of the view
	 */
	const QString viewsText();

	QString &status() { return m_status; }

	bool docked() { return ( m_tabBar != 0L ); }

	QString &caption() { return m_captionText; }

	bool sendInProgress() { return m_sendInProgress; }

	void historyUp();

	void historyDown();

	virtual void raise(bool activate=false);

	virtual void makeVisible();

	virtual bool isVisible();

	virtual QTextEdit *editWidget() { return static_cast<QTextEdit*>( m_edit ); }
	virtual QWidget *mainWidget() { return this; }

	/**
	 * Returns the HTML contents of the KHTML widget
	 * @return The contents of the view
	 */
	void nickComplete();

public slots:
	/**
	 * Initiates a cut action on the edit area of the chat view
	 */
	void cut();

	/**
	 * Initiates a copy action
	 * If there is text selected in the HTML view, that text is copied
	 * Otherwise, the entire edit area is copied.
	 */
	void copy();

	/**
	 * Initiates a paste action into the edit area of the chat view
	 */
	void paste();

	void print();

	void save();

	/**
	 * Selects all text in the chat view
	 */
	void selectAll();

	void pageUp();

	void pageDown();

	/**
	 * Sets the foreground color of the entry area, and outgoing messages
	 * @param newColor The new foreground color. If this is QColor(), then
	 * a color chooser dialog is opened
	 */
	void setFgColor( const QColor &newColor = QColor() );

	/**
	 * Sets the font of the edit area and outgoing messages to the specified value.
	 * @param newFont The new font to use.
	 */
	void setFont( const QFont &newFont );

	/*
	 * show a Font dialog and set the font selected by the user
	 */
	void setFont( );

	/**
	 * Sets the background color of the entry area, and outgoing messages
	 * @param newColor The new background color. If this is QColor(), then
	 * a color chooser dialog is opened
	 */
	void setBgColor( const QColor &newColor = QColor() );

	/**
	 * Sends the text currently entered into the edit area
	 */
	virtual void sendMessage();

	/**
	 * Appends a message to the chat view display area
	 * @param message The message to be appended
	 */
	void addChatMessage( KopeteMessage &message );

	/**
	 * Called when a message is recieved from someone
	 * @param message The message recieved
	 */
	virtual void appendMessage( KopeteMessage &message );

	/**
	 * Called when a typing event is recieved from a contact
	 * Updates the typing map and outputs the typing message into the status area
	 * @param contact The contact who is / isn't typing
	 * @param typing If the contact is typing now
	 */
	void remoteTyping( const KopeteContact *contact, bool typing );

	virtual void messageSentSuccessfully();

	virtual bool closeView( bool force = false );

	KParts::Part *part() const { return editpart; }

signals:
	/**
	 * Emits when a message is sent
	 * @param message The message sent
	 */
	void messageSent( KopeteMessage & );

	/**
	 * Emits every 4 seconds while the user is typing
	 * @param bool if the user is typing right now
	 */
	void typing( bool );

	void messageSuccess( ChatView* );

	/**
	 * Emits when the chat view is shown
	 */
	void shown();

	void closing( KopeteView* );

	void activated( KopeteView * );

	void captionChanged( bool active );

	void updateStatusIcon( const ChatView * );

private slots:
	void slotOpenURLRequest( const KURL &url, const KParts::URLArgs &args );
	void slotContactsContextMenu( KListView*, QListViewItem *item, const QPoint &point );
	void slotRepeatTimer();
	void slotRemoteTypingTimeout();
	void slotScrollView();
	void slotContactNameChanged( const QString &oldName, const QString &newName );

	/**
	 * Called when a contact is added to the KMM instance (A new person joins the chat).
	 * Adds this contact to the typingMap and the contact list view
	 * @param c The contact that joined the chat
	 * @param supress mean that no notifications are showed
	 */
	void slotContactAdded( const KopeteContact *c, bool surpress );

	/**
	 * Called when a contact is removed from the KMM instance (A person left the chat).
	 * Removes this contact from typingMap and the contact list view
	 * @param c The contact left the chat
	 * @param raison is the raison message
	 */
	void slotContactRemoved( const KopeteContact *c, const QString& raison );

	/**
	 * Called when a contact changes status, updates the contact list view and
	 * @param contact The contact who changed status
	 * @param status The new status of the contact
	 */
	void slotContactStatusChanged( KopeteContact *contact, const KopeteOnlineStatus &status, const KopeteOnlineStatus &oldstatus );

	/**
	 * Called when the chat's display name is changed
	 */
	void slotChatDisplayNameChanged();

	/**
	 * Called when text is changed in the edit area
	 */
	void slotTextChanged();

	void slotStopTimer();

	/**
	 * Called when KopetePrefs are saved
	 */
	void slotTransparancyChanged();
	void slotMarkMessageRead();

	/**
	 * Sets the background of the widget
	 * @param pm The new background image
	 */
	void slotUpdateBackground( const QPixmap &pm );

	void slotScrollingTo( int x, int y);

	void slotRefreshNodes();

	void slotRefreshView();

	void slotTransformComplete( const QVariant &result );

	void slotRightClick( const QString &, const QPoint &point );

	void slotCopyURL();

private:
	enum KopeteTabState { Normal , Highlighted , Changed , Typing , Message , Undefined };

	QPtrDict<QTimer> m_remoteTypingMap;
	KHTMLPart *chatView;
	KHTMLView *htmlWidget;
	bool isTyping;
	bool scrollPressed;
	QStringList historyList;
	int historyPos;
	bool bgChanged;
	QString unreadMessageFrom;
	QMap<const KopeteContact *, bool> typingMap;
	QMap<const KopeteContact *, KopeteContactLVI *> memberContactMap;
	KTextEdit* m_edit;
	QColor mBgColor;
	QColor mFgColor;
	QFont mFont;
	KListView *membersList;
	bool transparencyEnabled;
	bool bgOverride;
	bool isActive;
	bool m_sendInProgress;
	unsigned long messageId;
	QString m_lastMatch;
	QString backgroundFile;
	QString m_status;
	QPixmap m_icon;
	QPixmap m_iconLight;
	KCompletion *mComplete;
	HTMLElement activeElement;
	QValueList<KopeteMessage> messageList;

	KopeteTabState m_tabState;
	KRootPixmap *root;
	KDockWidget *viewDock;
	KDockWidget *membersDock;
	KDockWidget *editDock;
	KTabWidget *m_tabBar;
	KParts::Part *editpart;

	KAction *copyAction;
	KAction  *saveAction;
	KAction *printAction;
	KAction *closeAction;
	KAction *copyURLAction;

	KDockWidget::DockPosition membersDockPosition;

	KopeteChatWindow *m_mainWindow;

	QTimer *m_typingRepeatTimer;
	QTimer *m_typingStopTimer;

	/**
	 * Holds the string to be displayed on the tabs and window title
	 */
	QString m_captionText;

	void setTabState( KopeteTabState );

	/**
	 * Creates the members list widget
	 */
	void createMembersList();

	/**
	 * Read in saved options, such as splitter positions
	 */
	void readOptions();

	/**
	 * Refreshes the cuat view. Used to update the background.
	 */
	void refreshView();

	KopeteMessage messageFromNode( Node &n );
	void sendInternalMessage(const QString &msg);

	const QString styleHTML() const;

	QMap<unsigned long,KopeteMessage> messageMap;
};



#endif

// vim: set noet ts=4 sts=4 sw=4:

