/*
    kopeteplugindataobject.h - Kopete Plugin Data Object

    Copyright (c) 2003      by Olivier Goffart       <ogoffart@tiscalinet.be>
    Copyright (c) 2003      by Martijn Klingens      <klingens@kde.org>
    Kopete    (c) 2002-2003 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#ifndef KOPETEPLUGINDATAOBJECT_H
#define KOPETEPLUGINDATAOBJECT_H

#include <qobject.h>
#include <qmap.h>
#include <qdom.h>

class KopetePlugin;
class QDomElement;

class KopetePluginDataObjectPrivate;

/**
 * @author Olivier Goffart  <ogoffart@tiscalinet.be>
 *
 * this class handles the saving of the plugin data to xml files.
 * KopeteMetaContact, KopeteGroup, and KopeteAccount inherits from it
 *
 * It also allow to store an icon for this element.
 * Note than the icon support is not availiable  for KopeteAccount
 */

class KopetePluginDataObject : public QObject
{
	Q_OBJECT

public:
	KopetePluginDataObject( QObject *parent = 0L, const char *name = 0L );

	~KopetePluginDataObject();

	/**
	 * Set the plugin-specific data.
	 * The data in the provided QMap is a set of key/value pairs.
	 * Note that protocol plugins usually shouldn't use this method, but
	 * reimplement @ref KopeteContact::serialize() instead. This method
	 * is called by @ref KopeteProtocol for those classes.
	 *
	 * WARNING: This erases all old data stored for this object!
	 *          You may want to consider the @ref setPluginData() overload
	 *          that takes a single field as parameter.
	 */
	void setPluginData( KopetePlugin *plugin, const QMap<QString, QString> &value );

	/**
	 * Convenience method to store or change only a single field of the
	 * plugin data. As with the other @ref setPluginData() method, protocols
	 * are advised not to use this method and reimplement
	 * @ref KopeteContact::serialize() instead.
	 *
	 * Note that you should save the file after adding data or it will get lost.
	 */
	void setPluginData( KopetePlugin *plugin, const QString &key, const QString &value );

	/**
	 * Get the settings as stored previously by calls to @ref setPluginData()
	 *
	 * Note that calling this method for protocol plugins that use the
	 * @ref KopeteContact::serialize() API may yield unexpected results.
	 */
	QMap<QString, QString> pluginData( KopetePlugin *plugin ) const;

	/**
	 * Convenience method to retrieve only a single field from the plugin
	 * data. See @ref setPluginData().
	 *
	 * Note that plugin data is accessible only after it has been loaded
	 * from the XML file. Don't call this method before then (e.g. in
	 * constructors).
	 */
	QString pluginData( KopetePlugin *plugin, const QString &key ) const;

	/**
	 * The various icon states. Some state are reserved for Groups,
	 * other for metacontact.
	 * 'None' is the default icon.
	 *
	 * FIXME: This icon stuff looks far too specialized to me for
	 *        KopetePluginDataObject, it should be made a lot more generic
	 *        remain here - Martijn
	 */
	enum IconState { None, Open, Closed, Online, Away, Offline, Unknown };

	/**
	 * return the icon for this object, in the given state.
	 * if there is no icon registered for this state, the None icon is used
	 * if available
	 */
	QString icon( IconState state = None ) const;

	/**
	 * Set the icon in the given state
	 * To clear an entry, set a QString::null
	 */
	void setIcon( const QString &icon, IconState = None );

	/**
	 * return if yes or no the user wants to display some custom icon.
	 * you can use @ref icon() to know the icons to uses
	 */
	bool useCustomIcon() const;

	/**
	 * set if the user want to show custom icon he set with @ref setIcon
	 * this does not clear icons string if you set false
	 */
	void setUseCustomIcon( bool useCustomIcon );

protected:
	/**
	 * Return a XML representation of plugin data
	 */
	const QValueList<QDomElement> toXML();

	/**
	 * Write the plugin data to KConfig
	 */
	void writeConfig( const QString &configGroup ) const;

	/**
	 * Load plugin data from one Dom Element:
	 * It should be a <plugin-data> element or a <custom-icons> element. if not, nothing will happen
	 * @return true if something has ben loaded. false if the element was not a fine
	 */
	bool fromXML( const QDomElement &element );

private:
	KopetePluginDataObjectPrivate *d;
};

#endif

// vim: set noet ts=4 sts=4 sw=4:

