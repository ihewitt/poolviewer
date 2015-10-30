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
