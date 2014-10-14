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

#include "line.h"

Line::Line(QDeclarativeItem *parent):
    QDeclarativeItem(parent)
    , m_x1(0)
    , m_y1(0)
    , m_x2(0)
    , m_y2(0)
    , m_color(Qt::black)
    , m_penWidth(1)
    , m_penDashed(false)
{
    // Important, otherwise the paint method is never called
    setFlag(QGraphicsItem::ItemHasNoContents, false);
}

void Line::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
        Q_UNUSED(option);
	Q_UNUSED(widget);
        QPen pen(m_color, m_penWidth);

        if (m_penDashed)
            pen.setStyle(Qt::DotLine);

        painter->setPen(pen);
 
        if(smooth() == true) {
            painter->setRenderHint(QPainter::Antialiasing, true);
        }
 
        int x = qMin(m_x1, m_x2) - m_penWidth/2;
        int y = qMin(m_y1, m_y2) - m_penWidth/2;
 
        painter->drawLine(m_x1 - x, m_y1 - y, m_x2 - x, m_y2 - y);
}

void Line::setX1(int x1)
{
    if(m_x1 == x1)
        return;
    m_x1 = x1;
    updateSize();
    emit x1Changed();
    update();
}
 
void Line::setY1(int y1) {
    if(m_y1 == y1)
        return;
    m_y1 = y1;
    updateSize();
    emit y1Changed();
    update();
}

void Line::setX2(int x2) {
    if(m_x2 == x2)
        return;
    m_x2 = x2;
    updateSize();
    emit x2Changed();
    update();
}
 
void Line::setY2(int y2) {
    if(m_y2 == y2)
        return;
    m_y2 = y2;
    updateSize();
    emit y2Changed();
    update();
}
 
void Line::setColor(const QColor &color) {
    if(m_color == color)
        return;
    m_color = color;
    emit colorChanged();
    update();
}

void Line::setPenWidth(qreal newWidth) {
    if(m_penWidth == newWidth)
        return;
    m_penWidth = newWidth;
    updateSize();
    emit penWidthChanged();
    update();
}

void Line::setPenDashed(bool dashed)
{
    m_penDashed = dashed;
    emit penDashedChanged();
    update();
}

void Line::updateSize()
{
    setX(qMin(m_x1, m_x2) - m_penWidth/2);
    setY(qMin(m_y1, m_y2) - m_penWidth/2);
    setWidth(qAbs(m_x2 - m_x1) + m_penWidth);
    setHeight(qAbs(m_y2 - m_y1) + m_penWidth);
}
