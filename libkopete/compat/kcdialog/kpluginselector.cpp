/*  This file is part of the KDE project
    Copyright (C) 2002-2003 Matthias Kretz <kretz@kde.org>

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

#include "kpluginselector.h"
#include "kpluginselector_p.h"

#include <qvbox.h>
#include <qlabel.h>
#include <qstrlist.h>
#include <qfile.h>
#include <qstring.h>
#include <qlayout.h>
#include <qptrlist.h>
#include <qwidgetstack.h>
#include <qcursor.h>
#include <qapplication.h>
#include <qobjectlist.h>
#include <qcstring.h>

#include <kdeversion.h>
#include <kdebug.h>
#include <klocale.h>
#include <klistview.h>
#include <ksimpleconfig.h>
#include <kdialog.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <ktabctl.h>
#include <kcmoduleinfo.h>
#include <qvaluelist.h>
#include <kservice.h>
#include <ktrader.h>
#include "ktabwidget.h"
#include <kiconloader.h>
#include <kcmodule.h>
#include "kcmoduleinfo.h"
#include "kcmoduleloader.h"
#include <qsplitter.h>
#include <qframe.h>
#include "kplugininfo.h"
#include <kinstance.h>
#include <qptrdict.h>
#include <qstringlist.h>

struct KPluginSelectionWidget::KPluginSelectionWidgetPrivate
{
    KPluginSelectionWidgetPrivate( const QString & _instanceName,
            KPluginSelector * _kps, const QString & _cat,
            KConfigGroup * _config )
        : instanceName( _instanceName )
        , widgetstack( 0 )
        , kps( _kps )
        , config( _config )
        , catname( _cat )
        , currentplugininfo( 0 )
        , visible( true )
        , currentchecked( false )
        , changed( 0 )
    {
        moduleParentComponents.setAutoDelete( true );
    }

    ~KPluginSelectionWidgetPrivate()
    {
        delete config;
    }

    QMap<QCheckListItem*, KPluginInfo*> pluginInfoMap;

    QString instanceName; // isNull() for non-KParts plugins
    QWidgetStack * widgetstack;
    KPluginSelector * kps;
    KConfigGroup * config;

    QDict<KCModuleInfo> pluginconfigmodules;
    QMap<QString, int> widgetIDs;
    QMap<KPluginInfo*, bool> plugincheckedchanged;
    QString catname;
    QValueList<KCModule*> modulelist;
    QPtrDict<QStringList> moduleParentComponents;

    KPluginInfo * currentplugininfo;
    bool visible;
    bool currentchecked;
    int changed;
};

KPluginSelectionWidget::KPluginSelectionWidget( const QString & instanceName,
        KPluginSelector * kps, QWidget * parent, const QString & catname,
        const QString & category, KConfigGroup * config, const char * name )
    : QWidget( parent, name )
, d( new KPluginSelectionWidgetPrivate( instanceName, kps, catname, config ) )
{
    init( kpartsPluginInfos(), category );
}

KPluginSelectionWidget::KPluginSelectionWidget(
        const QValueList<KPluginInfo*> & plugininfos, KPluginSelector * kps,
        QWidget * parent, const QString & catname, const QString & category,
        KConfigGroup * config, const char * name )
    : QWidget( parent, name )
, d( new KPluginSelectionWidgetPrivate( 0, kps, catname, config ) )
{
    init( plugininfos, category );
}

inline QString KPluginSelectionWidget::catName() const
{
    return d->catname;
}

QValueList<KPluginInfo*> KPluginSelectionWidget::kpartsPluginInfos() const
{
    if( d->instanceName.isNull() )
    {
        QValueList<KPluginInfo*> list;
        return list; //nothing
    }

    QStringList desktopfilenames = KGlobal::dirs()->findAllResources( "data",
            d->instanceName + "/kpartplugins/*.desktop", true, false );
    return KPluginInfo::fromFiles( desktopfilenames );
}

void KPluginSelectionWidget::init( const QValueList<KPluginInfo*> & plugininfos,
        const QString & category )
{
    // setup Widgets
    ( new QVBoxLayout( this, 0, KDialog::spacingHint() ) )->setAutoAdd( true );
    KListView * listview = new KListView( this );
    connect( listview, SIGNAL( pressed( QListViewItem * ) ), this,
            SLOT( executed( QListViewItem * ) ) );
    connect( listview, SIGNAL( spacePressed( QListViewItem * ) ), this,
            SLOT( executed( QListViewItem * ) ) );
    connect( listview, SIGNAL( returnPressed( QListViewItem * ) ), this,
            SLOT( executed( QListViewItem * ) ) );
    connect( listview, SIGNAL( selectionChanged( QListViewItem * ) ), this,
            SLOT( executed( QListViewItem * ) ) );
    listview->setAcceptDrops( false );
    listview->setSelectionModeExt( KListView::Single );
    listview->setAllColumnsShowFocus( true );
    listview->addColumn( i18n( "Name" ) );
    listview->addColumn( i18n( "Description" ) );
    listview->addColumn( i18n( "Author" ) );
    listview->addColumn( i18n( "Version" ) );
    listview->addColumn( i18n( "License" ) );
    for( QValueList<KPluginInfo*>::ConstIterator it = plugininfos.begin();
            it != plugininfos.end(); ++it )
    {
        d->plugincheckedchanged[ *it ] = false;
        if( !( *it )->isHidden() &&
                ( category.isNull() || ( *it )->category() == category ) )
        {
            QCheckListItem * item = new QCheckListItem( listview,
                    ( *it )->name(), QCheckListItem::CheckBox );
            if( ! ( *it )->icon().isEmpty() )
                item->setPixmap( 0, SmallIcon( ( *it )->icon(), IconSize( KIcon::Small ) ) );
            item->setText( 1, ( *it )->comment() );
            item->setText( 2, ( *it )->author()  );
            item->setText( 3, ( *it )->version() );
            item->setText( 4, ( *it )->license() );
            item->setOn( ( *it )->isPluginEnabled() );
            d->pluginInfoMap.insert( item, *it );
        }
    }

    // widgetstack
    d->widgetstack = d->kps->widgetStack();
    load();
    // select and highlight the first item in the plugin list
    if( listview->firstChild() )
        listview->setSelected( listview->firstChild(), true );
}

KPluginSelectionWidget::~KPluginSelectionWidget()
{
    delete d;
}

bool KPluginSelectionWidget::pluginIsLoaded( const QString & pluginName ) const
{
    for( QMap<QCheckListItem*, KPluginInfo*>::ConstIterator it =
            d->pluginInfoMap.begin(); it != d->pluginInfoMap.end(); ++it )
        if( it.data()->pluginName() == pluginName )
            return it.data()->isPluginEnabled();
    return false;
}


QWidget * KPluginSelectionWidget::insertKCM( QWidget * parent,
        const KCModuleInfo & moduleinfo )
{
    KCModule * module = KCModuleLoader::loadModule( moduleinfo, false, parent );
    if( !module )
    {
        //FIXME: not very verbose
        QLabel * label = new QLabel( i18n( "Error" ), parent );
        label->setAlignment( Qt::AlignCenter );

        QApplication::restoreOverrideCursor();
        KCModuleLoader::showLastLoaderError( this );
        QApplication::setOverrideCursor( Qt::WaitCursor );

        return label;
    }
    // add the KCM to the list so that we can call load/save/defaults on it
    d->modulelist.append( module );
    QStringList * parentComponents = new QStringList(
            moduleinfo.service()->property(
                "X-KDE-ParentComponents" ).toStringList() );
    d->moduleParentComponents.insert( module, parentComponents );
    connect( module, SIGNAL( changed( bool ) ), SLOT( clientChanged( bool ) ) );
    return module;
}

void KPluginSelectionWidget::embeddPluginKCMs( KPluginInfo * plugininfo, bool checked )
{
    //if we have Services for the plugin we should be able to
    //create KCM(s)
    QApplication::setOverrideCursor( Qt::WaitCursor );
    if( plugininfo->kcmServices().size() > 1 )
    {
        // we need a tabwidget
        KTabWidget * tabwidget = new KTabWidget( d->widgetstack );
        tabwidget->setEnabled( checked );

        int id = d->widgetstack->addWidget( tabwidget );
        d->kps->configPage( id );
        d->widgetIDs[ plugininfo->pluginName() ] = id;

        for( QValueList<KService::Ptr>::ConstIterator it =
                plugininfo->kcmServices().begin();
                it != plugininfo->kcmServices().end(); ++it )
        {
            if( !( *it )->noDisplay() )
            {
                KCModuleInfo moduleinfo( *it );
                QWidget * module = insertKCM( tabwidget, moduleinfo );
                tabwidget->addTab( module, moduleinfo.moduleName() );
            }
        }
    }
    else
    {
        if( !plugininfo->kcmServices().front()->noDisplay() )
        {
            KCModuleInfo moduleinfo(
                    plugininfo->kcmServices().front() );
            QWidget * module = insertKCM( d->widgetstack, moduleinfo );
            module->setEnabled( checked );

            int id = d->widgetstack->addWidget( module );
            d->kps->configPage( id );
            d->widgetIDs[ plugininfo->pluginName() ] = id;
        }
    }
    QApplication::restoreOverrideCursor();
}

inline void KPluginSelectionWidget::updateConfigPage()
{
    updateConfigPage( d->currentplugininfo, d->currentchecked );
}

void KPluginSelectionWidget::updateConfigPage( KPluginInfo * plugininfo,
        bool checked )
{
    kdDebug( 702 ) << k_funcinfo << endl;
    d->currentplugininfo = plugininfo;
    d->currentchecked = checked;

    // if this widget is not currently visible (meaning that it's in a tabwidget
    // and another tab is currently opened) it's not allowed to change the
    // widgetstack
    if( ! d->visible )
        return;

    if( 0 == plugininfo )
    {
        d->kps->configPage( 1 );
        return;
    }

    if( plugininfo->kcmServices().empty() )
        d->kps->configPage( 1 );
    else
    {
        if( !d->widgetIDs.contains( plugininfo->pluginName() ) )
            // if no widget exists for the plugin create it
            embeddPluginKCMs( plugininfo, checked );
        else
        {
            // the page already exists
            int id = d->widgetIDs[ plugininfo->pluginName() ];
            d->kps->configPage( id );
            d->widgetstack->widget( id )->setEnabled( checked );
        }
    }
}

void KPluginSelectionWidget::clientChanged( bool didchange )
{
    kdDebug( 702 ) << k_funcinfo << endl;
    d->changed += didchange ? 1 : -1;
    if( d->changed == 1 )
        emit changed( true );
    else if( d->changed == 0 )
        emit changed( false );
    else if( d->changed < 0 )
        kdError( 702 ) << "negative changed value: " << d->changed << endl;
}

void KPluginSelectionWidget::tabWidgetChanged( QWidget * widget )
{
    if( widget == this )
    {
        d->visible = true;
        updateConfigPage();
    }
    else
        d->visible = false;
}

void KPluginSelectionWidget::executed( QListViewItem * item )
{
    kdDebug( 702 ) << k_funcinfo << endl;
    if( item == 0 )
        return;

    // Why not a dynamic_cast? - Martijn
    // because this is what the Qt API suggests; and since gcc 3.x I don't
    // trust dynamic_cast anymore - mkretz
    if( item->rtti() != 1 ) //check for a QCheckListItem
        return;

    QCheckListItem * citem = static_cast<QCheckListItem *>( item );
    bool checked = citem->isOn();
    kdDebug( 702 ) << "it's a " << ( checked ? "checked" : "unchecked" )
        << " QCheckListItem" << endl;

    KPluginInfo * info = d->pluginInfoMap[ citem ];
    if( info->isHidden() )
        kdFatal( 702 ) << "bummer" << endl;

    if ( info->isPluginEnabled() != checked )
    {
        kdDebug( 702 ) << "Item changed state, emitting changed()" << endl;

        if( ! d->plugincheckedchanged[ info ] )
        {
            ++d->changed;
            if ( d->changed == 1 )
                emit changed( true );
        }
        d->plugincheckedchanged[ info ] = true;

        checkDependencies( info );
    }
    else
    {
        if( d->plugincheckedchanged[ info ] )
        {
            --d->changed;
            if ( d->changed == 0 )
                emit changed( false );
        }
        d->plugincheckedchanged[ info ] = false;
        // FIXME: plugins that depend on this plugin need to be disabled, too
    }

    updateConfigPage( info, checked );
}

void KPluginSelectionWidget::load()
{
    kdDebug( 702 ) << k_funcinfo << endl;

    for( QMap<QCheckListItem*, KPluginInfo*>::Iterator it =
            d->pluginInfoMap.begin(); it != d->pluginInfoMap.end(); ++it )
    {
        KPluginInfo * info = it.data();
        info->load( d->config );
        it.key()->setOn( info->isPluginEnabled() );
        if( d->visible && info == d->currentplugininfo )
            d->currentchecked = info->isPluginEnabled();
    }

    for( QValueList<KCModule*>::Iterator it = d->modulelist.begin();
            it != d->modulelist.end(); ++it )
        if( ( *it )->changed() )
            ( *it )->load();

    updateConfigPage();
    // TODO: update changed state
}

void KPluginSelectionWidget::save()
{
    kdDebug( 702 ) << k_funcinfo << endl;

    for( QMap<QCheckListItem*, KPluginInfo*>::Iterator it =
            d->pluginInfoMap.begin(); it != d->pluginInfoMap.end(); ++it )
    {
        KPluginInfo * info = it.data();
        bool checked = it.key()->isOn();
        info->setPluginEnabled( checked );
        info->save( d->config );
    }
    QStringList updatedModules;
    for( QValueList<KCModule*>::Iterator it = d->modulelist.begin();
            it != d->modulelist.end(); ++it )
        if( ( *it )->changed() )
        {
            ( *it )->save();
            QStringList * names = d->moduleParentComponents[ *it ];
            for( QStringList::ConstIterator nameit = names->begin();
                    nameit != names->end(); ++nameit )
                if( updatedModules.find( *nameit ) == updatedModules.end() )
                    updatedModules.append( *nameit );
        }
    for( QStringList::ConstIterator it = updatedModules.begin(); it != updatedModules.end(); ++it )
        emit configCommitted( ( *it ).latin1() );

    updateConfigPage();
    kdDebug( 702 ) << "syncing config file" << endl;
    d->config->sync();
    d->changed = 0;
    emit changed( false );
}

void KPluginSelectionWidget::checkDependencies( const KPluginInfo * info )
{
    if( info->dependencies().isEmpty() )
        return;

    for( QStringList::ConstIterator it = info->dependencies().begin();
            it != info->dependencies().end(); ++it )
        for( QMap<QCheckListItem*,
                KPluginInfo*>::Iterator infoIt = d->pluginInfoMap.begin();
                infoIt != d->pluginInfoMap.end(); ++infoIt )
            if( infoIt.data()->pluginName() == *it )
            {
                if( !infoIt.key()->isOn() )
                {
                    infoIt.key()->setOn( true );
                    checkDependencies( infoIt.data() );
                }
                continue;
            }
}

class KPluginSelector::KPluginSelectorPrivate
{
    public:
        KPluginSelectorPrivate()
            : frame( 0 )
            , tabwidget( 0 )
            , widgetstack( 0 )
            , hideconfigpage( false )
            {
            }

        QFrame * frame;
        KTabWidget * tabwidget;
        QWidgetStack * widgetstack;
        QValueList<KPluginSelectionWidget *> pswidgets;
        bool hideconfigpage;
};

    KPluginSelector::KPluginSelector( QWidget * parent, const char * name )
    : QWidget( parent, name )
, d( new KPluginSelectorPrivate )
{
    QBoxLayout * vbox = new QVBoxLayout( this, 0, KDialog::spacingHint() );
    vbox->setAutoAdd( true );
    QSplitter * splitter = new QSplitter( Qt::Vertical, this, "PluginSelectionWidget splitter" );
    d->frame = new QFrame( splitter, "KPluginSelector top frame" );
    d->frame->setFrameStyle( QFrame::NoFrame );
    ( new QVBoxLayout( d->frame, 0, KDialog::spacingHint() ) )->setAutoAdd( true );

    // widgetstack
    d->widgetstack = new QWidgetStack( splitter,
            "KPluginSelector Config Pages" );
    d->widgetstack->setFrameStyle( QFrame::Panel | QFrame::Sunken );
    d->widgetstack->setMinimumSize( 200, 200 );

#if KDE_IS_VERSION( 3, 1, 92 )
    splitter->setOpaqueResize( KGlobalSettings::opaqueResize() );
#else
    splitter->setOpaqueResize( true );
#endif

    splitter->setResizeMode( d->widgetstack, QSplitter::FollowSizeHint );

    QLabel * label = new QLabel( i18n( "this plugin is not configurable" ),
            d->widgetstack );
    ( new QVBoxLayout( label, 0, KDialog::spacingHint() ) )->setAutoAdd( true );
    label->setAlignment( Qt::AlignCenter );
    label->setMinimumSize( 200, 200 );

    d->widgetstack->addWidget( label, 1 );

    configPage( 1 );
}

KPluginSelector::~KPluginSelector()
{
    delete d;
}

void KPluginSelector::checkNeedForTabWidget()
{
    kdDebug( 702 ) << k_funcinfo << endl;
    if( ! d->tabwidget && d->pswidgets.size() == 1 )
    {
        kdDebug( 702 ) << "no TabWidget and one KPluginSelectionWidget" << endl;
        // there's only one KPluginSelectionWidget yet, we need a TabWidget
        KPluginSelectionWidget * w = d->pswidgets.first();
        if( w )
        {
            kdDebug( 702 ) << "create TabWidget" << endl;
            d->tabwidget = new KTabWidget( d->frame,
                    "KPluginSelector TabWidget" );
            w->reparent( d->tabwidget, QPoint( 0, 0 ) );
            d->tabwidget->addTab( w, w->catName() );
            connect( d->tabwidget, SIGNAL( currentChanged( QWidget * ) ), w,
                    SLOT( tabWidgetChanged( QWidget * ) ) );
        }
    }
}

void KPluginSelector::addPlugins( const QString & instanceName,
        const QString & catname, const QString & category, KConfig * config )
{
    checkNeedForTabWidget();
    // FIXME: mem leak: the KSimpleConfig object needs to be deleted
    KConfigGroup * cfgGroup = new KConfigGroup( config ? config :
            new KSimpleConfig( instanceName ), "KParts Plugins" );
    kdDebug( 702 ) << k_funcinfo << "cfgGroup = " << cfgGroup << endl;
    KPluginSelectionWidget * w;
    if( d->tabwidget )
    {
        w = new KPluginSelectionWidget( instanceName, this,
                d->tabwidget, catname, category, cfgGroup );
        d->tabwidget->addTab( w, catname );
        connect( d->tabwidget, SIGNAL( currentChanged( QWidget * ) ), w,
                SLOT( tabWidgetChanged( QWidget * ) ) );
    }
    else
        w = new KPluginSelectionWidget( instanceName, this, d->frame,
                catname, category, cfgGroup );
    w->setMinimumSize( 200, 200 );
    connect( w, SIGNAL( changed( bool ) ), this, SIGNAL( changed( bool ) ) );
    connect( w, SIGNAL( configCommitted( const QCString & ) ), this,
            SIGNAL( configCommitted( const QCString & ) ) );
    d->pswidgets += w;
}

void KPluginSelector::addPlugins( const KInstance * instance, const QString &
        catname, const QString & category, KConfig * config )
{
    addPlugins( instance->instanceName(), catname, category, config );
}

void KPluginSelector::addPlugins( const QValueList<KPluginInfo*> & plugininfos,
        const QString & catname, const QString & category, KConfig * config )
{
    checkNeedForTabWidget();
    KConfigGroup * cfgGroup = new KConfigGroup( config ? config : KGlobal::config(), "Plugins" );
    kdDebug( 702 ) << k_funcinfo << "cfgGroup = " << cfgGroup << endl;
    KPluginSelectionWidget * w;
    if( d->tabwidget )
    {
        w = new KPluginSelectionWidget( plugininfos, this,
                d->tabwidget, catname, category, cfgGroup );
        d->tabwidget->addTab( w, catname );
        connect( d->tabwidget, SIGNAL( currentChanged( QWidget * ) ), w,
                SLOT( tabWidgetChanged( QWidget * ) ) );
    }
    else
        w = new KPluginSelectionWidget( plugininfos, this, d->frame,
                catname, category, cfgGroup );
    w->setMinimumSize( 200, 200 );
    connect( w, SIGNAL( changed( bool ) ), this, SIGNAL( changed( bool ) ) );
    connect( w, SIGNAL( configCommitted( const QCString & ) ), this,
            SIGNAL( configCommitted( const QCString & ) ) );
    d->pswidgets += w;
}

QWidgetStack * KPluginSelector::widgetStack()
{
    return d->widgetstack;
}

inline void KPluginSelector::configPage( int id )
{
    if( id == 1 )
    {
        // no config page
        if( d->hideconfigpage )
            d->widgetstack->hide();
        //else
            //d->widgetstack->
    }
    else
        d->widgetstack->show();

    d->widgetstack->raiseWidget( id );
}

void KPluginSelector::setShowEmptyConfigPage( bool show )
{
    d->hideconfigpage = !show;
}

void KPluginSelector::load()
{
    for( QValueList<KPluginSelectionWidget *>::Iterator it =
            d->pswidgets.begin(); it != d->pswidgets.end(); ++it )
    {
        ( *it )->load();
    }
}

void KPluginSelector::save()
{
    for( QValueList<KPluginSelectionWidget *>::Iterator it =
            d->pswidgets.begin(); it != d->pswidgets.end(); ++it )
    {
        ( *it )->save();
    }
}

void KPluginSelector::defaults()
{
    kdDebug( 702 ) << k_funcinfo << endl;

    // what should defaults do? here's what I think:
    // Pressing a button in the dialog should not change any widgets that are
    // not visible for the user. Therefor we may only change the currently
    // visible plugin's KCM. Restoring the default plugin selections is therefor
    // not possible. (if the plugin has multiple KCMs they will be shown in a
    // tabwidget - defaults() will be called for all of them)

    QWidget * pluginconfig = d->widgetstack->visibleWidget();
    KCModule * kcm = ( KCModule* )pluginconfig->qt_cast( "KCModule" );
    if( kcm )
    {
        kdDebug( 702 ) << "call KCModule::defaults() for the plugins KCM"
            << endl;
        kcm->defaults();
        return;
    }

    // doesn't work for plugins with more than one KCM
    QObjectList * kcms = pluginconfig->queryList( "KCModule", 0, false, false );
    QObjectListIt it( *kcms );
    QObject * obj;
    while( ( obj = it.current() ) != 0 )
    {
        ++it;
        ( ( KCModule* )obj )->defaults();
    }
    delete kcms;
    // FIXME: update changed state
}

// vim: sw=4 sts=4 et

#include "kpluginselector.moc"
#include "kpluginselector_p.moc"
