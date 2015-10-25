#include "utilities.h"

#include <QVariant>
#include <QTableWidgetItem>
#include "datastore.h"

QTableWidgetItem * createTableWidgetItem(const QVariant & content)
{
    QTableWidgetItem * item = new QTableWidgetItem();
    item->setFlags(item->flags() ^ Qt::ItemIsEditable);
    item->setData(Qt::DisplayRole, content);
    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

    return item;
}

// sums the duration of all the lanes
QTime getActualSwimTime(const Set & set)
{
    QTime duration(0, 0);

    for (size_t i = 0; i < set.times.size(); ++i)
    {
        duration = duration.addMSecs(set.times[i] * 1000);
    }
    return duration;
}
