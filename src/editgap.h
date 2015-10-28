#ifndef EDITGAP_H
#define EDITGAP_H

#include "ui_editgap.h"
#include "datastore.h"

class EditGap : public QDialog, private Ui::EditGap
{
    Q_OBJECT

public:
    explicit EditGap(QWidget *parent = 0);

    void setOriginalSet(const Set * _set);
    const Set & getModifiedSet() const;

private slots:
    void on_lengthsSpin_valueChanged(int arg1);

private:

    void update();

    const Set * original;
    Set modified;

    double gap;
};

#endif // EDITGAP_H
