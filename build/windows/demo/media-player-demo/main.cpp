#include "media-player-demo.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
    
	jukey::demo::MediaPlayerDemo demo;

	if (!demo.Init(&app)) {
		return -1;
	}
	demo.show();

	return app.exec();
}
