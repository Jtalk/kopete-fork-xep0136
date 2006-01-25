/*
    kopetecontactproperty.h

    Kopete::Contact Property class

    Copyright (c) 2004    by Stefan Gehn <metz AT gehn.net>
    Copyright (c) 2006    by Michaël Larouche <michael.larouche@kdemail.net>

    Kopete    (c) 2004-2006    by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#ifndef _KOPETECONTACTPROPERTY_H_
#define _KOPETECONTACTPROPERTY_H_

#include <QVariant>
#include <QFlags>

#include "kopete_export.h"

namespace Kopete
{

/**
 * @author Stefan Gehn <metz AT gehn.net>
 * @author Michaël Larouche <michael.larouche@kdemail.net>
 *
 * The template class for registering properties in Kopete
 * You need to use this if you want to set properties for a
 * Kopete::Contact
 **/
class KOPETE_EXPORT ContactPropertyTmpl
{
	public:
		enum ContactPropertyOption 
		{ 
			NoProperty = 0x0,
			PersistentProperty = 0x1, 
			RichTextProperty = 0x2, 
			PrivateProperty = 0x4 
		};
		Q_DECLARE_FLAGS(ContactPropertyOptions, ContactPropertyOption)

		/**
		 * Constructor only used for empty ContactPropertyTmpl objects
		 *
		 * Note: Only useful for the null object
		 **/
		ContactPropertyTmpl();

		/**
		 * Constructor
		 * @param key internal unique key for this template
		 * @param label a label to show for properties based on this template
		 * @param icon name of the icon to show for properties based on this template
		 * @param options set the options for that property. See ContactPropertyOption enum.
		 **/
		ContactPropertyTmpl( const QString &key,
			const QString &label,
			const QString &icon = QString::null,
			ContactPropertyOptions options = NoProperty);

		/**
		 * Copy constructor
		 **/
		ContactPropertyTmpl(const ContactPropertyTmpl &other);

		/** Destructor */
		~ContactPropertyTmpl();

		ContactPropertyTmpl &operator=(const ContactPropertyTmpl &other);

		bool operator==(const ContactPropertyTmpl &other) const;
		bool operator!=(const ContactPropertyTmpl &other) const;

		/**
		 * Getter for the unique key. Properties based on this template will be
		 * stored with this key
		 **/
		const QString &key() const;

		/**
		 * Getter for i18ned label
		 **/
		const QString &label() const;

		/**
		 * Getter for icon to show aside or instead of @p label()
		 **/
		const QString &icon() const;

		/**
	 	 * Return the options for that property.
		 */
		ContactPropertyOptions options() const;

		/**
		 * Returns true if properties based on this template should
		 * be saved across Kopete sessions, false otherwise.
		 **/
		bool persistent() const;

		/**
		 * Returns true if properties based on this template are HTML formatted
		 **/
		bool isRichText() const;

		/**
		 * Returns true if properties based on this template are invisible to the user
		 **/
		bool isPrivate() const;

		/**
		 * An empty template, check for it using isNull()
		 */
		static ContactPropertyTmpl null;

		/**
		 * Returns true if this object is an empty template
		 **/
		bool isNull() const;

		/**
		 * A Map of QString and ContactPropertyTmpl objects, based on QMap
		 **/
		typedef QMap<QString, ContactPropertyTmpl>  Map;

	private:
		class Private;
		Private *d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ContactPropertyTmpl::ContactPropertyOptions)

/**
 * @author Stefan Gehn <metz AT gehn.net>
 *
 * A data container for whatever information Kopete or any of its
 * plugins want to store for a Kopete::Contact
 **/
class KOPETE_EXPORT ContactProperty
{
	public:
		/**
		 * Constructor only used for empty ContactProperty objects
		 *
		 * Note: you cannot set a label or value later on!
		 **/
		ContactProperty();

		/**
		 * @param tmpl The contact property template this property is based on
		 * @param value The value this Property holds
		 **/
		ContactProperty(const ContactPropertyTmpl &tmpl, const QVariant &value);

		/** Destructor **/
		~ContactProperty();

		/**
		 * Getter for this properties template
		 **/
		const ContactPropertyTmpl &tmpl() const;

		/**
		 * Getter for this properties value
		 **/
		const QVariant &value() const;

		/**
		 * The null, i.e. empty, ContactProperty
		 */
		static ContactProperty null;

		/**
		 * Returns true if this object is an empty Property (i.e. it holds no
		 * value), false otherwise.
		 **/
		bool isNull() const;

		/**
		 * Returns true if this property is HTML formatted
		 **/
		bool isRichText() const;

		/**
		 * A map of key,ContactProperty items
		 **/
		typedef QMap<QString, ContactProperty> Map;

	private:
		class Private;
		Private *d;
};

} // END namespace Kopete

#endif //_KOPETECONTACTPROPERTY_H_
