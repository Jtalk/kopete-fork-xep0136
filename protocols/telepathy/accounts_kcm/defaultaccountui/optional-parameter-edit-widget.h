/*
 * This file is part of telepathy-accounts-kcm
 *
 * Copyright (C) 2009 Collabora Ltd. <info@collabora.co.uk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef TELEPATHY_ACCOUNTS_KCM_OPTIONAL_PARAMETER_EDIT_WIDGET_H
#define TELEPATHY_ACCOUNTS_KCM_OPTIONAL_PARAMETER_EDIT_WIDGET_H

#include "parameter-edit-widget.h"

class OptionalParameterEditWidget : public ParameterEditWidget
{
    Q_OBJECT

public:
    explicit OptionalParameterEditWidget(Tp::ProtocolParameterList parameters,
                                         const QVariantMap &values = QVariantMap(),
                                         QWidget *parent = 0);
    ~OptionalParameterEditWidget();

    virtual bool validateParameterValues();

private:
    class Private;
    Private * const d;
};


#endif  // Header guard

