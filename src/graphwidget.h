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
#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include <QWidget>
#include <QDateTime>

#include <stdio.h>

//store axis and data points.

class GraphWidget : public QWidget
{
Q_OBJECT

public:
    enum Style
    {
        Bar,
        Line
    };

    class Series
    {
    public:
        Series() { scaled=false;}

        bool hasDoubles() { return !doubles.empty();}
        bool hasInts() { return !integers.empty();}
        bool hasTimes() { return !seconds.empty();}

        int size() {
            if (hasDoubles()) return doubles.size();
            if (hasInts()) return integers.size();
            if (hasTimes()) return seconds.size();
            return 0;}

        void getDouble( double &ret, int i, int j=-1);
        void getInt( int &ret, int i, int j=-1);
        void getTime( int &ret, int i, int j=-1);
        
        // scale point at i between min and max
        int getY( int i, int h)
        {
            if (!scaled)
                calcScales();
            
            if (hasDoubles())
            {
                double data = doubles[i];
                double s=(maxDbl-minDbl);
                if (s==0) return 0;
                double result = ((data - minDbl) * h) / s;
                return abs(result);
            }
            else if (hasInts())
            {
                int data = integers[i];
                int s = maxInt-minInt;
                if (s==0) return 0;
                int result = ((data - minInt) * h) / s;
                return result;
            }
            else if (hasTimes())
            {
                int data = seconds[i];
                int s = maxTime-minTime;
                if (s==0) return 0;
                int result = ((data - minTime) *h) / s;
                return result;
            }
            return 0;           
        }

        QString getV(int y, int h)
        {
            if (!scaled)
                calcScales();
            
            if (hasDoubles())
            {
                double n = minDbl + y * (maxDbl-minDbl) / h;
                return QString::number(n,'g',2);
            }
            else if (hasInts())
            {
                int n = minInt + y * (maxInt-minInt) / h;
                return QString::number(n);
            }
            else if (hasTimes())
            {
                QString text;
                int timeVal = minTime + y *(maxTime-minTime)/h;
                //convert secs to h:m:s

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
                return text;
            }
            return QString(); 
        }

        void calcScales()
        {
            if (hasDoubles())
            {
                minDbl = 9999;
                maxDbl = 0;
                for (size_t i=0; i < doubles.size(); i++)
                {
                    if (doubles[i] > maxDbl)
                        maxDbl = doubles[i];
                    if ((doubles[i]>=0) && doubles[i] < minDbl)
                        minDbl = doubles[i];
                }

                maxDbl = abs(maxDbl)+1;
                minDbl = abs(minDbl)-1;
                if (minDbl < 0) minDbl = 0;
            }
            else if (hasInts())
            {
                minInt = 9999;
                maxInt = 0;

                for (size_t i=0; i < integers.size(); i++)
                {
                    if (integers[i] > maxInt)
                        maxInt = integers[i];
                    if ((integers[i]>=0) && integers[i] < minInt)
                        minInt = integers[i];
                }

                maxInt = maxInt - (maxInt%10) + 10;
                minInt = minInt - (minInt%10) - 10;
                if (minInt < 0) minInt = 0;
            }
            else if (hasTimes())
            {
                minTime = 0;
                maxTime = 0;

                for (size_t i=0; i < seconds.size(); i++)
                {
                    if (seconds[i] > maxTime)
                        maxTime = seconds[i];
                }
                maxTime = maxTime + (maxTime/60);
            }
            scaled = true;
        }

        // template and replace these
        std::vector<double> doubles;
        double minDbl,maxDbl;

        std::vector<int> integers;
        int minInt, maxInt;

        std::vector<int> seconds;
        int minTime, maxTime;

        QString label;

    private:
        bool scaled;
    };


    GraphWidget(QWidget *parent = 0);

protected:
    void paintEvent(QPaintEvent *event);

public:
    void setGraphs(bool effic, bool stroke, bool rate, bool speed);

    void clear();

	void drawSeries( QPainter &painter,
                     QColor pen,
                     int set);

    void drawBars( QPainter &painter,
                   QColor pen,
                   int set);
    void drawBars( QPainter &painter,
                   QColor pen,
                   QColor pen2,
                   int set,
                   int set2);

	void drawVAxis( QPainter &painter,
				    QColor pen,
                    int set );

	void drawXAxis(QPainter &painter,
				   QColor pen );
	
    //All series on same axis
    std::vector<QString> xaxis;
    std::vector<Series> series;

    Style style;

 private:    
    //Enabled graphs
    bool effic;
    bool rate;
    bool speed;
    bool stroke;

};

#endif
