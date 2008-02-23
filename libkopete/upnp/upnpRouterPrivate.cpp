/*
    upnpRouterPrivate.cpp -

    Copyright (c) 2007-2008 by Romain Castan      <romaincastan@gmail.com>
    Copyright (c) 2007-2008 by Bertrand Demay     <bertranddemay@gmail.com>

    Kopete    (c) 2002-2008 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include "upnpRouterPrivate.h"

UpnpRouterPrivate::UpnpRouterPrivate()
{
	router = new Router;
}

UpnpRouterPrivate::UpnpRouterPrivate(const QUrl &url)
{
	router = new Router(url);
}

UpnpRouterPrivate::UpnpRouterPrivate(const UpnpRouterPrivate &router)
{
	this->router = router.router;
}
		
UpnpRouterPrivate &UpnpRouterPrivate::operator=(const UpnpRouterPrivate &router)
{
	this->router = router.router;
	return *this;
}

QString UpnpRouterPrivate::routerDescription() const
{
	return router->routerDescription();
}

/**
* TODO finish to implementation of isEmpty
*/
bool UpnpRouterPrivate::isEmpty() const
{
	return this->router->isEmpty();
}

bool UpnpRouterPrivate::isValid() const
{
	return this->router->isValid();
}

bool UpnpRouterPrivate::openPort(quint16 port, const QString &typeProtocol, const QString &protocol)
{
	UPnp *d = UPnp::upnp();
	bool send = false;
	QList<QString> paramNameAction;
	QList<QString> paramValueAction;

	char c_port[50];

	paramNameAction.append(QString("NewRemoteHost"));
	paramNameAction.append(QString("NewExternalPort"));
	paramNameAction.append(QString("NewProtocol"));
	paramNameAction.append(QString("NewInternalPort"));
	paramNameAction.append(QString("NewInternalClient"));
	paramNameAction.append(QString("NewEnabled"));
	paramNameAction.append(QString("NewPortMappingDescription"));
	paramNameAction.append(QString("NewLeaseDuration"));
	
	sprintf(c_port,"%d",port);

	
	paramValueAction.append(QString(""));
	paramValueAction.append(c_port);
	paramValueAction.append(typeProtocol);
	paramValueAction.append(c_port);
	paramValueAction.append(d->hostAddress().toString());
	paramValueAction.append(QString("1"));
	paramValueAction.append(protocol);
	paramValueAction.append(QString("0"));
 
	if(d->sendAction(router->routerSettingUrl(),router->service().serviceType(), router->service().controlURL(),QString("AddPortMapping"),paramNameAction,paramValueAction))
	{
		send = true;
	}
	return send;
}

bool UpnpRouterPrivate::closePort(quint16 port, const QString &typeProtocol)
{
	UPnp *d = UPnp::upnp();
	bool send = false;
	QList<QString> paramNameAction;
	QList<QString> paramValueAction;

	char c_numPort[50];

	paramNameAction.append(QString("NewRemoteHost"));
	paramNameAction.append(QString("NewExternalPort"));
	paramNameAction.append(QString("NewProtocol"));

	sprintf(c_numPort,"%d",port);

	paramValueAction.append(QString(""));
	paramValueAction.append(c_numPort);
	paramValueAction.append(typeProtocol);

	if(d->sendAction(router->routerSettingUrl(),router->service().serviceType(), router->service().controlURL(),QString("DeletePortMapping"),paramNameAction,paramValueAction))
		send = true;
	return true;
}

