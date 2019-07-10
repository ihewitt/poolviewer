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
#include <QPoint>
#include <QTime>

#include <stdio.h>

#include "graphwidget.h"

// Draw shaded bar
void drawBar(QPainter &painter,
             const QRect &rect,
             QColor col)
{
    QColor hi = col.lighter(160);
    QColor lo = col.darker(150);

    int w = rect.width()/4;

    painter.fillRect(rect, col);
    painter.fillRect(rect.left()+w, rect.top(), w, rect.height(), hi);
    painter.fillRect(rect.right(), rect.top(), -w*2, rect.height(), lo);
}

void GraphWidget::Series::getDouble(double &ret, int i, int j)
{
    double t = doubles[i];

    for (int c = i + 1; c < j; c++)
    {
        t += doubles[c];
    }
    if (j<0)
        ret = t;
    else
        ret = t/(j-i);
}

void GraphWidget::Series::getInt(int &ret, int i, int j)
{
    int t = integers[i];

    for (int c = i + 1; c < j; c++)
    {
        t += integers[c];
    }
    if (j<=0)
        ret = t;
    else
        ret = t/(j-i);
}

void GraphWidget::Series::getTime(int &ret, int i, int j)
{
    int t = seconds[i];

    for (int c = i + 1; c < j; c++)
    {
        t += seconds[c];
    }
    if (j>0)
        t = t/(j-i);

    ret = t;
}


GraphWidget::GraphWidget(QWidget *parent)
    : QWidget(parent)
{
}

void GraphWidget::setGraphs(bool beffic, bool bstroke, bool brate, bool bspeed)
{
    effic = beffic;
    stroke = bstroke;
    rate = brate;
    speed = bspeed;
}

// Draw bar graph
void GraphWidget::drawBars( QPainter &painter,
                            QColor pen,
                            int set)
{
    int w = width();
    int h = height();

    Series& graph = series[set];
    graph.maxGraph = -1;

    graph.calcScales();

    size_t i;
    size_t j=graph.size();

    if (!j)
        return;

    std::vector<QString> labels;
    std::vector<int> vals; // Y axis values to display

    QString label;

    //aggregate data to draw.
    for (i=0; i < j;)
    {
        int val=0;
        int num=0;
        label = xaxis[i];
        while ( i < j && label == xaxis[i])
        {
            val += graph.getY(i,h);
            num++;
            i++;
        }
        labels.push_back(label);
        vals.push_back(val);
    }

    //now rescale
    int max=0;
    for (i=0; i < vals.size(); i++)
    {
        if (vals[i]>max)
        {
            max = vals[i];
        }
    }

    graph.maxGraph = max;

    int thick = w/(vals.size()*3.25);
    if (thick<0) thick=2;

    for (i=0; i < vals.size(); i++)
    {
        int x = (set*thick) + w * i / (vals.size());

        int l = h * vals[i] / max;
        int y = h - l;

        drawBar(painter, QRect(x,y,thick-4,l), pen);
    }
}

//Draw stacked bar graph.
void GraphWidget::drawBars( QPainter &painter,
                            QColor pen1,
                            QColor pen2,
                            int set1, int set2)
{
    int w = width();
    int h = height();

    Series& graph1 = series[set1];
    Series& graph2 = series[set2];
    graph1.maxGraph = -1;
    graph2.maxGraph = -1;

    graph1.calcScales();
    graph2.calcScales();

    if (graph1.maxTime > graph2.maxTime)
        graph2.maxTime = graph1.maxTime;
    else
        graph1.maxTime = graph2.maxTime;

    int i;
    int j=graph1.size();
    if (!j)
        return;

    std::vector<QString> labels;
    std::vector<int> vals1, vals2;

    QString label;
    //aggregate data to draw.
    for (i=0; i < j;)
    {
        int val1=0,val2=0;
        int num=0;
        label = xaxis[i];
        while ( i < j && label == xaxis[i])
        {
            val1 += graph1.getY(i,h);
            val2 += graph2.getY(i,h);
            num++;
            i++;
        }
        labels.push_back(label);
        vals1.push_back(val1);
        vals2.push_back(val2);
    }

    //now rescale
    int max=0;
    for (i=0; i < (int)vals1.size(); i++)
    {
        if (vals1[i] + vals2[i] > max)
        {
            max = vals1[i] + vals2[i];
        }
    }

    graph1.maxGraph = max;
    graph2.maxGraph = max;

    int thick = w/(labels.size()*3.25);
    if (thick<0) thick=2;

    j=labels.size();
    for (i=0; i < j; i++)
    {
        int x = (set2 * thick) + w * i / j;

        int l1 = h * vals1[i] / max;
        int l2 = h * vals2[i] / max;

        int y = h - l1;

        //        painter.setPen(pen);
        drawBar(painter, QRect(x,y,thick-4,l1), pen1);
        drawBar(painter, QRect(x,y,thick-4,l2), pen2);
    }
}

// Draw series line graph
void GraphWidget::drawSeries( QPainter &painter,
                              QColor pen,
                              int set)
{
    int w = width();
    int h = height();

    Series& graph = series[set];
    graph.maxGraph = -1;

    int i;
    int j=graph.size();

    if (!j)
        return;

    std::vector<QString> labels;
    std::vector<QString> tags;
    std::vector<int> vals; // Y axis values to show

    QString label;

    //aggregate data by label
    for (i=0; i < j;)
    {
        int val=0;
        int num=0;
        label = xaxis[i];
        int s=i;
        while ( i < j && label == xaxis[i])
        {
            val += graph.getY(i,h);
            num++;
            i++;
        }
        labels.push_back(label);
        vals.push_back(val / num);

        QString text;
        //get label to tag
        if (graph.hasDoubles())
        {
            double dblVal;
            graph.getDouble( dblVal, s, s+num );
            text = QString::number(dblVal, 'g', 3);
        }
        else if (graph.hasInts())
        {
            int intVal;
            graph.getInt( intVal, s, s+num );
            text = QString::number(intVal);
        }
        else if (graph.hasTimes())
        {
            int timeVal;
            graph.getTime( timeVal, s, s+num );
            int s = timeVal % 60;
            timeVal = timeVal/60;
            int m = timeVal % 60;
            timeVal = timeVal/60;
            int h = timeVal;
            if (h>0)
            {
                text = QString("%1:%2:%3")
                        .arg(h,2,10,QChar('0'))
                        .arg(m,2,10,QChar('0'))
                        .arg(s,2,10,QChar('0'));
            }
            else
            {
                text = QString("%1:%2")
                        .arg(m,2,10,QChar('0'))
                        .arg(s,2,10,QChar('0'));
            }
        }
        tags.push_back(text);
    }

    //now rescale
    int max=0;
    for (i=0; i < (int)vals.size(); i++)
    {
        if (vals[i]>max)
        {
            max = vals[i];
        }
    }
    graph.maxGraph = max;

    j=labels.size();
    int space = w / j;

    int l=0;

    QPoint pt(0,0);
    for (i=0; i<j; i++)
    {
        l=vals[i];

        if (labels[i].isEmpty()) //Gap in series
        {
            pt = QPoint(0,0);
            continue;
        }

        int x =  (space/2)+i * space;
        int y = h - l;

        QPoint pt1(x,y);

        bool dotted=false;
        if (l == 0)
        {
            pt1.setY(pt.y());              // just continue line along
            dotted = true;
        }

        QPen dot(Qt::DotLine);

        if (!pt.isNull())
        {
            dot.setColor(Qt::darkGray);
            if (dotted)
                painter.setPen(dot);
            else
                painter.setPen(Qt::darkGray);

            painter.drawLine(pt.x()+1,pt.y()+1,pt1.x()+1,pt1.y()+1);

            if (dotted)
            {
                dot.setColor(pen);
                painter.setPen(dot);
            }
            else
                painter.setPen(pen);
            painter.drawLine(pt,pt1);
        }

        if (!dotted)
        {
            painter.drawEllipse(pt1.x()+1,pt1.y()+1, 3,3);
            painter.drawEllipse(pt1, 3,3);
        }
        painter.save();
        painter.translate(pt1);

        QString text = tags[i];

        if (!dotted)
        {
            painter.setPen(Qt::lightGray);
            painter.drawText(6,-9,text);

            painter.setPen(pen);
            painter.drawText(5,-10,text);
        }
        painter.restore();

        pt = pt1;
    }
}
//draw axis

void GraphWidget::drawXAxis(QPainter &painter,
                            QColor pen )
{
    int h = height();
    int w = width();

    painter.setPen(pen);
    painter.drawLine(0, h, w, h);

    //aggregate to unique labels
    std::vector<QString> labels;
    QString label;
    int i;
    int j = xaxis.size();
    for (i=0; i < j;)
    {
        label = xaxis[i];
        while ( i < j && label == xaxis[i])
        {
            i++;
        }
        labels.push_back(label);
    }

    j=labels.size();
    int space = w / j;

    if (!j)
        return;

    QPoint pt(0,0);
    for (i=0; i < j; i++)
    {
        int x =  (space/2)+space*i;

        painter.save();

        QString s=labels[i];

        painter.translate(x-5,10);
        // painter.rotate(45);
        painter.drawText(5,0,s);
        painter.restore();
    }

}

void GraphWidget::drawVAxis( QPainter &painter,
                             QColor pen,
                             int set )
{
    int h = height();
    int w = width();

    painter.setPen(pen);

    int pos;
    if (set == 0)
        pos = 30;
    if (set == 1)
        pos = 60;
    if (set == 2)
        pos = w-35;
    if (set == 3)
        pos = w-80;

    painter.drawLine(pos, 15, pos, h);

    bool right=false;
    if (pos > (w/2))
        right=true;

    if (right)
        painter.drawText(pos+3, 10, series[set].label);
    else
        painter.drawText(pos-28, 10, series[set].label);


    for (int tick=0; tick<10; tick++)
    {
        int y = (tick)*(h/10);

        QString s = series[set].getV( tick, 10 );

        if (right)
        {
            painter.drawLine(pos+5,h-y,pos,h-y);
            painter.drawText(pos+3,h-y-5,s);
        }
        else
        {
            painter.drawLine(pos-5,h-y,pos,h-y);
            painter.drawText(pos-25,h-y-5,s);
        }
    }

}

void GraphWidget::paintEvent(QPaintEvent *)
{
    if (xaxis.size()==0)
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // Add this to config dialog
    painter.setFont(QFont("FreeSans",9,-1,true));

    int w = width();
    int h = height();

    painter.save();
    painter.fillRect(0,0,w,h,Qt::white);

    painter.setViewport(QRect(60,0,w-140,h));

    drawXAxis(painter, Qt::black);

    // horizontal gridlines
    {
        painter.setPen(QPen(Qt::gray, 1, Qt::DashLine));
        for (int tick=0; tick<10; tick++)
        {
            int y = tick*(h/10);
            painter.drawLine(0,h-y,w,h-y);
        }
    }

    if (style == Line)
    {
        if (effic)
            drawSeries(painter, Qt::red,       0);
        if (speed)
            drawSeries(painter, Qt::darkGreen, 1);
        if (stroke)
            drawSeries(painter, Qt::blue,      2);
        if (rate)
            drawSeries(painter, Qt::magenta,   3);
    }
    else if (style == Bar)
    {
        drawBars(painter, Qt::red,       0);
        drawBars(painter, Qt::darkGreen, 1);
        //stack duration and rest
        drawBars(painter, Qt::blue,Qt::magenta,      3,2);
    }

    painter.restore();

    drawVAxis(painter, Qt::red, 0);
    drawVAxis(painter, Qt::darkGreen, 1);
    if (style == Line)
    {
        drawVAxis(painter, Qt::blue, 2);
    }
    drawVAxis(painter, Qt::magenta, 3);


}

void GraphWidget::clear()
{
    xaxis.clear();
    series.clear();
}
