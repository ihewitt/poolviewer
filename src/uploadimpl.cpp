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

#include <QFileDialog>

#include <stdio.h>
#include "uploadimpl.h"
#include "syncimpl.h"

#include "datastore.h"

#include "FIT.hpp"

bool ReadCSV( const std::string & name, std::vector<ExerciseSet>& );
bool SaveCSV( const std::string & name, std::vector<ExerciseSet>& );

UploadImpl::UploadImpl( QWidget * parent, Qt::WindowFlags f) 
    : QDialog(parent, f)
{
    exdata.clear();
    setupUi(this);
}

void UploadImpl::fillList()
{
    std::vector<ExerciseSet>::iterator i;
    
    int pos=0;
    QDateTime run;
    for (i=exdata.begin(); i!= exdata.end(); ++i, ++pos)
    {
        if (run != QDateTime(i->date,i->time))
        {
            run = QDateTime(i->date,i->time);
            
            // TODO replace this with custom drawn control
            QString line = QString("[%1] \t%2")
                .arg(run.toString("yyyy/MM/dd hh:mm"))
                .arg(i->lengths);
            
            QListWidgetItem* i = new QListWidgetItem(line);
            i->setData(Qt::UserRole, pos);

// if set already uploaded, check and disable
            if (ds->findExercise(run) >= 0)
            {
                //                i->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
                i->setFlags(0);
                i->setCheckState(Qt::Checked);
            }
            else
            {
                i->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
                i->setCheckState(Qt::Unchecked);
            }
            listWidget->addItem(i);
        }
    }
}

/*
  void UploadImpl::exportButton()
  {
  QString file;
  // change to use suffix.
  // fileDialog.setDefaultSuffix("csv");
  file = QFileDialog::getSaveFileName(this,
  tr("Export Exercise Data"),
  "",
  tr("Comma separated files (*.csv *.txt)"));
  if (!file.isEmpty())
  {
  SaveCSV(qPrintable(file), exdata, false);
  }
  }
*/

void UploadImpl::importButton()
{
    QString file;
    file = QFileDialog::getOpenFileName(this,
                                        tr("Import Exercise Data"),
                                        "",
                                        tr("Comma separated files (*.csv *.txt);;Garmin FIT file (*.fit)"));
    
    if (!file.isEmpty())
    {
	if ( QFileInfo(file.toLower()).suffix() == "fit") {
	  // Garmin FIT file
            QFile fitFile(file);
            if (!fitFile.open(QIODevice::ReadOnly)) return;
            QByteArray blob = fitFile.readAll();            
            std::vector<uint8_t> fitData(blob.begin(), blob.end());
           
            FIT fit;
            fit.parse (fitData, exdata);
	  
	} else {
	  // csv file
	  ReadCSV(qPrintable(file), exdata);
	}
        if (exdata.size())
          {
              fillList();
          }

    }
}

void UploadImpl::selectAll()
{
    for (int n=0; n< listWidget->count(); ++n)
    {
        QListWidgetItem* i = listWidget->item(n);

        if ( i->flags() & Qt::ItemIsEnabled )
        {
            i->setCheckState(Qt::Checked);
        }
    }
}

void UploadImpl::selectNone()
{
    for (int n=0; n< listWidget->count(); ++n)
    {
        QListWidgetItem* i = listWidget->item(n);

        if ( (i->checkState() == Qt::Checked) &&
             (i->flags() & Qt::ItemIsEnabled) )
        {
            i->setCheckState(Qt::Unchecked);            
        }
    }
}

void UploadImpl::syncButton()
{
    SyncImpl win(this);
    win.exec();

    win.getData(exdata);
    fillList();
}

/*
 * Add highlighted entries into main data store
 */
void UploadImpl::add()
{
    // checked rather than selected seems more intuitive
    //	QList<QListWidgetItem *> items;
    //  items = listWidget->selectedItems();
	
    for (int n=0; n< listWidget->count(); ++n)
    {
        QListWidgetItem* i = listWidget->item(n);

        if ( (i->checkState() == Qt::Checked) &&
             (i->flags() & Qt::ItemIsEnabled) )
        {
            // userrole data contains location exercise list
            unsigned int pos = i->data(Qt::UserRole).toInt();

            //Disable once uploaded
            i->setFlags(0);
            i->setCheckState(Qt::Checked);
            
            // TODO add session ids to remove this date grouping hack.
            QDateTime initial(exdata[pos].date, exdata[pos].time);

            std::vector<ExerciseSet> sets;
            while (pos < exdata.size() &&
                   QDateTime(exdata[pos].date, exdata[pos].time) == initial)
            {
                sets.push_back(exdata[pos++]);
            }
            if (sets.size())
                ds->add(sets);
        }
    }
    close();
}
