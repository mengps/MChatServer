#ifndef CHATTCPSERVER_H
#define CHATTCPSERVER_H
#include <QTcpServer>
#include <QPointer>

class QQmlEngine;
class QQuickWindow;
class ChatTcpServer : public QTcpServer
{
    Q_OBJECT

public:
    ChatTcpServer(QQmlEngine *engine, QObject *parent = nullptr);
    ~ChatTcpServer();

    void loadWindow();

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    QQmlEngine *m_qmlengine;
    QPointer<QQuickWindow> m_window;
};

#endif // CHATTCPSERVER_H
