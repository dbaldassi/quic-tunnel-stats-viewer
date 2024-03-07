#ifndef DISPLAYBASE_H
#define DISPLAYBASE_H

#include <QString>
#include <QMap>
#include <QChart>
#include <QLineSeries>

#include <filesystem>

namespace fs = std::filesystem;

class QWidget;
class QListWidget;
class QLineSeries;
class StatsLineChart;
class StatsLineChartView;
class QTreeWidget;

class DisplayBase
{

protected:
    using StatMap = QMap<uint8_t, std::tuple<QString, QLineSeries*, QChart*>>;
    QMap<QString, StatMap> _path_keys;

    QWidget     * _tab;
    QListWidget * _legend;
    QTreeWidget * _info;

    enum StatsKeyProperty : uint8_t
    {
        NAME,
        SERIE,
        CHART,
        NUM_KEY
    };

    template<typename T>
    void add_point(const QString& path, uint8_t key, const T& point)
    {
        auto map = _path_keys[path];
        auto s = std::get<StatsKeyProperty::SERIE>(map[key]);
        *s << point;
    }

    inline void add_serie(const QString& path, uint8_t key)
    {
        const auto& map = _path_keys[path];
        auto* serie = std::get<StatsKeyProperty::SERIE>(map[key]);
        auto* chart = std::get<StatsKeyProperty::CHART>(map[key]);

        if(chart) chart->addSeries(serie);
    }

    void create_legend();
    StatsLineChart * create_chart();
    StatsLineChartView * create_chart_view(QChart* chart);
    void create_serie(const fs::path&p, uint8_t key);

    virtual void init_map(StatMap& map, bool signal = true) = 0;

public:
    DisplayBase(QWidget* tab, QListWidget* legend, QTreeWidget* info);

    virtual void load(const fs::path& path) = 0;
    virtual void unload(const fs::path& path);
};

#endif // DISPLAYBASE_H
