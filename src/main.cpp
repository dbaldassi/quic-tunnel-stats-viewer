#include <QApplication>
#include <iostream>

#include "main_window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow window;

    if(argc < 2) {
        std::cout << "Error: missing results path argument" << "\n\n"
                  << "Usage : " << argv[0] << " <path_to_result>"
                  << std::endl;
        return EXIT_FAILURE;
    }

    window.set_stats_dir(argv[1]);
    window.load();
    window.show();

    return app.exec();
}
