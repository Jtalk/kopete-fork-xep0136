/*
    Kopete Yahoo Protocol
    Handles several lists such as buddylist, ignorelist and so on

    Copyright (c) 2005 André Duffeck <andre.duffeck@kdemail.net>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#ifndef STATUSNOTIFIERTASK_H
#define STATUSNOTIFIERTASK_H

#include "task.h"

class QString;

/**
@author André Duffeck
*/
class StatusNotifierTask : public Task
{
Q_OBJECT
public:
	StatusNotifierTask(Task *parent);
	~StatusNotifierTask();
	
	bool take(Transfer *transfer);

protected:
	bool forMe( Transfer *transfer ) const;
	void parseStatus( Transfer *transfer );
signals:
	void statusChanged( const QString&, int, const QString&, int );
	void error( const QString& );
};

#endif
