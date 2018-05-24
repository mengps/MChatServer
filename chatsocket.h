#ifndef CHATSOCKET_H
#define CHATSOCKET_H

#include <QTcpSocket>
#include <QDateTime>
#include "mymessagedef.h"

class QTimer;
class ChatSocket : public QTcpSocket
{
    Q_OBJECT

public:
    ChatSocket(qintptr socketDescriptor, QObject *parent = nullptr);
    ~ChatSocket();

public slots:
    void writeClientData(MSG_TYPE type, QByteArray data);
    void readClientData();

private slots:
    void heartbeat();
    void continueWrite(qint64 sentSize);
    void checkHeartbeat();
    void onDisconnected();

signals:
    void clientDisconnected(const QString &ip);
    void consoleMessage(const QString &message);

private:
    QTimer *m_heartbeat;
    QDateTime m_lastTime;
    QByteArray m_data;
    qint64 m_fileBytes;
};

#endif // CHATSOCKET_H
