#ifndef CHATTCPSERVER_H
#define CHATTCPSERVER_H

#include "chatsocket.h"
#include <QTcpServer>
#include <QPointer>
#include <QMap>

class QQmlEngine;
class QQuickWindow;
class ChatSocket;
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
    void disposeMessage(const QByteArray &sender, const QByteArray &receiver, msg_t type, msg_option_t option, const QByteArray &data);

private:
    Database *m_database;
    QQmlEngine *m_qmlengine;
    QPointer<QQuickWindow> m_window;
    QMap<QString, ChatSocket*> m_users;
};

#endif // CHATTCPSERVER_H
