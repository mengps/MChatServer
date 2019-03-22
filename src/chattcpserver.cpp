#include "chattcpserver.h"

#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QThread>

ChatTcpServer::ChatTcpServer(QQmlEngine *engine, QObject *parent)
    : QTcpServer(parent)
    , m_qmlengine(engine)
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

    QMetaObject::invokeMethod(m_window, "displayServerInfo",
                              Q_ARG(QVariant, QVariant(server_ip)), Q_ARG(QVariant, QVariant(43800)));
}

void ChatTcpServer::incomingConnection(qintptr socketDescriptor)
{
    QThread *thread = new QThread;
    ChatSocket *socket = new ChatSocket(socketDescriptor);

    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    connect(socket, &ChatSocket::hasNewMessage, this, &ChatTcpServer::disposeMessage);
    connect(socket, &ChatSocket::logMessage, this, [this](const QString &message)
    {
        QMetaObject::invokeMethod(m_window, "addMessage", Q_ARG(QVariant, QVariant(message)));
    });
    connect(socket, &ChatSocket::clientLoginSuccess, this, [this](const QString &username, const QString &ip)
    {
        ChatSocket *socket = qobject_cast<ChatSocket *>(sender());
        m_users[username] = socket;
        QMetaObject::invokeMethod(m_window, "addNewClient", Q_ARG(QVariant, username), Q_ARG(QVariant, ip),
                                  Q_ARG(QVariant, socket->status()));
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

void ChatTcpServer::saveRecord(const QByteArray &sender, const QByteArray &receiver, const QByteArray &data)
{
    //以后会设计一个专门格式化存储聊天记录的工具
    QFile file("users/" + QString(sender) + "/messageText/MSG" + QString(receiver) + ".txt");
    file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    QTextStream out(&file);
    out << "[time:" << QDateTime::currentDateTime().toString("yyyyMMdd hhmmss") << "]" << endl
        << "[data:" << QString::fromLocal8Bit(data) << "]" << endl;
    file.close();

    file.setFileName("users/" + QString(receiver) + "/messageText/MSG" + QString(sender) + ".txt");
    file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    out << "[time:" << QDateTime::currentDateTime().toString("yyyyMMdd hhmmss") << "]" << endl
        << "[data:" << QString::fromLocal8Bit(data) << "]" << endl;
    file.close();
}

void ChatTcpServer::writeDataToClient(const QByteArray &sender, const QByteArray &receiver, msg_t type, const QByteArray &data)
{
    //因为在不同的线程中，使用invokeMethod调用
    //此函数简化了操作
    QMetaObject::invokeMethod(m_users[QString(receiver)], "writeClientData",  Q_ARG(QByteArray, sender),
            Q_ARG(msg_t, type),  Q_ARG(msg_option_t, MO_NULL), Q_ARG(QByteArray, data));
}

void ChatTcpServer::disposeMessage(const QByteArray &sender, const QByteArray &receiver, msg_t type, msg_option_t option, const QByteArray &data)
{
    switch (type)
    {
    case MT_TEXT:
    {
        //将双方的消息存入
        saveRecord(sender, receiver, data);
        if (m_users.contains(QString(receiver)))    //如果另一方在线
            writeDataToClient(sender, receiver, MT_TEXT, data);
        else m_database->addUnreadMessage(QString(sender), QString(receiver));    //不在线则unreadMessage+1，下次登录时发送
        break;
    }
    case MT_STATECHANGE:
    {
        QMetaObject::invokeMethod(m_window, "stateChange", Q_ARG(QVariant, QString(sender)), Q_ARG(QVariant, m_users[sender]->status()));
        QStringList friends = m_database->getUserFriends(sender);
        for (auto it : friends)
        {
            if (m_users.contains(it))    //如果好友在线，则为其发送状态更新
                writeDataToClient(sender, it.toLatin1(), MT_STATECHANGE, data);
        }
        break;
    }
    case MT_ADDFRIEND:
    {
        if (m_users.contains(QString(receiver)))    //如果另一方在线
            writeDataToClient(sender, receiver, MT_ADDFRIEND, ADDFRIEND);
    }

    default:
        break;
    }
}
