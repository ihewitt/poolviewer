/*
 * This file is part of PoolViewer
 * Copyright (c) 2011 Ivor Hewitt
 *
 * PoolViewer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PoolViewer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PoolViewer.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QPainter>
#include <QTextCharFormat>

#include <stdio.h>

#include "calendar.h"

const char* eff[] =
{
    "Below Average",
    "Average",
    "Above Average",
    "Very Good",
    "Excellent"
};

QColor fgcolours[] =
    {
        QColor(0x00,0x00,0x00),
        QColor(0x00,0x00,0x00),
        QColor(0x00,0x00,0x00),
        QColor(0xff,0xff,0xff),
        QColor(0xff,0xff,0xff)
    };

QColor bgcolours[] =
    {
        QColor(0xff,0x33,0x00),
        QColor(0xff,0x99,0x00),
        QColor(0xff,0xff,0x33),
        QColor(0x00,0xff,0x33),
        QColor(0xff,0x33,0x99)
    };


int effToId(int effic)
{
    if (effic < 30) return 4;
    if (effic < 40) return 3;
    if (effic < 50) return 2;
    if (effic < 70) return 1;
    return 0;
}

CalendarWidget::CalendarWidget(QWidget *parent)
	: QCalendarWidget(parent)
{
//    setMouseTracking(true);
}

void CalendarWidget::paintCell ( QPainter * painter, const QRect & rect, const QDate & date ) const
{
    int h = rect.height();
    int cellh = h/2;
    int w = rect.width();
    int cellw = w/3;

    QCalendarWidget::paintCell(painter, rect, date);
    
    if (total.find(date) != total.end())
    {
        QFont cur = painter->font();

        // painter->setPen(Qt::black);
        // QString txt = QString::number(date.day());
        // painter->drawText(rect.left(),rect.top()+10, txt);

        //draw efficiency lights across the bottom
        const Totals &eff = total.find(date)->second;

        painter->setPen(Qt::black);
                
        painter->fillRect(rect.left(), rect.bottom()-cellh, cellw, cellh,
                          bgcolours[effToId(eff.max_eff)]);

        painter->drawText(QRect(rect.left(),rect.bottom()-cellh,cellw,cellh),
                          Qt::AlignCenter|Qt::AlignVCenter,
                          QString::number(eff.max_eff));

        painter->fillRect(rect.left()+cellw, rect.bottom()-cellh, cellw, cellh,
                          bgcolours[effToId(eff.avg_eff)]);

        painter->drawText(QRect(rect.left()+cellw,rect.bottom()-cellh,cellw,cellh),
                          Qt::AlignCenter|Qt::AlignVCenter,
                          QString::number(eff.avg_eff));

        painter->fillRect(rect.left()+cellw*2, rect.bottom()-cellh, cellw, cellh,
                          bgcolours[effToId(eff.min_eff)]);

        painter->drawText(QRect(rect.left()+cellw*2,rect.bottom()-cellh,cellw,cellh),
                          Qt::AlignCenter|Qt::AlignVCenter,
                          QString::number(eff.min_eff));

        //draw distance across the top
        painter->fillRect(rect.left()+cellw, rect.top(), cellw*2, cellh,
                          QColor(0,51,102));

        painter->setPen(Qt::cyan);
        painter->drawText(QRect(rect.left()+cellw, rect.top(), cellw*2, cellh),
                          Qt::AlignCenter|Qt::AlignVCenter,
                          QString::number( eff.dist ));

        QFont ft;
        ft.setBold(true);
        painter->setFont(ft);

        if (date.dayOfWeek()>5)
            painter->setPen(Qt::red);
        else
            painter->setPen(Qt::black);
        
        painter->drawText(QRect(rect.left(), rect.top(), cellw, cellh),
                          Qt::AlignCenter|Qt::AlignVCenter,
                          QString::number( date.day() ));
        
        painter->setFont(cur);
    }
}

void CalendarWidget::setData(const std::map<QDate, Totals> _lens)
{
    total = _lens;

    std::map<QDate,Totals>::iterator i;

    QTextCharFormat n;
    n.setVerticalAlignment(QTextCharFormat::AlignSuperScript);
    //    n.setHorizontalAlignment(QTextCharFormat::AlignLeft);
    setDateTextFormat(QDate(), n);

    QTextCharFormat c;
    c.setFontWeight(QFont::Bold);
    c.setUnderlineStyle(QTextCharFormat::SingleUnderline);
    c.setVerticalAlignment(QTextCharFormat::AlignBottom);
    //    c.setHorizontalAlignment(QTextCharFormat::AlignLeft);

    for (i=total.begin(); i!=total.end(); i++)
    {
        Totals &t = i->second;
        
        QString tip = 
            QString("<table><tr><td><b>Distance</b></td><td>%1m</td></tr>"
                    "<tr><td><b>Worst Efficiency</b></td><td>%2 (%3)</td></tr>"
                    "<tr><td><b>Average Efficiency</b></td><td>%4 (%5)</td></tr>"
                    "<tr><td><b>Best Efficiency</b></td><td>%6 (%7)</td></tr></table>")
            .arg(t.dist)
            .arg(t.max_eff)
            .arg(eff[effToId(t.max_eff)])
            .arg(t.avg_eff)
            .arg(eff[effToId(t.avg_eff)])
            .arg(t.min_eff)
            .arg(eff[effToId(t.min_eff)]);
        
        c.setToolTip( tip );
        setDateTextFormat(i->first,c);
    }
}


