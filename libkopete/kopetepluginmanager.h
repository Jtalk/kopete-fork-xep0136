/*
    kopetepluginmanager.h - Kopete Plugin Loader

    Copyright (c) 2002-2003 by Duncan Mac-Vicar Prett <duncan@kde.org>
    Copyright (c) 2002-2003 by Martijn Klingens       <klingens@kde.org>

    Kopete    (c) 2002-2003 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#ifndef KOPETEPLUGINMANAGER_H
#define KOPETEPLUGINMANAGER_H

#include <qmap.h>
#include <qobject.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qvaluelist.h>

class KPluginInfo;

class KopetePlugin;

/**
 * @author Duncan Mac-Vicar Prett <duncan@kde.org>
 * @author Martijn Klingens <klingens@kde.org>
 */
class KopetePluginManager : public QObject
{
	Q_OBJECT

public:
	/**
	 * Retrieve the plugin loader instance.
	 */
	static KopetePluginManager* self();

	~KopetePluginManager();

	/**
	 * Returns a list of all available plugins for the given category.
	 * Currently there are two categories, "Plugins" and "Protocols", but
	 * you can add your own categories if you want.
	 *
	 * If you pass an empty string you get the complete list of ALL plugins.
	 *
	 * You can query all information on the plugins through the @ref KPluginInfo
	 * interface.
	 */
	QValueList<KPluginInfo *> availablePlugins( const QString &category = QString::null ) const;

	/**
	 * Returns a list of all plugins that are actually loaded.
	 * If you omit the category you get all, otherwise it's a filtered list.
	 * See also @ref availablePlugins().
	 */
	QMap<KPluginInfo *, KopetePlugin *> loadedPlugins( const QString &category = QString::null ) const;

	/**
	 * @brief Search by plugin name. This is the key used as X-KDE-PluginInfo-Name in
	 * the .desktop file, e.g. "kopete_jabber"
	 *
	 * @return The @ref KopetePlugin object found by the search, or a null
	 * pointer if the plugin is not loaded.
	 *
	 * If you want to also load the plugin you can better use @ref loadPlugin, which returns
	 * the pointer to the plugin if it's already loaded.
	 */
	KopetePlugin *plugin( const QString &pluginName ) const;

	/**
	 * @brief Return the short user-visible name of the plugin.
	 *
	 * If you want to have the internal name, use @ref pluginId() instead.
	 *
	 * @return The name of the protocol, in the user's locale.
	 */
	QString pluginName( const KopetePlugin *plugin ) const;

	/**
	 * @brief Return the internal name of the plugin.
	 *
	 * You cannot display this name on the screen, it's used internally for
	 * passing around IDs. Use @ref pluginName() for a string ready for display.
	 *
	 * @return The name of the protocol, in the user's locale.
	 */
	QString pluginId( const KopetePlugin *plugin ) const;

	/**
	 * @brief Unload the plugin specified by @p pluginName
	 */
	bool unloadPlugin( const QString &pluginName );

	/**
	 * @brief Retrieve all registered address book fields for a given plugin.
	 *
	 * @return A QStringList object containing all the address book fields
	 * available to the plugin.
	 * @return An empty QStringList if the plugin is invalid.
	 */
	QStringList addressBookFields( KopetePlugin *p ) const;

	/**
	 * @brief Retrieve the name of the icon for a @ref KopetePlugin.
	 *
	 * @return An empty string if the given plugin is not loaded
	 * or the filename of the icon to use.
	 */
	QString pluginIcon( const KopetePlugin *plugin ) const;

	/**
	 * Shuts down the plugin manager on Kopete shutdown, but first
	 * unloads all plugins asynchronously.
	 *
	 * After 3 seconds all plugins should be removed; what's still left
	 * by then is unloaded through a hard delete instead.
	 *
	 * Note that this call also derefs the plugin manager from the event
	 * loop, so do NOT call this method when not terminating Kopete!
	 */
	void shutdown();

	/**
	 * Enable a plugin.
	 *
	 * This marks a plugin as enabled in the config file, so loadAll()
	 * can pick it up later.
	 *
	 * This method does not actually load a plugin, it only edits the
	 * config file.
	 *
	 * @param name is the name of the plugin as it is listed in the .desktop
	 * file in the X-KDE-Library field.
	 *
	 * Returns false when no appropriate plugin can be found.
	 */
	bool setPluginEnabled( const QString &name, bool enabled = true );

	/**
	 * Plugin loading mode. Used by @loadPlugin. Code that doesn't want to block
	 * the GUI and/or lot a lot of plugins at once should use Async loading.
	 * The default is sync loading.
	 */
	enum PluginLoadMode { LoadSync, LoadAsync };

public slots:
	/**
	 * @brief Load a single plugin by plugin name. Returns an existing plugin
	 * if one is already loaded in memory.
	 *
	 * If mode is set to Async, the plugin will be queued and loaded in
	 * the background. This method will return a null pointer. To get
	 * the loaded plugin you can track the @ref pluginLoaded() signal.
	 *
	 * See also @ref plugin().
	 */
	KopetePlugin * loadPlugin( const QString &pluginId, PluginLoadMode mode = LoadSync );

	/**
	 * @brief Loads all the enabled plugins. Also used to reread the
	 * config file when the configuration has changed.
	 */
	void loadAllPlugins();

signals:
	/**
	 * @brief Signals a new plugin has just been loaded.
	 */
	void pluginLoaded( KopetePlugin *plugin );

	/**
	 * @brief All plugins have been loaded by the plugin manager.
	 *
	 * This signal is emitted exactly ONCE, when the plugin manager has emptied
	 * its plugin queue for the first time. This means that if you call an async
	 * loadPlugin() before loadAllPlugins() this signal is probably emitted after
	 * the initial call completes, unless you are quick enough to fill the queue
	 * before it completes, which is a dangerous race you shouldn't count upon :)
	 *
	 * The signal is delayed one event loop iteration through a singleShot timer,
	 * but that is not guaranteed to be enough for account instantiation. You may
	 * need an additional timer for it in the code if you want to programmatically
	 * act on it.
	 *
	 * If you use the signal for enabling/disabling GUI objects there is little
	 * chance a user is able to activate them in the short while that's remaining,
	 * the slow part of the code is over now and the remaining processing time
	 * is neglectable for the user.
	 */
	void allPluginsLoaded();

private slots:
	/**
	 * @brief Cleans up some references if the plugin is destroyed
	 */
	void slotPluginDestroyed( QObject *plugin );

	/**
	 * shutdown() starts a timer, when it fires we force all plugins
	 * to be unloaded here by deref()-ing the event loop to trigger the plugin
	 * manager's destruction
	 */
	void slotShutdownTimeout();

	/**
	 * Common entry point to deref() the KApplication. Used both by the clean
	 * shutdown and the timeout condition of slotShutdownTimeout()
	 */
	void slotShutdownDone();

	/**
	 * Emitted by a KopetePlugin when it's ready for unload
	 */
	void slotPluginReadyForUnload();

	/**
	 * Load a plugin from our queue. Does nothing if the queue is empty.
	 * Schedules itself again if more plugins are pending.
	 */
	void slotLoadNextPlugin();

private:
	/**
	 * @internal
	 *
	 * The internal method for loading plugins.
	 * Called by @ref loadPlugin directly or through the queue for async plugin
	 * loading.
	 */
	KopetePlugin * loadPluginInternal( const QString &pluginId );

	/**
	 * @internal
	 *
	 * Find the KPluginInfo structure by key. Reduces some code duplication.
	 *
	 * Returns a null pointer when no plugin info is found.
	 */
	KPluginInfo * infoForPluginId( const QString &pluginId ) const;

	KopetePluginManager();

	static KopetePluginManager *s_self;

	class KopetePluginManagerPrivate;
	KopetePluginManagerPrivate *d;
};

#endif // KOPETEPLUGINMANAGER_H

// vim: set noet ts=4 sts=4 sw=4:

