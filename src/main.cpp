#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "chattcpserver.h"
#include "framelesswindow.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<FramelessWindow>("an.framelessWindow", 1, 0, "FramelessWindow");
    qRegisterMetaType<MSG_TYPE>("MSG_TYPE");
    qRegisterMetaType<MSG_OPTION_TYPE>("MSG_OPTION_TYPE");

    QQmlApplicationEngine engine;
    ChatTcpServer server(&engine);
    if (!server.listen(QHostAddress::AnyIPv4, 43800))
    {
        QGuiApplication::exit(1);
    }
    else server.loadWindow();

    return app.exec();
}
