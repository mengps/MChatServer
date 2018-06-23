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

    qDebug() << sentBytes << m_fileBytes;
    if (sentBytes >= m_fileBytes)
    {
        m_fileBytes = 0;
        sentBytes = 0;
        m_data.clear();
        return;
    }

    write(m_data);
    QThread::msleep(10);
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

void ChatSocket::toJsonStringAndSend(const UserInfo &info, const QMap<QString, QStringList> &friends)
{
    QJsonObject object;
    object.insert("Username", info.username);
    object.insert("Nickname", info.nickname);
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
    writeClientData(m_username, MT_USERINFO, QJsonDocument(object).toJson());

    qDebug() << object;
}

void ChatSocket::writeClientData(const QByteArray &sender, MSG_TYPE type, QByteArray data)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_9);

    MSG_FLAG_TYPE flag = MSG_FLAG;
    MSG_MD5_TYPE md5 = QCryptographicHash::hash(QByteArray(), QCryptographicHash::Md5);

    m_data = data.toBase64();
    md5 = QCryptographicHash::hash(m_data, QCryptographicHash::Md5);
    m_fileBytes = m_data.size() + sizeof(flag) + sizeof(type) + sizeof(MSG_SIZE_TYPE) +
            sizeof (QByteArray) + sender.size() + sizeof(QByteArray) + m_username.size() +
            sizeof(QByteArray) + md5.size();

    out << flag << type << m_data.size() << sender << m_username << md5;
    write(block);
    QThread::msleep(10);
}

void ChatSocket::readClientData()
{
    static int got_size = 0;
    static MSG_TYPE type = MT_UNKNOW;
    static MSG_ID_TYPE sender = MSG_ID_TYPE();
    static MSG_ID_TYPE receiver = MSG_ID_TYPE();
    static MSG_MD5_TYPE md5;

    if (m_data.size() == 0)  //必定为消息头
    {        
        QDataStream in(this);
        in.setVersion(QDataStream::Qt_5_9);

        MSG_FLAG_TYPE flag;
        in >> flag;
        if (flag != MSG_FLAG)
        {
            readAll();
            return;
        }

        in >> type;
        if (type == MT_HEARTBEAT)           //心跳检测，直接返回
        {
            readAll();
            return;
        }

        MSG_SIZE_TYPE size;
        in >> size >> sender >> receiver >> md5;
        m_data.resize(size);
        qDebug() << "header type" << type;
    }
    else                                //合并数据
    {
        QByteArray data = read(bytesAvailable());
        m_data.replace(got_size, data.size(), data);
        got_size += data.size();
    }

    if (got_size == m_data.size())     //接收完毕
    {
        QByteArray md5_t = QCryptographicHash::hash(m_data, QCryptographicHash::Md5);
        if (md5 == md5_t)   //正确的消息
        {
            QString str = QString::fromLocal8Bit(QByteArray::fromBase64(m_data));
            m_data.clear();

            switch (type)
            {
            case MT_CHECK:
            {
                QStringList list = str.split("%%");
                qDebug() << "登录信息：" << list;
                m_database = new Database(QString::number((int)QThread::currentThreadId(), 16), this);
                UserInfo info = m_database->getUserInfo(list.at(0).toLocal8Bit());
                if (info.password == list.at(1).toLocal8Bit())
                {
                    writeClientData(QByteArray(), MT_CHECK, QString("1").toLocal8Bit());
                    m_username = list.at(0).toLocal8Bit();        //记录该socket的帐号
                    emit clientLoginSuccess(m_username, peerAddress().toString());
                }
                else writeClientData(QByteArray(), MT_CHECK, QString("0").toLocal8Bit());

                break;
            }

            case MT_USERINFO:
            {
                qDebug() << "MT_USERINFO";
                if (str == "1")
                {
                    auto info = m_database->getUserInfo(m_username);
                    auto friends = m_database->getUserFriends(m_username);
                    toJsonStringAndSend(info, friends);
                }
                break;
            }

            case MT_SHAKE:
                qDebug() << "发送给" << receiver << "的窗口震动:" << str;
                emit hasNewMessage(m_username, receiver, MT_SHAKE, QByteArray());
                break;

            case MT_TEXT:
                qDebug() << "发送给" << receiver << "的消息:" << str;
                emit hasNewMessage(m_username, receiver, MT_TEXT, str.toLocal8Bit());
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

            got_size = 0;           //重新开始
            type = MT_UNKNOW;
            sender.clear();
            receiver.clear();
            md5.clear();
        }
    }
}
