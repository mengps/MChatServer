#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QNetworkInterface>
#include <QThread>
#include "chattcpserver.h"
#include "chatsocket.h"

ChatTcpServer::ChatTcpServer(QQmlEngine *engine, QObject *parent)
    :   m_qmlengine(engine), QTcpServer(parent)
{

}

ChatTcpServer::~ChatTcpServer()
{

}

void ChatTcpServer::loadWindow()
{
    QQmlComponent component(m_qmlengine, "qrc:/main.qml");
    QObject *obj = component.create();
    m_window = qobject_cast<QQuickWindow *>(obj);
    m_window->requestActivate();
    m_window->show();

    QString ipAddress;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();

    for (int i = 0; i < ipAddressesList.size(); ++i)
    {
        if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
                ipAddressesList.at(i).protocol() == QAbstractSocket::IPv4Protocol)
        {
            ipAddress = ipAddressesList.at(i).toString();
            break;
        }
    }

    if (ipAddress.isEmpty())
        ipAddress = QHostAddress(QHostAddress::LocalHost).toString();

    QMetaObject::invokeMethod(m_window, "displayServerInfo",
                              Q_ARG(QVariant, QVariant("10.103.0.13")), Q_ARG(QVariant, QVariant(43800)));
}

void ChatTcpServer::incomingConnection(qintptr socketDescriptor)
{
    QThread *thread = new QThread;
    ChatSocket *socket = new ChatSocket(socketDescriptor);

    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    connect(socket, &ChatSocket::consoleMessage, this, [this](const QString &message)
    {
        QMetaObject::invokeMethod(m_window, "addMessage", Q_ARG(QVariant, QVariant(message)));
    });
    connect(socket, &ChatSocket::clientDisconnected, this, [this](const QString &ip)
    {
        QMetaObject::invokeMethod(m_window, "removeClient", Q_ARG(QVariant, QVariant(ip)));
    });
    QMetaObject::invokeMethod(m_window, "addNewClient", Q_ARG(QVariant, QVariant(socket->peerAddress().toString())));

    socket->moveToThread(thread);
    thread->start();
}
