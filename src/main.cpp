#include "chattcpserver.h"
#include "framelesswindow.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<FramelessWindow>("an.framelessWindow", 1, 0, "FramelessWindow");
    qRegisterMetaType<msg_t>("msg_t");
    qRegisterMetaType<msg_option_t>("msg_option_t");

    QQmlApplicationEngine engine;
    ChatTcpServer server(&engine);
    if (!server.listen(QHostAddress::AnyIPv4, server_port))
    {
        QGuiApplication::exit(1);
    }
    else server.loadWindow();

    return app.exec();
}
