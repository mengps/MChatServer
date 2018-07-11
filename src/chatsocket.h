#ifndef CHATSOCKET_H
#define CHATSOCKET_H

#include <QTcpSocket>
#include <QDateTime>
#include <QQueue>
#include "database.h"
#include "mymessagedef.h"

class QTimer;
class ChatSocket : public QTcpSocket
{
    Q_OBJECT

public:
    ChatSocket(qintptr socketDescriptor, QObject *parent = nullptr);
    ~ChatSocket();

public slots:
    void writeClientData(const QByteArray &sender, MSG_TYPE type, MSG_OPTION_TYPE option, QByteArray data);
    void readClientData();

private slots:
    void heartbeat();
    void continueWrite(qint64 sentSize);
    void checkHeartbeat();
    void onDisconnected();
    void processNextMessage();
    //将查询到的数据转换成JSON并发送回客户端
    void toJsonStringAndSend(const UserInfo &info, const QMap<QString, QStringList> &friends);

signals:
    void clientLoginSuccess(const QString &username, const QString &ip);
    void clientDisconnected(const QString &username);
    void hasNewMessage(const QByteArray &sender, const QByteArray &receiver, MSG_TYPE type, MSG_OPTION_TYPE option, const QByteArray &data);
    void consoleMessage(const QString &message);

private:
    QTimer *m_heartbeat;
    QDateTime m_lastTime;
    QByteArray m_data;
    qint64 m_fileBytes;
    QByteArray m_username;
    QQueue<Message *> m_messageQueue;
    bool m_hasMessageProcessing;          //指示是否有消息在处理中
    Database *m_database;
};

#endif // CHATSOCKET_H
