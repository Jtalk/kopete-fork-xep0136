/*  This file is part of the KDE project
    Copyright (C) 2003 Matthias Kretz <kretz@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.

*/

#include "ksettings/pluginpage.h"
#include "kpluginselector.h"
#include <qlayout.h>
#include <kdialog.h>
#include "ksettings/dispatcher.h"

namespace KSettings
{

class PluginPage::PluginPagePrivate
{
    public:
        PluginPagePrivate()
            : selwid( 0 )
        {
        }

        KPluginSelector * selwid;
};

    PluginPage::PluginPage( QWidget * parent, const char * name, const QStringList & args )
    : KCModule( parent, name, args )
    , d( new PluginPagePrivate )
{
    ( new QVBoxLayout( this, 0, KDialog::spacingHint() ) )->setAutoAdd( true );
    d->selwid = new KPluginSelector( this );
    connect( d->selwid, SIGNAL( changed( bool ) ), this, SLOT( setChanged( bool ) ) );
}

    PluginPage::PluginPage( KInstance * instance, QWidget * parent, const QStringList & args )
    : KCModule( instance, parent, args )
    , d( new PluginPagePrivate )
{
    ( new QVBoxLayout( this, 0, KDialog::spacingHint() ) )->setAutoAdd( true );
    d->selwid = new KPluginSelector( this );
    connect( d->selwid, SIGNAL( changed( bool ) ), this, SLOT( setChanged( bool ) ) );
    connect( d->selwid, SIGNAL( configCommitted( const QCString & ) ),
            Dispatcher::self(), SLOT( reparseConfiguration( const QCString & ) ) );
}

PluginPage::~PluginPage()
{
    delete d;
}

KPluginSelector * PluginPage::pluginSelector()
{
    return d->selwid;
}

void PluginPage::load()
{
    d->selwid->load();
}

void PluginPage::save()
{
    d->selwid->save();
}

void PluginPage::defaults()
{
    d->selwid->defaults();
}

} //namespace

#include "pluginpage.moc"
// vim: sw=4 sts=4 et
