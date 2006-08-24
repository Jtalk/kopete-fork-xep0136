// -*- c++ -*-

/*
 *  Copyright (C) 2003, Richard J. Moore <rich@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 */

#ifndef KJSEMBED_BINDINGS_KOPETEMESSAGE_H
#define KJSEMBED_BINDINGS_KOPETEMESSAGE_H

#include <kjsembed/jsbindingplugin.h>
#include <kjsembed/jsproxy_imp.h>
#include <kopetemessage.h>

namespace KJSEmbed {
namespace Bindings {

class KopeteMessageLoader
	: public JSBindingPlugin
{
public:
	KopeteMessageLoader( QObject *parent, const char *name, const QStringList &args );
	KJS::Object createBinding(KJSEmbedPart *jspart, KJS::ExecState *exec, const KJS::List &args) const;
};

class JSKopeteMessageImp : public JSProxyImp
{
	/** Identifiers for the methods provided by this class. */
	enum MethodId
	{
		bgColor,
		setBgColor,
		fgColor,
		setFgColor,
		font,
		setFont,
		plainBody,
		setPlainBody,
		richBody,
		setRichBody,
		type,
		importance,
		setImportance,
		asXML
	};

public:
	JSKopeteMessageImp( KJS::ExecState *exec, int id );
	virtual ~JSKopeteMessageImp();

	static void addBindings( KJS::ExecState *exec, KJS::Object &object );

	/** Returns true iff this object implements the call function. */
	virtual bool implementsCall() const { return true; }

	KJS::Value call( KJS::ExecState *exec, KJS::Object &self, const KJS::List &args );

	static Kopete::Message *message( KJS::Object &object );

private:
	int extractInt( KJS::ExecState *exec, const KJS::List &args, int idx);
	QString extractString(KJS::ExecState *exec, const KJS::List &args, int idx);
	int mid;
};

} // Bindings namespace
} // KJSEmbed namespace

#endif

