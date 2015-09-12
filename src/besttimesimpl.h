#ifndef BESTTIMESIMPL_H
#define BESTTIMESIMPL_H

#include "ui_besttimesimpl.h"

class DataStore;

class BestTimesImpl : public QDialog, private Ui::besttimesimpl
{
    Q_OBJECT

public:
    explicit BestTimesImpl(QWidget *parent = 0);

    void setDataStore(const DataStore *_ds);

private slots:
    void on_pushButton_clicked();

private:
    const DataStore *ds;
};

#endif // BESTTIMESIMPL_H
