#ifndef RECEIVEDBITRATEDISPLAY_H
#define RECEIVEDBITRATEDISPLAY_H

#include <QString>
#include <QColor>

#include <filesystem>

class QWidget;
class QListWidget;

namespace fs = std::filesystem;

class ReceivedBitrateDisplay
{
    QWidget* _tab;
    QListWidget    * _legend;

    void create_legend_item(const QString& text, const QColor& color);
    void create_legend();
public:

    // bitrate.csv
    static const QColor LINK_COLOR;
    static const QColor BITRATE_COLOR;
    static const QColor FPS_COLOR;
    static const QColor FRAME_DROPPED_COLOR;
    static const QColor FRAME_DECODED_COLOR;
    static const QColor FRAME_KEY_DECODED_COLOR;
    static const QColor FRAME_RENDERED_COLOR;

    // quic.csv
    static const QColor QUIC_SENT_COLOR;

    ReceivedBitrateDisplay(QWidget* tab, QListWidget* legend);
    ~ReceivedBitrateDisplay() = default;

    // load bitrate.csv, quic.csv
    void load(const fs::path& path);
    void unload(const fs::path& path);
};

#endif // RECEIVEDBITRATEDISPLAY_H
