#ifndef CHATTCPSERVER_H
#define CHATTCPSERVER_H
#include <QTcpServer>
#include <QPointer>
#include <QMap>
#include "chatsocket.h"

class QQmlEngine;
class QQuickWindow;
class ChatSocket;
class ChatTcpServer : public QTcpServer
{
    Q_OBJECT

public:
    ChatTcpServer(QQmlEngine *engine, QObject *parent = nullptr);
    ~ChatTcpServer();

    void loadWindow();

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void disposeMessage(const QByteArray &sender, const QByteArray &receiver, MSG_TYPE type, const QByteArray &data);

private:
    QQmlEngine *m_qmlengine;
    QPointer<QQuickWindow> m_window;
    QMap<QString, ChatSocket*> m_users;
};

#endif // CHATTCPSERVER_H
