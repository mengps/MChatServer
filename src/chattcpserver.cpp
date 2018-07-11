#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QNetworkInterface>
#include <QThread>
#include <QDateTime>
#include "chattcpserver.h"

ChatTcpServer::ChatTcpServer(QQmlEngine *engine, QObject *parent)
    :   m_qmlengine(engine), QTcpServer(parent)
{
    m_database = new Database("ServerConnection", this);
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
                              Q_ARG(QVariant, QVariant("127.0.0.1 本地主机")), Q_ARG(QVariant, QVariant(43800)));
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
        qDebug() << m_users;
    });

    socket->moveToThread(thread);
    thread->start();
}

void ChatTcpServer::disposeMessage(const QByteArray &sender, const QByteArray &receiver, MSG_TYPE type, MSG_OPTION_TYPE option, const QByteArray &data)
{
    //将双方的消息存入
    QFile file("users/" + QString(sender) + "/messageText/MSG" + QString(sender) + ".txt");
    file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    QTextStream out(&file);
    out << "[time:" << QDateTime::currentDateTime().toString("yyyyMMdd hhmmss") << "]" << endl
        << "[type:" << type << "]" << endl
        << "[option:" << option << "]" << endl
        << "[data:" << QString::fromLocal8Bit(data) << "]" << endl;
    file.close();

    file.setFileName("users/" + QString(receiver) + "/messageText/MSG" + QString(receiver) + ".txt");
    file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    out << "[time:" << QDateTime::currentDateTime().toString("yyyyMMdd hhmmss") << "]" << endl
        << "[type:" << type << "]" << endl
        << "[option:" << option << "]" << endl
        << "[data:" << QString::fromLocal8Bit(data) << "]" << endl;
    file.close();

    if (m_users.contains(QString(receiver)))    //如果另一方在线
        QMetaObject::invokeMethod(m_users[QString(receiver)], "writeClientData",  Q_ARG(QByteArray, sender),
                Q_ARG(MSG_TYPE, type),  Q_ARG(MSG_OPTION_TYPE, MO_NULL), Q_ARG(QByteArray, data));
    else m_database->addUnreadMessage(QString(receiver));    //不在线则unreadMessage+1，下次登录时发送
}
