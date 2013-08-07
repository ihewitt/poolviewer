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

#include <stdio.h>
#include <QTextCharFormat>
#include "dialogimpl.h"

std::vector<Workout> workouts;

void LoadData()
{
	//Fake load

	Workout work1;
	Set set1;
	Set set2;

	set1.len=10;
	set2.len=5;

	work1.sets.push_back(set1);
	work1.sets.push_back(set2);
	workouts.push_back(work1);
}

//
DialogImpl::DialogImpl( QWidget * parent, Qt::WFlags f) 
	: QDialog(parent, f)
{
	setupUi(this);
	LoadData();

	QTextCharFormat c;
	c.setFontWeight(QFont::Bold);

	calendarWidget->setDateTextFormat(QDate(2011,12,7),c); 

	workoutGrid->showGrid();
	workoutGrid->setColumnCount(3);
	workoutGrid->setRowCount(3);
}

void DialogImpl::selectedDate(QDate)
{
	DEBUG("selectedd\n");
	//locate date and populate workouts


	std::vector<Workout>::iterator i;
	int x=0,y=0;
	for (i=workouts.begin(); i!= workouts.end(); ++i)
	{
		std::vector<Set>::iterator j;
		for (j=i->sets.begin(); j!=i->sets.end(); ++j)
		{
			QTableWidgetItem *item = new QTableWidgetItem(tr("%1").arg(x+y*3));
			workoutGrid->setItem(x,y,item);
			x++;
		}
		y++;

	}

}

void DialogImpl::selectedWorkout(QModelIndex)
{
	DEBUG("selectedw\n");
}


//
