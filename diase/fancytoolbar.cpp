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
#include "fancytoolbar.h"
#include "fancybutton.h"
#include <QDebug>

FancyToolBar::FancyToolBar(QWidget *parent) :
    QToolBar(parent)
{
    setStyleSheet("QToolBar {"
                  "background: qlineargradient("
                  "x1: 0, y1: 0, x2: 1, y2: 0,"
                  "stop: 0 #3f3f3f, stop: 0.25 #4e4e4e,"
                  "stop: 0.5 #5e5e5e, stop: 0.75 #707070,"
                  "stop: 1.0 #818181"
                  "); }");
}

void FancyToolBar::addFancyButton(FancyButton *button)
{
    m_buttons.append(button);
    connect(button, SIGNAL(clicked(bool)), SLOT(onFancyButtonClicked(bool)));

    QToolBar::addWidget(button);
}

void FancyToolBar::onFancyButtonClicked(bool checked)
{
    FancyButton *checkedButton;

    if (!checked)
        return;

    checkedButton = qobject_cast<FancyButton *>(sender());
    foreach (FancyButton *button, m_buttons) {
        if (button != checkedButton) {
            button->setChecked(false);
        }
    }
}
