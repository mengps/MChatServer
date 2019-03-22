#include "chatsocket.h"

#include <QCryptographicHash>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QThread>

ChatSocket::ChatSocket(qintptr socketDescriptor, QObject *parent)
    : QTcpSocket(parent)
    , m_status(Offline)
{
    if (!setSocketDescriptor(socketDescriptor))
    {
        emit logMessage(errorString());
        deleteLater();
    }

    m_heartbeat = new QTimer(this);
    m_heartbeat->setInterval(60000);
    m_sendDataBytes = 0;
    m_sendData = QByteArray();
    m_recvData = QByteArray();
    m_lastTime = QDateTime::currentDateTime();

    connect(this, &ChatSocket::readyRead, this, &ChatSocket::heartbeat);
    connect(this, &ChatSocket::bytesWritten, this, &ChatSocket::continueWrite, Qt::DirectConnection);
    connect(this, &ChatSocket::readyRead, this, [this]()
    {
        m_recvData += readAll();
        processRecvMessage();
    });
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

    if (sentBytes >= m_sendDataBytes)
    {
        m_sendDataBytes = 0;
        sentBytes = 0;
        m_sendData.clear();
        m_hasMessageProcessing = false;
        if (!m_messageQueue.isEmpty())
            processNextSendMessage();       //如果消息队列不为空，则继续处理下一条待发送消息
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
    emit logMessage(peerAddress().toString() + " 断开连接..");
    deleteLater();
}

void ChatSocket::processNextSendMessage()
{
    if (!m_hasMessageProcessing && !m_messageQueue.isEmpty())
    {
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_9);
        Message *message = m_messageQueue.dequeue();
        out << *message;
        m_sendData = block;
        m_sendDataBytes = block.size();
        m_hasMessageProcessing = true;
        delete message;

        write(block);
        flush();        //立即发送消息
    }
}

void ChatSocket::toJsonAndSend(const UserInfo &info, const QMap<QString, QList<FriendInfo> > &friends)
{
    QJsonObject object;
    object.insert("Username", info.username);
    object.insert("Nickname", info.nickname);
    object.insert("HeadImage", info.headImage);
    object.insert("Gender", info.gender);
    object.insert("Birthday", info.birthday);
    object.insert("Signature", info.signature);
    object.insert("UnreadMessage", 0);
    object.insert("Level", info.level);
    QJsonArray friendList;
    for (auto it = friends.constBegin(); it != friends.constEnd(); it++)
    {
        QJsonArray array;
        for (auto it2 : it.value())
        {
            UserInfo info2 = m_database->getUserInfo(it2.friendname);
            QJsonObject object2;
            object2.insert("Username", info2.username);
            object2.insert("Nickname", info2.nickname);
            object2.insert("HeadImage", info2.headImage);
            object2.insert("Gender", info2.gender);
            object2.insert("Birthday", info2.birthday);
            object2.insert("Signature", info2.signature);
            object2.insert("UnreadMessage", it2.unreadMessage);
            object2.insert("Level", info2.level);
            array.append(object2);
        }
        QJsonObject obj;
        obj.insert("Group", it.key());
        obj.insert("Friend", array);
        friendList.append(obj);
    }
    object.insert("FriendList", friendList);
    writeClientData(SERVER_ID, MT_USERINFO, MO_NULL, QJsonDocument(object).toJson());
    qDebug() << object;
}

bool ChatSocket::updateInfomation(const QByteArray &infoJson)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(infoJson, &error);
    if (!doc.isNull() && (error.error == QJsonParseError::NoError))
    {
        if (doc.isObject())
        {
            UserInfo info;
            QJsonObject object = doc.object();
            QJsonValue value = object.value("Username");
            if (value.isString())
            {
                if (m_username != value.toString())
                    return false;
                info.username = value.toString();
            }
            value = object.value("Password");
            if (value.isString())
                info.password = value.toString();
            value = object.value("Nickname");
            if (value.isString())
                info.nickname = value.toString();
            value = object.value("Gender");
            if (value.isString())
                info.gender = value.toString();
            /*value = object.value("Background");
            if (value.isString())
                info.background = value.toString();*/
            value = object.value("HeadImage");
            if (value.isString())
                info.headImage = value.toString();
            value = object.value("Signature");
            if (value.isString())
                info.signature = value.toString();
            value = object.value("Birthday");
            if (value.isString())
                info.birthday = value.toString();
            value = object.value("Level");
            if (value.isDouble())
                info.level = value.toInt();

            qDebug() << info;
            m_database->setUserInfo(info);
        }
    }
    else
    {
        qDebug() << __func__ << "更新不成功：" << error.errorString();
        return false;
    }

    return true;
}

QByteArray ChatSocket::infoToJson(const UserInfo &info)
{
    QJsonObject object;
    object.insert("Username", info.username);
    object.insert("Nickname", info.nickname);
    object.insert("HeadImage", info.headImage);
    object.insert("Gender", info.gender);
    object.insert("Birthday", info.birthday);
    object.insert("Signature", info.signature);
    object.insert("Level", info.level);

    return QJsonDocument(object).toJson();
}

void ChatSocket::writeClientData(const QByteArray &sender, msg_t type, msg_option_t option, const QByteArray &data)
{
    QByteArray base64 = data.toBase64();
    QByteArray md5 = QCryptographicHash::hash(base64, QCryptographicHash::Md5);

    MessageHeader header = { MSG_FLAG, type, msg_size_t(base64.size()), option, sender, m_username, md5 };
    Message *message = new Message(header, base64);
    m_messageQueue.enqueue(message);
    processNextSendMessage();
}

void ChatSocket::processRecvMessage()
{
    //尝试读取一个完整的消息头
    if (m_recvHeader.isEmpty() && m_recvData.size() > 0)
    {
        MessageHeader header;
        QDataStream in(&m_recvData, QIODevice::ReadOnly);
        in.setVersion(QDataStream::Qt_5_9);
        in >> header;

        if (header.isEmpty()) return;
        qDebug() << header;
        m_recvHeader = header;
        m_recvData.remove(0, header.getSize() + 4); //data为QByteArray，前面有4字节的大小

        //如果成功读取了一个完整的消息头，但flag不一致(即：不是我的消息)
       if (get_flag(m_recvHeader) != MSG_FLAG)
       {
           m_recvHeader = MessageHeader();
           return;
       }
    }

    //如果数据大小不足一条消息
    int size = int(get_size(m_recvHeader));
    if (m_recvData.size() < size)
        return;

    auto rawData = m_recvData.left(size);
    m_recvData = m_recvData.mid(size);

    auto md5 = QCryptographicHash::hash(rawData, QCryptographicHash::Md5);
    auto data = QByteArray::fromBase64(rawData);
    if (md5 != get_md5(m_recvHeader)) return;

    qDebug() << "md5 一致，消息为：" + data;
    qDebug() << "消息大小：" + QString::number(data.size());

    switch (get_type(m_recvHeader))
    {
    case MT_HEARTBEAT:
        break;
    case MT_CHECK:
    {
        QString str = QString::fromLocal8Bit(data);
        QStringList list = str.split("%%");
        qDebug() << "登录信息：" << list;
        m_username = list.at(0).toLatin1();        //记录该socket的帐号
        m_database = new Database(m_username + QString::number(qintptr(QThread::currentThreadId()), 16), this);
        UserInfo info = m_database->getUserInfo(list.at(0));
        if (info.password == list.at(1))
        {
            writeClientData(SERVER_ID, MT_CHECK, MO_NULL, CHECK_SUCCESS);
            emit clientLoginSuccess(m_username, peerAddress().toString());
        }
        else
        {
            writeClientData(SERVER_ID, MT_CHECK, MO_NULL, CHECK_FAILURE);
        }
        break;
    }
    case MT_USERINFO:
    {
        qDebug() << "MT_USERINFO" << get_option(m_recvHeader);
        if (get_option(m_recvHeader) == MO_DOWNLOAD)
        {
            auto info = m_database->getUserInfo(m_username);
            auto friendsInfo = m_database->getUserFriendsInfo(m_username);
            toJsonAndSend(info, friendsInfo);
        }
        else if (get_option(m_recvHeader) == MO_UPLOAD)
        {
            updateInfomation(data);
        }
        break;
    }
    case MT_STATECHANGE:
    {
        m_status = ChatStatus(data.toInt());
        emit hasNewMessage(m_username, get_receiver(m_recvHeader), MT_STATECHANGE, MO_NULL, data);
        break;
    }
    case MT_SEARCH:
    {
        QString username = QString::fromLocal8Bit(data);
        qDebug() << "获取" << username << "的信息";
        UserInfo info = m_database->getUserInfo(username);

        QByteArray sendData = infoToJson(info);
        writeClientData(SERVER_ID, MT_SEARCH, MO_NULL, sendData);
        break;
    }
    case MT_SHAKE:
    {
        QString str = QString::fromLocal8Bit(data);
        qDebug() << "发送给" << get_receiver(m_recvHeader) << "的窗口震动";
        emit hasNewMessage(m_username, get_receiver(m_recvHeader), MT_SHAKE, get_option(m_recvHeader), data);
        break;
    }
    case MT_TEXT:
    {
        QString str = QString::fromLocal8Bit(data);
        qDebug() << "发送给" << get_receiver(m_recvHeader) << "的消息:" << str;
        emit hasNewMessage(m_username, get_receiver(m_recvHeader), MT_TEXT, get_option(m_recvHeader), data);
        break;
    }

    case MT_IMAGE:
        break;

    case MT_FILE:
        break;

    case MT_ADDFRIEND:
    {
        qDebug() << "发送给" << get_receiver(m_recvHeader) << "的添加好友请求。";
        emit hasNewMessage(m_username, get_receiver(m_recvHeader),
                           MT_ADDFRIEND, get_option(m_recvHeader), data);
        break;
    }

    case MT_UNKNOW:
        break;

    default:
        break;
    }
    //处理结束，清空消息头
    m_recvHeader = MessageHeader();
}
