 /*
    kopetechatwindowstylemanager.cpp - Manager all chat window styles

    Copyright (c) 2005      by Michaël Larouche     <larouche@kde.org>

    Kopete    (c) 2002-2005 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "kopetechatwindowstylemanager.h"

// Qt includes
#include <QStack>

// KDE includes
#include <kstandarddirs.h>
#include <kdirlister.h>
#include <kdebug.h>
#include <kurl.h>
#include <kglobal.h>
#include <karchive.h>
#include <kzip.h>
#include <ktar.h>
#include <kmimetype.h>
#include <kio/netaccess.h>
#include <kstaticdeleter.h>
#include <ksharedconfig.h>
#include <kglobal.h>


#include "kopetechatwindowstyle.h"

class ChatWindowStyleManager::Private
{
public:
	Private()
	 : styleDirLister(0)
	{}

	~Private()
	{
		if(styleDirLister)
		{
			styleDirLister->deleteLater();
		}

		qDeleteAll(stylePool);
	}

	KDirLister *styleDirLister;
	QStringList availableStyles;

	// key = style name, value = ChatWindowStyle instance
	QHash<QString, ChatWindowStyle*> stylePool;

	QStack<KUrl> styleDirs;
};

static KStaticDeleter<ChatWindowStyleManager> ChatWindowStyleManagerstaticDeleter;

ChatWindowStyleManager *ChatWindowStyleManager::s_self = 0;

ChatWindowStyleManager *ChatWindowStyleManager::self()
{
	if( !s_self )
	{
		ChatWindowStyleManagerstaticDeleter.setObject( s_self, new ChatWindowStyleManager() );
	}
	
	return s_self;
}

ChatWindowStyleManager::ChatWindowStyleManager(QObject *parent)
	: QObject(parent), d(new Private())
{
	kDebug(14000) << k_funcinfo;
	loadStyles();
}

ChatWindowStyleManager::~ChatWindowStyleManager()
{	
	kDebug(14000) << k_funcinfo;
	delete d;
}

void ChatWindowStyleManager::loadStyles()
{
        QStringList chatStyles = KGlobal::dirs()->findDirs( "appdata", QLatin1String( "styles" ) );
        QString localStyleDir( KStandardDirs::locateLocal( "appdata", QLatin1String("styles/"),true) );
        if( !chatStyles.contains(localStyleDir))
                chatStyles<<localStyleDir;
	foreach(const QString &style, chatStyles)
	{
		kDebug(14000) << k_funcinfo << style;
		d->styleDirs.push( KUrl(style) );
	}
	
	d->styleDirLister = new KDirLister(this);
	d->styleDirLister->setDirOnlyMode(true);

	connect(d->styleDirLister, SIGNAL(newItems(const KFileItemList &)), this, SLOT(slotNewStyles(const KFileItemList &)));
	connect(d->styleDirLister, SIGNAL(completed()), this, SLOT(slotDirectoryFinished()));

	if( !d->styleDirs.isEmpty() )
		d->styleDirLister->openUrl(d->styleDirs.pop(), true);
}

QStringList ChatWindowStyleManager::getAvailableStyles() const
{
	return d->availableStyles;
}

int ChatWindowStyleManager::installStyle(const QString &styleBundlePath)
{
	QString localStyleDir( KStandardDirs::locateLocal( "appdata", QString::fromUtf8("styles/") ) );
	
	KArchiveEntry *currentEntry = 0L;
	KArchiveDirectory* currentDir = 0L;
	KArchive *archive = 0L;

	if( localStyleDir.isEmpty() )
	{
		return StyleNoDirectoryValid;
	}

	// Find mimetype for current bundle. ZIP and KTar need separate constructor
	QString currentBundleMimeType = KMimeType::findByPath(styleBundlePath, 0, false)->name();
	if(currentBundleMimeType == "application/zip")
	{
		archive = new KZip(styleBundlePath);
	}
	else if( currentBundleMimeType == "application/x-compressed-tar" || currentBundleMimeType == "application/x-bzip-compressed-tar" || currentBundleMimeType == "application/x-gzip" || currentBundleMimeType == "application/x-bzip" )
	{
		archive = new KTar(styleBundlePath);
	}
	else
	{
		return StyleCannotOpen;
	}

	if ( !archive->open(QIODevice::ReadOnly) )
	{
		delete archive;

		return StyleCannotOpen;
	}

	const KArchiveDirectory* rootDir = archive->directory();

	// Ok where we go to check if the archive is valid.
	// Each time we found a correspondance to a theme bundle, we add a point to validResult.
	// A valid style bundle must have:
	// -a Contents, Contents/Resources, Co/Res/Incoming, Co/Res/Outgoing dirs
	// main.css, Footer.html, Header.html, Status.html files in Contents/Ressources.
	// So for a style bundle to be valid, it must have a result greather than 8, because we test for 8 required entry.
	int validResult = 0;
	QStringList entries = rootDir->entries();
	// Will be reused later.
	QStringList::Iterator entriesIt, entriesItEnd = entries.end();
	for(entriesIt = entries.begin(); entriesIt != entries.end(); ++entriesIt)
	{
		currentEntry = const_cast<KArchiveEntry*>(rootDir->entry(*entriesIt));
// 		kDebug() << k_funcinfo << "Current entry name: " << currentEntry->name();
		if (currentEntry->isDirectory())
		{
			currentDir = dynamic_cast<KArchiveDirectory*>( currentEntry );
			if (currentDir)
			{
				if( currentDir->entry(QString::fromUtf8("Contents")) )
				{
// 					kDebug() << k_funcinfo << "Contents found";
					validResult += 1;
				}
				if( currentDir->entry(QString::fromUtf8("Contents/Resources")) )
				{
// 					kDebug() << k_funcinfo << "Contents/Resources found";
					validResult += 1;
				}
				if( currentDir->entry(QString::fromUtf8("Contents/Resources/Incoming")) )
				{
// 					kDebug() << k_funcinfo << "Contents/Resources/Incoming found";
					validResult += 1;
				}
				if( currentDir->entry(QString::fromUtf8("Contents/Resources/Outgoing")) )
				{
// 					kDebug() << k_funcinfo << "Contents/Resources/Outgoing found";
					validResult += 1;
				}
				if( currentDir->entry(QString::fromUtf8("Contents/Resources/main.css")) )
				{
// 					kDebug() << k_funcinfo << "Contents/Resources/main.css found";
					validResult += 1;
				}
				if( currentDir->entry(QString::fromUtf8("Contents/Resources/Footer.html")) )
				{
// 					kDebug() << k_funcinfo << "Contents/Resources/Footer.html found";
					validResult += 1;
				}
				if( currentDir->entry(QString::fromUtf8("Contents/Resources/Status.html")) )
				{
// 					kDebug() << k_funcinfo << "Contents/Resources/Status.html found";
					validResult += 1;
				}
				if( currentDir->entry(QString::fromUtf8("Contents/Resources/Header.html")) )
				{
// 					kDebug() << k_funcinfo << "Contents/Resources/Header.html found";
					validResult += 1;
				}
				if( currentDir->entry(QString::fromUtf8("Contents/Resources/Incoming/Content.html")) )
				{
// 					kDebug() << k_funcinfo << "Contents/Resources/Incoming/Content.html found";
					validResult += 1;
				}
				if( currentDir->entry(QString::fromUtf8("Contents/Resources/Outgoing/Content.html")) )
				{
// 					kDebug() << k_funcinfo << "Contents/Resources/Outgoing/Content.html found";
					validResult += 1;
				}
			}
		}
	}
// 	kDebug() << k_funcinfo << "Valid result: " << QString::number(validResult);
	// The archive is a valid style bundle.
	if(validResult >= 8)
	{
		bool installOk = false;
		for(entriesIt = entries.begin(); entriesIt != entries.end(); ++entriesIt)
		{
			currentEntry = const_cast<KArchiveEntry*>(rootDir->entry(*entriesIt));
			if(currentEntry && currentEntry->isDirectory())
			{
				// Ignore this MacOS X "garbage" directory in zip.
				if(currentEntry->name() == QString::fromUtf8("__MACOSX"))
				{
					continue;
				}
				else
				{
					currentDir = dynamic_cast<KArchiveDirectory*>(currentEntry);
					if(currentDir)
					{
						currentDir->copyTo(localStyleDir + currentDir->name());
						installOk = true;
					}
				}
			}
		}

		archive->close();
		delete archive;

		if(installOk)
			return StyleInstallOk;
		else
			return StyleUnknow;
	}
	else
	{
		archive->close();
		delete archive;

		return StyleNotValid;
	}

	if(archive)
	{
		archive->close();
		delete archive;
	}

	return StyleUnknow;
}

bool ChatWindowStyleManager::removeStyle(const QString &styleName)
{
	kDebug(14000) << k_funcinfo << styleName;
	// Find for the current style in avaiableStyles map.
	int foundStyleIdx = d->availableStyles.indexOf(styleName);

	if(foundStyleIdx != -1)
	{
		d->availableStyles.removeAt(foundStyleIdx);

		// Remove and delete style from pool if needed.
		if( d->stylePool.contains(styleName) )
		{
			ChatWindowStyle *deletedStyle = d->stylePool[styleName];
			d->stylePool.remove(styleName);
			delete deletedStyle;
		}
	
		QStringList styleDirs = KGlobal::dirs()->findDirs("appdata", QString("styles/%1").arg(styleName));
		if(styleDirs.isEmpty())
		{
			kDebug(14000) << k_funcinfo << "Failed to find style" << styleName;
			return false;
		}

		// attempt to delete all dirs with this style
		int numDeleted = 0;
		foreach( const QString& stylePath, styleDirs )
		{
		KUrl urlStyle(stylePath);
		// Do the actual deletion of the directory style.
		if(KIO::NetAccess::del( urlStyle, 0 ))
			numDeleted++;
		}
		return numDeleted == styleDirs.count();
	}
	else
	{
		return false;
	}
}

ChatWindowStyle *ChatWindowStyleManager::getStyleFromPool(const QString &styleName)
{
	if( d->stylePool.contains(styleName) )
	{
		kDebug(14000) << k_funcinfo << styleName << " was on the pool";

		// NOTE: This is a hidden config switch for style developers
		// Check in the config if the cache is disabled.
		// if the cache is disabled, reload the style every time it's getted.
		KConfigGroup config(KGlobal::config(), "KopeteStyleDebug");
		bool disableCache = config.readEntry("disableStyleCache", false);
		if(disableCache)
		{
			d->stylePool[styleName]->reload();
		}

		return d->stylePool[styleName];
	}
	else
	{
		// Build a chat window style and list its variants, then add it to the pool.
		ChatWindowStyle *style = new ChatWindowStyle(styleName, ChatWindowStyle::StyleBuildNormal);
		d->stylePool.insert(styleName, style);

		kDebug(14000) << k_funcinfo << styleName << " is just created";

		return style;
	}

	return 0;
}

void ChatWindowStyleManager::slotNewStyles(const KFileItemList &dirList)
{
	foreach(KFileItem *item, dirList)
	{
		// Ignore data dir(from deprecated XSLT themes)
		if( !item->url().fileName().contains(QString::fromUtf8("data")) )
		{
			kDebug(14000) << k_funcinfo << "Listing: " << item->url().fileName();
			// If the style path is already in the pool, that's mean the style was updated on disk
			// Reload the style
			QString styleName = item->url().fileName();
			if( d->stylePool.contains(styleName) )
			{
				kDebug(14000) << k_funcinfo << "Updating style: " << styleName;

				d->stylePool[styleName]->reload();

				// Add to avaialble if required.
				if( d->availableStyles.indexOf(styleName) == -1 )
					d->availableStyles.append(styleName);
			}
			else
			{
				// TODO: Use name from Info.plist
				d->availableStyles.append(styleName);
			}
		}
	}
}

void ChatWindowStyleManager::slotDirectoryFinished()
{
	// Start another scanning if the directories stack is not empty
	if( !d->styleDirs.isEmpty() )
	{
		kDebug(14000) << k_funcinfo << "Starting another directory.";
		d->styleDirLister->openUrl(d->styleDirs.pop(), true);
	}
	else
	{
		emit loadStylesFinished();
	}
}

#include "kopetechatwindowstylemanager.moc"
