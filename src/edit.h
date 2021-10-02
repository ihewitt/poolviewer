/*
 * This file is part of PoolViewer
 * Copyright (c) 2015 Ivor Hewitt
 * Copyright (c) 2015 Andrea Odetti
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

#ifndef EDIT_H
#define EDIT_H

#include "ui_edit.h"
#include "datastore.h"

class Edit : public QDialog, private Ui::Edit
{
    Q_OBJECT

public:
    explicit Edit(QWidget *parent, const Workout &workout );

//    bool setWorkout(const Workout & _wrk);
    bool getModifiedWrk(Workout &wrk) const;
    bool isDeleted() const { return deleted; }

private slots:
    void on_gapTimeUsedSpin_valueChanged(double arg1);

    void on_newLengthsSpin_valueChanged(int arg1);

    void on_deleteAllButton_clicked();

    void on_deleteButton_clicked();

    void on_adjustButton_clicked();

    void on_setsGrid_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);

    void on_revertButton_clicked();

    void on_strokesEdit_valueChanged(int arg1);

    void on_deleteLength_clicked();

    void on_squashLength_clicked();

    void on_balanceLength_clicked();

    void on_lengthsGrid_itemSelectionChanged();

private:

    void calculate();

    const Workout & original;
    Workout modified;

    int currentSet;
    Set setOrig;
    Set setMod;

    double gap;

    bool deleted;
    bool changed;

    void populate(const Workout& _wrk);
    void fillLengths(const Set & set);
};

#endif // EDIT_H
