/*
    Kopete , The KDE Instant Messenger
    Copyright (c) 2001-2002 by Duncan Mac-Vicar Prett <duncan@kde.org>

    Viva Chile Mierda!
    Started at Wed Dec 26 03:12:10 CLST 2001, Santiago de Chile

    Kopete    (c) 2002-2003 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include "kopete.h"

#include <dcopclient.h>
#include "kopeteiface.h"

#define KOPETE_VERSION "0.8.2"

static const char description[] =
	I18N_NOOP( "Kopete, the KDE Instant Messenger" );

static KCmdLineOptions options[] =
{
	{ "noplugins",              I18N_NOOP( "Do not load plugins. This option overrides all other options." ), 0 },
	{ "noconnect",              I18N_NOOP( "Disable auto-connection." ), 0 },
	{ "autoconnect <accounts>", I18N_NOOP( "Auto-connect the specified accounts. Use a comma-separated list\n"
		"to auto-connect multiple accounts." ), 0 },
	{ "disable <plugins>",      I18N_NOOP( "Do not load the specified plugin. Use a comma-separated list\n"
		"to disable multiple plugins." ), 0 },
	{ "load-plugins <plugins>", I18N_NOOP( "Load only the specified plugins. Use a comma-separated list\n"
		"to load multiple plugins. This option has no effect when\n"
		"--noplugins is set and overrides all other plugin related\n"
		"command line options." ), 0 },
	{ "!+[plugin]",            I18N_NOOP( "Load specified plugins" ), 0 },
	KCmdLineLastOption
};

int main( int argc, char *argv[] )
{
	KAboutData aboutData( "kopete", I18N_NOOP("Kopete"),
		KOPETE_VERSION, description, KAboutData::License_GPL,
		I18N_NOOP("(c) 2001-2003, Duncan Mac-Vicar Prett\n(c) 2002-2003, Kopete Development Team"), "kopete-devel@kde.org", "http://kopete.kde.org");

	aboutData.addAuthor ( "Duncan Mac-Vicar Prett", I18N_NOOP("Original author, Developer"), "duncan@kde.org", "http://www.mac-vicar.org/~duncan" );
	aboutData.addAuthor ( "Martijn Klingens", I18N_NOOP("Developer"), "klingens@kde.org" );
	aboutData.addAuthor ( "Till Gerken", I18N_NOOP("Developer, Jabber plugin maintainer"), "till@tantalo.net");
	aboutData.addAuthor ( "Olivier Goffart", I18N_NOOP("Developer, MSN plugin maintainer"), "ogoffart@tiscalinet.be");
	aboutData.addAuthor ( "Gav Wood", I18N_NOOP("WinPopup plugin maintainer"), "gjw102@york.ac.uk" );
	aboutData.addAuthor ( "Grzegorz Jaskiewicz", I18N_NOOP("Developer, Gadu plugin maintainer"), "gj@pointblue.com.pl" );
	aboutData.addAuthor ( "Zack Rusin", I18N_NOOP("Developer, original Gadu plugin author"), "zack@kde.org" );
	aboutData.addAuthor ( "Chris TenHarmsel", I18N_NOOP("Developer, Oscar"), "tenharmsel@users.sourceforge.net", "http://bemis.kicks-ass.net");
	aboutData.addAuthor ( "Jason Keirstead", I18N_NOOP("Developer, IRC maintainer"), "jason@keirstead.org", "http://www.keirstead.org");
	aboutData.addAuthor ( "Chris Howells", I18N_NOOP("Connection status plugin author"), "howells@kde.org", "http://chrishowells.co.uk");
	aboutData.addAuthor ( "Andy Goossens", I18N_NOOP("Developer"), "andygoossens@pandora.be" );
	aboutData.addAuthor ( "Will Stephenson", I18N_NOOP("Developer, icons, plugins"), "lists@stevello.free-online.co.uk" );
	aboutData.addAuthor ( "Matt Rogers", I18N_NOOP("Developer, Yahoo plugin maintainer"), "mattrogers@sbcglobal.net" );

	aboutData.addCredit ( "Luciash d' Being", I18N_NOOP("Kopete's icon author") );
	aboutData.addCredit ( "Vladimir Shutoff", I18N_NOOP("SIM icq library") );
	aboutData.addCredit ( "Tom Linsky", I18N_NOOP("OscarSocket author"), "twl6@po.cwru.edu" );
	aboutData.addCredit ( "Herwin Jan Steehouwer", I18N_NOOP("KxEngine icq code") );
	aboutData.addCredit ( "Olaf Lueg", I18N_NOOP("Kmerlin MSN code") );
	aboutData.addCredit ( "Justin Karneges", I18N_NOOP("Psi Jabber code") );
	aboutData.addCredit ( "Steve Cable", I18N_NOOP("Sounds") );
	aboutData.addCredit ( "Stefan Gehn", I18N_NOOP("Developer"), "metz@gehn.net", "http://metz.gehn.net" );

	aboutData.addCredit ( "Nick Betcher", I18N_NOOP("Former developer, project co-founder"), "nbetcher@kde.org");
	aboutData.addCredit ( "Daniel Stone", I18N_NOOP("Former developer, Jabber plugin author"), "daniel@fooishbar.org", "http://fooishbar.org");
	aboutData.addCredit ( "Ryan Cumming", I18N_NOOP("Former developer"), "ryan@kde.org" );
	aboutData.addCredit ( "Richard Stellingwerff", I18N_NOOP("Former developer"), "remenic@linuxfromscratch.org");
	aboutData.addCredit ( "Hendrik vom Lehn", I18N_NOOP("Former developer"), "hennevl@hennevl.de", "http://www.hennevl.de");
	aboutData.addCredit ( "Andres Krapf", I18N_NOOP("Former developer"), "dae@chez.com" );
	aboutData.addCredit ( "Carsten Pfeiffer", I18N_NOOP("Misc bugfixes and enhancements"), "pfeiffer@kde.org" );

	aboutData.setTranslator( I18N_NOOP("_: NAME OF TRANSLATORS\nYour names"),
		I18N_NOOP("_: EMAIL OF TRANSLATORS\nYour emails") );

	KCmdLineArgs::init( argc, argv, &aboutData );
	KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.
	KUniqueApplication::addCmdLineOptions();

	Kopete kopete;
	kapp->dcopClient()->setDefaultObject( (new KopeteIface())->objId() ); // Has to be called before exec

	kopete.exec();
}

// vim: set noet ts=4 sts=4 sw=4:

