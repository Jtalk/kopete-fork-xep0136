/*
    Tests for the emoticon engine

    Copyright (c) 2003      by Martijn Klingens       <klingens@kde.org>

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

#include <qtextstream.h>

#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kstandarddirs.h>

#include "kopeteemoticons.h"
#include "kopeteprefs.h"

static QTextStream _out( stdout, IO_WriteOnly );

/*
  There are three sets of tests, the Kopete 0.7 baseline with tests that were
  working properly in Kopete 0.7.x. When these fail it's a real regression.

  The second set are those known to work in the current codebase.

  The last set is the set with tests that are known to fail right now.

  Each entry in a set consists of two strings. The first string is the input,
  the second string is the EXPECTED output. If the two differ the test is
  considered failed and the actual result is output for comparison.

  A set should end with an entry containing at least one null pointer.
*/

typedef const char * TestSet[][ 2 ];


static TestSet kopete07Baseline =
{
	{ NULL, NULL }
};

static TestSet knownGood =
{
	{ ":):)", "<img align=\"center\" width=\"16\" height=\"16\" src=\"smile.png\" title=\":)\"/>"
			  "<img align=\"center\" width=\"16\" height=\"16\" src=\"smile.png\" title=\":)\"/>" },
	{ "http://", "http://" },
	{ "<img src=\"...\" title=\":-)\" />", "<img src=\"...\" title=\":-)\" />" },
	{ "End of sentence:p", "End of sentence<img align=\"center\" width=\"16\" height=\"16\" src=\"tongue.png\" title=\":p\"/>" },
	{ NULL, NULL }
};

static TestSet knownBroken =
{
	{ "&gt;:-)", "<img align=\"center\" width=\"16\" height=\"16\" src=\"devil.png\" title=\"&gt;:-)\"/>" },
	{ ":))", ":))" },
	{ "In a sentence:practical example", "In a sentence:practical example" },
	{ "Bla (&nbsp;)", "Bla (&nbsp;)" },
	{ NULL, NULL }
};

void runTests( QString description, TestSet tests )
{
	// Detect the image path by copying some code from kopeteemoticons.cpp
	QString path = KGlobal::dirs()->findResource( "data", "kopete/pics/emoticons/Default/smile.png" ).replace( "smile.png", QString::null );

	_out << endl;
	_out << "* Running test set '" << description << "'" << endl;

	uint i = 0;
	while ( tests[ i ][ 0 ] && tests[ i ][ 1 ] )
	{
		QString result = KopeteEmoticons::parseEmoticons( tests[ i ][ 0 ] ).replace( path, QString::null );

		if ( result == tests[ i ][ 1 ] )
		{
			_out << "  - Succeeded test for '" << tests[ i ][ 0 ] << "'" << endl;
		}
		else
		{
			_out << "  - FAILED test for '" << tests[ i ][ 0 ] << "'" << endl;
			_out << "    Expected output: '" << tests[ i ][ 1 ] << "'" << endl;
			_out << "    Real output:     '" << result << "'" << endl;
		}

		i++;
	}
}

int main( int argc, char *argv[] )
{
	KAboutData aboutData( "kopeteemoticontest", "kopeteemoticontest", "version" );
	KCmdLineArgs::init( argc, argv, &aboutData );
	KApplication app( "kopeteemoticontest" );

	// Set prefs (but don't save them :)
	KopetePrefs::prefs()->setUseEmoticons( true );
	KopetePrefs::prefs()->setIconTheme( "Default" );

	runTests( "Baseline of working emoticons in Kopete 0.7", kopete07Baseline );
	runTests( "Known working tests", knownGood );
	runTests( "Known broken tests", knownBroken );

	return 0;
}

// vim: set noet ts=4 sts=4 sw=4:

