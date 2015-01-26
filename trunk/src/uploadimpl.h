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

#ifndef UPLOADIMPL_H
#define UPLOADIMPL_H
//
#include <QDialog>
#include "ui_upload.h"
#include "exerciseset.h"

//
class DataStore;
class UploadImpl : public QDialog, public Ui::UploadDlg
{
Q_OBJECT
public:
    UploadImpl( QWidget * parent = 0, Qt::WindowFlags f = 0 );

	void fillList();
	void setDataStore(DataStore *_ds) { ds = _ds;}

private slots:

	virtual void selectAll();
	virtual void selectNone();
	virtual void syncButton();
	virtual void importButton();
    //	virtual void exportButton();
	virtual void add();


private:
	std::vector<ExerciseSet> exdata;
	DataStore *ds;

};

#endif
