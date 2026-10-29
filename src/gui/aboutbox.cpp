// SPDX-FileCopyrightText: 2006, 2008-2010, 2013-2019, 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later


#include "gui/aboutbox.h"

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QSysInfo>
#include <QTextEdit>

#define THANKS(name) #name"<br>"

AboutBox::AboutBox(QWidget* parent, Qt::WindowFlags f)
    : QDialog(parent, f)
{
    setObjectName("AboutBox");
    setWindowTitle(tr("About SpeedCrunch"));

    QString msg = "<center>";
    msg += "<img src=\":/speedcrunch.png\"><br>";
    msg += "<b>SpeedCrunch " SPEEDCRUNCH_VERSION;
#ifdef SPEEDCRUNCH_PORTABLE
    msg += " Portable";
#endif
    msg += "</b><br>";
    const QString osName = QSysInfo::prettyProductName()
        .remove(QLatin1Char('(')).remove(QLatin1Char(')'));
    msg += osName.toHtmlEscaped() + "<br>";
    msg += "Qt " + QLatin1String(QT_VERSION_STR) + "<br>";

    const QString authors = "<p><b>%1</b><br>%2";
    msg += authors.arg(tr("Main author | Logo | Maintainer"), "Helder Correia");

    msg += "<p>" + QString("<b>%1</b><br>").arg(tr("Core developers"));
    msg +=
        THANKS(Felix Krull)
        THANKS(Hadrien Theveneau)
        THANKS(Pol Welter)
        THANKS(Teyut)
        ;

    msg += authors.arg(tr("Original author"), "Ariya Hidayat");
    msg += authors.arg(tr("Math engine"), "Wolf Lammen");

    msg += "<p>" + QString("<b>%1</b><br>").arg(tr("Thanks"));
    msg +=
        THANKS(Alan Birtles)
        THANKS(Alan Davies)
        THANKS(Alejandro Villarreal)
        THANKS(Alessandro Portale)
        THANKS(Alexey Gorishny)
        THANKS(Alexey Kouznetsov)
        THANKS(Anders Lund)
        THANKS(Andreas Scherer)
        THANKS(Antonio Mannella)
        THANKS(Artem Golovinsky)
        THANKS(Aur&eacute;lien G&acirc;teau)
        THANKS(Bart Martens)
        THANKS(Benjamin Robin)
        THANKS(Bernhard Schiffner)
        THANKS(Brian Hale)
        THANKS(Christian Ehrlich)
        THANKS(Damir Perisa)
        THANKS(Daniel Sch&auml;ufele)
        THANKS(@ddidderr)
        THANKS(Derek O&#39;Connor)
        THANKS(D.J.L. Sales)
        THANKS(Enrico R&oacute;s)
        THANKS(Eugeniu Plamadeala)
        THANKS(F Chris Carrera)
        THANKS(Francesco di Cugno)
        THANKS(Francis Tseng)
        THANKS(@FrankMountainFace)
        THANKS(Gary Cramblitt)
        THANKS(Henrik Nordstrom)
        THANKS(Henrique Pinto)
        THANKS(James Yuzwalk)
        THANKS(Jean-Michel Frouin)
        THANKS(Jean-Remy Falleri)
        THANKS(@jetp250)
        THANKS(Johan Thelin)
        THANKS(Johannes Lange)
        THANKS(Jonathan Avraham)
        THANKS(Jonathan Riddell)
        THANKS(Lars Ivar Igesund)
        THANKS(l.inc)
        THANKS(Luigi Toscano)
        THANKS(Maciek Borowka)
        THANKS(Marco Wegner)
        THANKS(@Matth7878)
        THANKS(Matthew J Smith)
        THANKS(Matthias Prager)
        THANKS(Melchior Franz)
        THANKS(Michael Pyne)
        THANKS(Mikko Syrj&auml;)
        THANKS(Mohamed Eldesoky)
        THANKS(Nelson Almeida)
        THANKS(@nfshr)
        THANKS(Nikolay Zlatev)
        THANKS(Noah Davis)
        THANKS(Oliver Gubler)
        THANKS(@pandora1986)
        THANKS(Pedro Montes Alcalde)
        THANKS(Petri Damst&eacute;n)
        THANKS(Philippe Fremy)
        THANKS(Pieter Pareit)
        THANKS(@playerofgames)
        THANKS(@rafa4mat)
        THANKS(Rafael Fraga)
        THANKS(@RedNova84)
        THANKS(Roberto Alsina)
        THANKS(Rodrigo Pedra)
        THANKS(Roger Sachan)
        THANKS(Roland &#39;liquidat&#39; Wolters)
        THANKS(@Sacryn)
        THANKS(Sarah Hobbs)
        THANKS(S&eacute;bastien Szymanski)
        THANKS(Stephan Binner)
        THANKS(Steven Honeyman)
        THANKS(Theodore R. Smith)
        THANKS(Thomas Luebking)
        THANKS(Thomas Nagy)
        THANKS(@tosione)
        THANKS(Vibet Alexis)
        THANKS(Vitaly Zaitsev)
        THANKS(Vladimir Pouzanov)
        THANKS(Wictor Lund)
        THANKS(Witold Wysota)
        THANKS(1100101)
        ;

    msg += "</p><p>";

    msg += "<p><b>";
    msg += tr("Copyright (C) 2004-2026 The SpeedCrunch developers");
    msg += "</b></p>";

    msg += "<p>";
    msg += tr("This program is free software; you can redistribute it and/or "
              "modify it under the terms of the GNU General Public License "
              "as published by the Free Software Foundation; either version 2 "
              "of the License, or (at your option) any later version");
    msg += "</p>";
    msg += "<p>";
    msg += tr("This program is distributed in the hope that it will be useful, "
              "but WITHOUT ANY WARRANTY; without even the implied warranty of "
              "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
              "GNU General Public License for more details.");
    msg += "</p>";

    msg += "</center>";

    QTextEdit* textEdit = new QTextEdit(this);
    textEdit->setReadOnly(true);
    textEdit->setText(msg);

    QPushButton* closeButton = new QPushButton(this);
    closeButton->setText(tr("Close"));
    connect(closeButton, SIGNAL(clicked(bool)), this, SLOT(close()));

    QGridLayout* mainLayout = new QGridLayout(this);
    mainLayout->addWidget(textEdit, 0, 0, 1, 2);
    mainLayout->addWidget(closeButton, 1, 1, 1, 1);
    mainLayout->setColumnStretch(0, 1);

    setWindowTitle(tr("About SpeedCrunch"));
}
