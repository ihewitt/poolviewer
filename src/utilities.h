#ifndef UTILITIES_H
#define UTILITIES_H

class QTableWidgetItem;
class QVariant;

// ReadOnly, horizontal Right aligned, non editable
QTableWidgetItem * createTableWidgetItem(const QVariant & content);

#endif // UTILITIES_H
