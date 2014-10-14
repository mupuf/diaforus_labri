/*
 *    Copyright 2012  Romain Perier <romain.perier@labri.fr>
 *
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License as
 *    published by the Free Software Foundation; either version 2 of 
 *    the License, or (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LINE_H
#define LINE_H
 
#include <QDeclarativeItem>
#include <QPainter>
 
class Line : public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(int x1 READ x1 WRITE setX1 NOTIFY x1Changed)
    Q_PROPERTY(int y1 READ y1 WRITE setY1 NOTIFY y1Changed)
    Q_PROPERTY(int x2 READ x2 WRITE setX2 NOTIFY x2Changed)
    Q_PROPERTY(int y2 READ y2 WRITE setY2 NOTIFY y2Changed)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qreal penWidth READ penWidth WRITE setPenWidth NOTIFY penWidthChanged)
    Q_PROPERTY(bool penDashed READ penDashed WRITE setPenDashed NOTIFY penDashedChanged)
 
public:
    Line(QDeclarativeItem *parent = 0);
 
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
 
    int x1() const { return m_x1; }
    int y1() const { return m_y1; }
    int x2() const { return m_x2; }
    int y2() const { return m_y2; }
    QColor color() const { return m_color; }
    qreal penWidth() const { return m_penWidth; }
    bool penDashed() const { return m_penDashed; }

    void setX1(int x1);
    void setY1(int y1);
    void setX2(int x2);
    void setY2(int y2);
    void setColor(const QColor &color);
    void setPenWidth(qreal newWidth);
    void setPenDashed(bool dashed);
 
Q_SIGNALS:
    void x1Changed();
    void y1Changed();
    void x2Changed();
    void y2Changed();
    void colorChanged();
    void penWidthChanged();
    void penDashedChanged();
 
protected:
    void updateSize();
 
protected:
    int m_x1;
    int m_y1;
    int m_x2;
    int m_y2;
    QColor m_color;
    qreal m_penWidth;
    bool m_penDashed;
};
 
QML_DECLARE_TYPE(Line)

#endif /* LINE_H */
