#ifndef CHATSOCKET_H
#define CHATSOCKET_H

#include "database.h"
#include "protocol.h"
#include <QQueue>
#include <QTcpSocket>
#include <QDateTime>

class QTimer;
class ChatSocket : public QTcpSocket
{
    Q_OBJECT

public:
    ChatSocket(qintptr socketDescriptor, QObject *parent = nullptr);
    ~ChatSocket();

signals:
    void clientLoginSuccess(const QString &username, const QString &ip);
    void clientDisconnected(const QString &username);
    void hasNewMessage(const QByteArray &sender, const QByteArray &receiver,
                       msg_t type, msg_option_t option, const QByteArray &data);
    void logMessage(const QString &message);

public slots:
    void writeClientData(const QByteArray &sender, msg_t type, msg_option_t option,
                         const QByteArray &data);

private slots:
    void heartbeat();
    void continueWrite(qint64 sentSize);
    void checkHeartbeat();
    void onDisconnected();
    void processNextSendMessage();
    void processRecvMessage();
    //将查询到的数据转换成JSON并发送回客户端
    void toJsonAndSend(const UserInfo &info, const QMap<QString, QStringList> &friends);

private:
    QTimer *m_heartbeat;
    QDateTime m_lastTime;
    qint64 m_sendDataBytes;
    QByteArray m_sendData;
    QByteArray m_recvData;
    MessageHeader m_recvHeader;

    QByteArray m_username;
    QQueue<Message *> m_messageQueue;
    bool m_hasMessageProcessing;          //指示是否有消息在处理中
    Database *m_database;
};

#endif // CHATSOCKET_H
