/*
  Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
  Copyright (c) 2000 Matthias Elter <elter@kde.org>
  Copyright (c) 2003 Daniel Molkentin <molkentin@kde.org>
  Copyright (c) 2003 Matthias Kretz <kretz@kde.org>

  This file is part of the KDE project
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License version 2, as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
  Boston, MA 02111-1307, USA.
*/


#include "kcmoduleinfo.h"

#include <kdesktopfile.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kstandarddirs.h>

#if 0 // I don't think Kopete needs this - Martijn
KCModuleInfo::KCModuleInfo(const QString& desktopFile)
  : _fileName(desktopFile), d(0L)
{
  _allLoaded = false;

  //kdDebug(1208) << "desktopFile = " << desktopFile << endl;
  init( KService::serviceByStorageId(desktopFile) );
}
#endif

KCModuleInfo::KCModuleInfo( KService::Ptr moduleInfo )
  : _fileName( moduleInfo->desktopEntryPath() )
{
  kdDebug() << k_funcinfo << _fileName << endl;
  _allLoaded = false;

  init(moduleInfo);
}

KCModuleInfo::KCModuleInfo( const KCModuleInfo &rhs )
    : d( 0 )
{
    ( *this ) = rhs;
}

// this re-implementation exists to ensure that other code always calls
// our re-implementation, so in case we add data to the d pointer in the future 
// we can be sure that we get called when we are copied.
KCModuleInfo &KCModuleInfo::operator=( const KCModuleInfo &rhs )
{
    _keywords = rhs._keywords;
    _name = rhs._name;
    _icon = rhs._icon;
    _lib = rhs._lib;
    _handle = rhs._handle;
    _fileName = rhs._fileName;
    _doc = rhs._doc;
    _comment = rhs._comment;
    _needsRootPrivileges = rhs._needsRootPrivileges;
    _isHiddenByDefault = rhs._isHiddenByDefault;
    _allLoaded = rhs._allLoaded;
    _service = rhs._service;

    // d pointer ... once used.

    return *this;
}

bool KCModuleInfo::operator==( const KCModuleInfo & rhs ) const
{
  return ( ( _name == rhs._name ) && ( _lib == rhs._lib ) && ( _fileName == rhs._fileName ) );
}

bool KCModuleInfo::operator!=( const KCModuleInfo & rhs ) const
{
  return ! operator==( rhs );
}

KCModuleInfo::~KCModuleInfo() { }

void KCModuleInfo::init(KService::Ptr s)
{
  _service = s;
  // set the modules simple attributes
  setName(_service->name());
  setComment(_service->comment());
  setIcon(_service->icon());

  // library and factory
  setLibrary(_service->library());

  // get the keyword list
  setKeywords(_service->keywords());
}

void
KCModuleInfo::loadAll() 
{
  _allLoaded = true;

  // library and factory
  setHandle(_service->property("X-KDE-FactoryName").toString());

  QVariant tmp;

  // read weight
  tmp = _service->property( "X-KDE-Weight" );
  setWeight( tmp.isValid() ? tmp.toInt() : 100 );

  // does the module need super user privileges?
  tmp = _service->property( "X-KDE-RootOnly" );
  setNeedsRootPrivileges( tmp.isValid() ? tmp.toBool() : false );

  // does the module need to be shown to root only?
  // Deprecated !
  tmp = _service->property( "X-KDE-IsHiddenByDefault");
  setIsHiddenByDefault( tmp.isValid() ? tmp.toBool() : false );

  // get the documentation path
  setDocPath( _service->property( "DocPath" ).toString() );
}

QString
KCModuleInfo::docPath() const
{
  if (!_allLoaded) 
    const_cast<KCModuleInfo*>(this)->loadAll();

  return _doc;
}

QString
KCModuleInfo::handle() const
{
  if (!_allLoaded) 
    const_cast<KCModuleInfo*>(this)->loadAll();

  if (_handle.isEmpty())
     return _lib;

  return _handle;
}

int
KCModuleInfo::weight() const
{
  if (!_allLoaded) 
    const_cast<KCModuleInfo*>(this)->loadAll();

  return _weight;
}

bool
KCModuleInfo::needsRootPrivileges() const
{
  if (!_allLoaded) 
    const_cast<KCModuleInfo*>(this)->loadAll();

  return _needsRootPrivileges;
}

bool
KCModuleInfo::isHiddenByDefault() const
{
  if (!_allLoaded)
    const_cast<KCModuleInfo*>(this)->loadAll();

  return _isHiddenByDefault;
}

// vim: ts=2 sw=2 et
