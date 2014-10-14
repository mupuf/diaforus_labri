/*
 *   Copyright (C) 2012  Romain Perier <romain.perier@labri.fr>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "fancybutton.h"
#include <QDebug>
#include <QEvent>

FancyButton::FancyButton(const QString &text, QWidget *parent):
    QToolButton(parent)
{
    setText(text);
    setCheckable(true);
    applyDefaultStyleSheet();
    connect(this, SIGNAL(toggled(bool)), SLOT(onClicked(bool)));
}

void FancyButton::applyDefaultStyleSheet()
{
    setStyleSheet("QToolButton {"
                  "border: 1px;"
                  "height: 45px ;width: 70px; color: white; font: bold;"
                  "background:  qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #3f3f3f,"
                  "stop: 0.25 #4e4e4e, stop: 0.5 #5e5e5e, stop: 0.75 #707070 stop: 1.0 #818181)"
                  "};");
}

void FancyButton::onClicked(bool checked)
{
    if (checked) {
        setStyleSheet("QToolButton {"
                      "height: 45px ;width: 70px;border: 1px solid black; color: #535353; font: bold;"
                      "background:  qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #aaaaaa,"
                      "stop: 0.5 #cdcdcd, stop: 1.0 #e8e8e8)"
                       "};");
    } else {
        applyDefaultStyleSheet();
    }
}
