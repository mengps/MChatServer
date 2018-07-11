#include <QTimer>
#include <QThread>
#include <QDataStream>
#include <QHostAddress>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "chatsocket.h"

ChatSocket::ChatSocket(qintptr socketDescriptor, QObject *parent)
    :   QTcpSocket(parent)
{
    if (!setSocketDescriptor(socketDescriptor))
    {
        emit consoleMessage(errorString());
        deleteLater();
    }

    m_heartbeat = new QTimer(this);
    m_heartbeat->setInterval(60000);
    m_fileBytes = 0;
    m_lastTime = QDateTime::currentDateTime();

    connect(this, &ChatSocket::readyRead, this, &ChatSocket::heartbeat);
    connect(this, &ChatSocket::bytesWritten, this, &ChatSocket::continueWrite, Qt::DirectConnection);
    connect(this, &ChatSocket::readyRead, this, &ChatSocket::readClientData);
    connect(this, &ChatSocket::disconnected, this, &ChatSocket::onDisconnected);
    connect(m_heartbeat, &QTimer::timeout, this, &ChatSocket::checkHeartbeat);

    m_heartbeat->start();                 //开始心跳
}

ChatSocket::~ChatSocket()
{

}

void ChatSocket::heartbeat()
{
    if (!m_heartbeat->isActive())
        m_heartbeat->start();    
    m_lastTime = QDateTime::currentDateTime();
}

void ChatSocket::continueWrite(qint64 sentSize)
{
    static int sentBytes = 0;
    sentBytes += sentSize;

    qDebug() << __func__ << sentBytes << m_fileBytes;
    if (sentBytes >= m_fileBytes)
    {
        m_fileBytes = 0;
        sentBytes = 0;
        m_data.clear();
        m_hasMessageProcessing = false;
        if (!m_messageQueue.isEmpty())
            processNextMessage();       //如果消息队列不为空，则继续处理下一条待发送消息
    }
    else
    {
        write(m_data);
        QThread::msleep(10);
    }
}

void ChatSocket::checkHeartbeat()
{
    if (m_lastTime.secsTo(QDateTime::currentDateTime()) >= 60)   //检测掉线，停止心跳
    {
        qDebug() << "heartbeat 超时, 即将断开连接";
        m_heartbeat->stop();
        disconnectFromHost();
    }
}

void ChatSocket::onDisconnected()
{
    emit clientDisconnected(m_username);
    emit consoleMessage(peerAddress().toString() + " 断开连接..");
    deleteLater();
}

void ChatSocket::processNextMessage()
{
    if (!m_hasMessageProcessing && !m_messageQueue.isEmpty())
    {
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_9);
        Message *message = m_messageQueue.dequeue();
        out << *message;
        m_data = block;
        m_fileBytes = block.size();
        m_hasMessageProcessing = true;
        delete message;

        write(block);
        flush();        //立即发送消息
    }
}

void ChatSocket::toJsonStringAndSend(const UserInfo &info, const QMap<QString, QStringList> &friends)
{
    QJsonObject object;
    object.insert("Username", info.username);
    object.insert("Nickname", info.nickname);
    object.insert("HeadImage", info.headImage);
    object.insert("Gender", info.gender);
    object.insert("Birthday", info.birthday);
    object.insert("Signature", info.signature);
    object.insert("UnreadMessage", info.unreadMessage);
    object.insert("Level", info.level);
    QJsonArray friendList;
    for (auto it = friends.constBegin(); it != friends.constEnd(); it++)
    {
        QJsonArray array;
        for (auto it2 : it.value())
        {
            UserInfo info2 = m_database->getUserInfo(it2);
            QJsonObject object2;
            object2.insert("Username", info2.username);
            object2.insert("Nickname", info2.nickname);
            object2.insert("HeadImage", info2.headImage);
            object2.insert("Gender", info2.gender);
            object2.insert("Birthday", info2.birthday);
            object2.insert("Signature", info2.signature);
            object2.insert("UnreadMessage", info2.unreadMessage);
            object2.insert("Level", info2.level);
            array.append(object2);


        }
        QJsonObject obj;
        obj.insert("Group", it.key());
        obj.insert("Friend", array);
        friendList.append(obj);
    }
    object.insert("FriendList", friendList);
    writeClientData(m_username, MT_USERINFO, MO_NULL, QJsonDocument(object).toJson());
    qDebug() << object;
}

void ChatSocket::writeClientData(const QByteArray &sender, MSG_TYPE type, MSG_OPTION_TYPE option, QByteArray data)
{
    QByteArray base64 = data.toBase64();
    MSG_MD5_TYPE md5 = QCryptographicHash::hash(base64, QCryptographicHash::Md5);

    MessageHeader header = { MSG_FLAG, type, base64.size(), option, sender, m_username, md5 };
    Message *message = new Message(header, base64);
    m_messageQueue.enqueue(message);
    processNextMessage();
}

void ChatSocket::readClientData()
{
    static int gotSize = 0;

    if (m_data.size() == 0)
    {
        QByteArray data;
        data = read(bytesAvailable());
        m_data.replace(gotSize, data.size(), data);
        gotSize += data.size();
        qDebug() << gotSize << m_data.size() << m_data;
    }

    if (gotSize == m_data.size())     //接收完毕
    {
        Message message;
        QDataStream in(&m_data, QIODevice::ReadOnly);
        in.setVersion(QDataStream::Qt_5_9);
        in >> message;
        QByteArray md5_t = QCryptographicHash::hash(message.data, QCryptographicHash::Md5);

        if (message.header.md5 == md5_t)   //正确的消息
        {
            QString str = QString::fromLocal8Bit(QByteArray::fromBase64(message.data));
            switch (message.header.type)
            {
            case MT_CHECK:
            {
                QStringList list = str.split("%%");
                qDebug() << "登录信息：" << list;
                m_database = new Database(QString::number((int)QThread::currentThreadId(), 16), this);
                UserInfo info = m_database->getUserInfo(list.at(0).toLocal8Bit());
                if (info.password == list.at(1).toLocal8Bit())
                {
                    writeClientData(QByteArray(), MT_CHECK, MO_NULL, CHECK_SUCCESS);
                    m_username = list.at(0).toLatin1();        //记录该socket的帐号
                    emit clientLoginSuccess(m_username, peerAddress().toString());
                }
                else writeClientData(QByteArray(), MT_CHECK, MO_NULL, CHECK_FAIL);

                break;
            }

            case MT_USERINFO:
            {
                qDebug() << "MT_USERINFO" << message.header.option;
                if (message.header.option == MO_DOWNLOAD)
                {
                    auto info = m_database->getUserInfo(m_username);
                    auto friends = m_database->getUserFriends(m_username);
                    toJsonStringAndSend(info, friends);
                }
                else if (message.header.option == MO_UPLOAD)
                {

                }
                break;
            }

            case MT_SHAKE:
                qDebug() << "发送给" << message.header.receiver << "的窗口震动";
                emit hasNewMessage(m_username, message.header.receiver, MT_SHAKE, message.header.option, str.toLocal8Bit());
                break;

            case MT_TEXT:
                qDebug() << "发送给" << message.header.receiver << "的消息:" << str;
                emit hasNewMessage(m_username, message.header.receiver, MT_TEXT, message.header.option, str.toLocal8Bit());
                break;

            case MT_IMAGE:
                break;

            case MT_HEADIMAGE:
                //m_database->get
                //writeClientData(MT_HEADIMAGE, );
                break;

            case MT_UNKNOW:

                break;
            }
        }
        gotSize = 0;
        m_data.clear();
    }
}
