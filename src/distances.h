#ifndef DISTANCES_H
#define DISTANCES_H

#include <QString>
#include <vector>

struct Distance
{
    double distance; // in meters
    QString label;
};


std::vector<Distance> getDistanceFromConfig();

#endif // DISTANCES_H
