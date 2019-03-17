#ifndef CHATTCPSERVER_H
#define CHATTCPSERVER_H

#include "chatsocket.h"

#include <QMap>
#include <QPointer>
#include <QTcpServer>

class ChatSocket;
class QQmlEngine;
class QQuickWindow;
class ChatTcpServer : public QTcpServer
{
    Q_OBJECT

public:
    ChatTcpServer(QQmlEngine *engine, QObject *parent = nullptr);
    ~ChatTcpServer() override;

    void loadWindow();

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    //保存聊天记录
    void saveRecord(const QByteArray &sender, const QByteArray &receiver, const QByteArray &data);
    //写入数据到指定客户端连接
    void writeDataToClient(const QByteArray &sender, const QByteArray &receiver, msg_t type, const QByteArray &data);
    //处理消息
    void disposeMessage(const QByteArray &sender, const QByteArray &receiver, msg_t type, msg_option_t option, const QByteArray &data);

private:
    Database *m_database;
    QQmlEngine *m_qmlengine;
    QPointer<QQuickWindow> m_window;
    QMap<QString, ChatSocket*> m_users;
};

#endif // CHATTCPSERVER_H
