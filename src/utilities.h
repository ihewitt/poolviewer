#ifndef UTILITIES_H
#define UTILITIES_H

#include <QTime>

class QTableWidgetItem;
class QVariant;
class Set;

// ReadOnly, horizontal Right aligned, non editable
QTableWidgetItem * createTableWidgetItem(const QVariant & content);

// sums the duration of all the lanes
QTime getActualSwimTime(const Set & set);

#endif // UTILITIES_H
