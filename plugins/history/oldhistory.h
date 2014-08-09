/*
	historyplugin.h

	Copyright (c) 2003-2005 by Olivier Goffart       <ogoffart at kde.org>
			  (c) 2003 by Stefan Gehn                 <metz AT gehn.net>
	Kopete    (c) 2003-2004 by the Kopete developers  <kopete-devel@kde.org>

	*************************************************************************
	*                                                                       *
	* This program is free software; you can redistribute it and/or modify  *
	* it under the terms of the GNU General Public License as published by  *
	* the Free Software Foundation; either version 2 of the License, or     *
	* (at your option) any later version.                                   *
	*                                                                       *
	*************************************************************************
*/

#ifndef OLDHISTORY_H
#define OLDHISTORY_H

class OldHistory
{
public:
	static void handle();
private:
	static void convert();
	static bool detect();
};

#endif // OLDHISTORY_H
