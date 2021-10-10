#include "distances.h"

#include <QStringList>
#include <QRegularExpression>
#include <QSettings>

namespace
{

    const std::map<QString, double> units = {{"m", 1.0}, {"y", 0.9144}, {"mi", 1609.344}};

    Distance convertToken(const QString & token)
    {
        const QRegularExpression re("^(\\d+\\.?\\d*)(\\w+)$");
        const QRegularExpressionMatch match = re.match(token);
        if (match.hasMatch())
        {
            const QString number = match.captured(1);
            const QString unit = match.captured(2);
            Distance distance;
            distance.label = token;
            bool ok;
            distance.distance = number.toDouble(&ok) * units.at(unit);
            if (ok)
            {
                return distance;
            }
        }
        throw std::runtime_error("Cannot convert " + token.toStdString());
    }

    std::vector<Distance> parseDistanceString(const QString & s)
    {
        if (s.isEmpty())
        {
            throw std::runtime_error("Cannot convert an empty string");
        }
        const QString s1 = s.toLower().remove(' ');
        QStringList tokens = s1.split(",");
        std::vector<Distance> result;

        for (const QString & token : tokens)
        {
            result.push_back(convertToken(token));
        }

        std::sort(result.begin(), result.end(), [](const auto & lhs, const auto & rhs) { return lhs.distance < rhs.distance;});
        return result;
    }

    // make sure this string can be parsed!
    const QString defaultDistances = "50m,100m,200m,400m,800m,1500m,1.2mi,2000m,2.4mi,5000m";
}

std::vector<Distance> getDistanceFromConfig()
{
    QSettings settings("Swim", "Poolmate");

    const QString s = settings.value("distances").toString();

    try
    {
        return parseDistanceString(s);
    }
    catch (const std::runtime_error & )
    {
        return parseDistanceString(defaultDistances);
    }
}
