#include "received_bitrate_display.h"

#include <QTabWidget>
#include <QListWidget>

#include <iostream>

#include "csv_reader.h"

const QColor ReceivedBitrateDisplay::LINK_COLOR = QColorConstants::Black;
const QColor ReceivedBitrateDisplay::BITRATE_COLOR = QColorConstants::Red;
const QColor ReceivedBitrateDisplay::FPS_COLOR = QColorConstants::Red;
const QColor ReceivedBitrateDisplay::FRAME_DROPPED_COLOR = QColorConstants::DarkBlue;
const QColor ReceivedBitrateDisplay::FRAME_DECODED_COLOR = QColorConstants::DarkGreen;
const QColor ReceivedBitrateDisplay::FRAME_KEY_DECODED_COLOR = QColorConstants::Magenta;
const QColor ReceivedBitrateDisplay::FRAME_RENDERED_COLOR = QColorConstants::DarkYellow;

const QColor ReceivedBitrateDisplay::QUIC_SENT_COLOR = QColorConstants::Green;


ReceivedBitrateDisplay::ReceivedBitrateDisplay(QWidget* tab, QListWidget* legend)
    : _tab(tab), _legend(legend)
{

}

void ReceivedBitrateDisplay::create_legend_item(const QString& text, const QColor& color)
{
    QListWidgetItem * item = new QListWidgetItem(_legend);
    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    item->setCheckState(Qt::Unchecked);
    item->setForeground(QBrush(color));
    item->setText(text);
}

void ReceivedBitrateDisplay::create_legend()
{
    create_legend_item("link", LINK_COLOR);
    create_legend_item("Received bitrate", BITRATE_COLOR);
    create_legend_item("QUIC sent bitrate", QUIC_SENT_COLOR);
    create_legend_item("FPS", FPS_COLOR);
    create_legend_item("frame dropped", FRAME_DROPPED_COLOR);
    create_legend_item("Frame decoded", FRAME_DECODED_COLOR);
    create_legend_item("Frame key decoded", FRAME_KEY_DECODED_COLOR);
    create_legend_item("Frame rendered bitrate", FRAME_RENDERED_COLOR);
}

// bitrate.csv : time, bitrate, link, fps, frame_dropped, frame_decoded, frame_keydecoded, frame_rendered
// quic.csv : time, bitrate
void ReceivedBitrateDisplay::load(const fs::path& p)
{
    create_legend();

    fs::path path = p / "bitrate.csv";

    using BitrateReader = CsvReaderTypeRepeat<',', int, 8>;

    for(auto &it : BitrateReader(path)) {
        const auto& [time, bitrate, link, fps, frame_dropped, frame_decoded, frame_key_decoded, frame_rendered] = it;

        std::cout << time << " "
                  << bitrate << " "
                  << link << " "
                  << fps << " "
                  << frame_dropped << " "
                  << frame_decoded << " "
                  << frame_key_decoded << " "
                  << frame_rendered << " "
                  << std::endl;
    }
}

void ReceivedBitrateDisplay::unload(const fs::path& path)
{

}
