/*
 * showtextdlg.cpp - dialog for displaying a text file
 * Copyright (C) 2003  Justin Karneges
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include"showtextdlg.h"

#include<qlayout.h>
#include<q3textedit.h>
#include<qpushbutton.h>
#include<qfile.h>
#include<qtextstream.h>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QHBoxLayout>


ShowTextDlg::ShowTextDlg(const QString &fname, bool rich, QWidget *parent, const char *name)
:QDialog(parent, name, FALSE, Qt::WDestructiveClose)
{
	QString text;

	QFile f(fname);
	if(f.open(QIODevice::ReadOnly)) {
		QTextStream t(&f);
		while(!t.eof())
			text += t.readLine() + '\n';
		f.close();
	}

	QVBoxLayout *vb1 = new QVBoxLayout(this);
	vb1->setSpacing(8);
	Q3TextEdit *te = new Q3TextEdit(this);
	te->setReadOnly(TRUE);
	te->setTextFormat(rich ? Q3TextEdit::RichText : Q3TextEdit::PlainText);
	te->setText(text);

	vb1->addWidget(te);

	QHBoxLayout *hb1 = new QHBoxLayout(vb1);
	hb1->addStretch(1);
	QPushButton *pb = new QPushButton(tr("&OK"), this);
	connect(pb, SIGNAL(clicked()), SLOT(accept()));
	hb1->addWidget(pb);
	hb1->addStretch(1);

	resize(560, 384);
}

#include "showtextdlg.moc"
