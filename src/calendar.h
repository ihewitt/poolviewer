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

#ifndef CALENDARWIDGET_H
#define CALENDARWIDGET_H

#include <map>

#include <QCalendarWidget>

class CalendarWidget : public QCalendarWidget
{
Q_OBJECT

public:
    struct Totals {
        int dist;
        int min_eff;
        int avg_eff;
        int max_eff;
    };

    CalendarWidget(QWidget *parent = 0);

    void setData(const std::map<QDate, Totals>);

protected:
    virtual void	paintCell ( QPainter * painter, const QRect & rect, const QDate & date ) const;

private:
    std::map<QDate, Totals> total;
};

#endif
