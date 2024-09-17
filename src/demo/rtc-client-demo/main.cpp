#include "rtc-client-demo.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	jukey::demo::RtcClientDemo demo;
	if (!demo.Init(&app)) {
		return -1;
	}
	demo.show();
	
	return app.exec();
}
