/*
 * This file is part of PoolViewer
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

#ifndef EDITGAP_H
#define EDITGAP_H

#include "ui_editgap.h"
#include "datastore.h"

class EditGap : public QDialog, private Ui::EditGap
{
    Q_OBJECT

public:
    explicit EditGap(QWidget *parent = 0);

    // returns true if it is possible to turn a gap into lanes
    // false in case: negative gap
    //                or gap < 1 sec
    // if false, one should NOT call exec!
    bool setOriginalSet(const Set * _set);
    const Set & getModifiedSet() const;

private slots:
    void on_gapTimeUsedSpin_valueChanged(double arg1);

    void on_newLengthsSpin_valueChanged(int arg1);

private:

    void calculate();

    const Set * original;
    Set modified;

    double gap;
};

#endif // EDITGAP_H
