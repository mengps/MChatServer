#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QNetworkInterface>
#include <QThread>
#include "chattcpserver.h"

ChatTcpServer::ChatTcpServer(QQmlEngine *engine, QObject *parent)
    :   m_qmlengine(engine), QTcpServer(parent)
{

}

ChatTcpServer::~ChatTcpServer()
{

}

void ChatTcpServer::loadWindow()
{
    QQmlComponent component(m_qmlengine, "qrc:/qml/main.qml");
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
    connect(socket, &ChatSocket::hasNewMessage, this, &ChatTcpServer::disposeMessage);
    connect(socket, &ChatSocket::consoleMessage, this, [this](const QString &message)
    {
        QMetaObject::invokeMethod(m_window, "addMessage", Q_ARG(QVariant, QVariant(message)));
    });
    connect(socket, &ChatSocket::clientLoginSuccess, this, [this](const QString &username, const QString &ip)
    {
        ChatSocket *socket = qobject_cast<ChatSocket *>(sender());
        m_users[username] = socket;
        QMetaObject::invokeMethod(m_window, "addNewClient", Q_ARG(QVariant, username), Q_ARG(QVariant, ip));
    });
    connect(socket, &ChatSocket::clientDisconnected, this, [this](const QString &username)
    {
        m_users.remove(username);
        QMetaObject::invokeMethod(m_window, "removeClient", Q_ARG(QVariant, QVariant(username)));
    });

    socket->moveToThread(thread);
    thread->start();
}

void ChatTcpServer::disposeMessage(const QByteArray &sender, const QByteArray &receiver, MSG_TYPE type, const QByteArray &data)
{
    if (m_users.contains(QString(receiver)))
        QMetaObject::invokeMethod(m_users[QString(receiver)], "writeClientData",  Q_ARG(QByteArray, sender),
                Q_ARG(MSG_TYPE, type),  Q_ARG(QByteArray, data));
    else
    {
     //write to database
    }
}
