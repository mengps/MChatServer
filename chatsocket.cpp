#include <QTimer>
#include <QThread>
#include <QDataStream>
#include <QHostAddress>
#include <QCryptographicHash>
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
    connect(this, &ChatSocket::bytesWritten, this, &ChatSocket::continueWrite);
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
    qDebug() << sentBytes << m_fileBytes << QString::fromLocal8Bit(m_data);

    if (sentBytes >= m_fileBytes)
    {
        m_fileBytes = 0;
        sentBytes = 0;
        m_data.clear();
        return;
    }

    QThread::msleep(10);
    write(m_data);
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
    emit clientDisconnected(peerAddress().toString());
    emit consoleMessage(peerAddress().toString() + " 断开连接..");
    deleteLater();
}

void ChatSocket::writeClientData(MSG_TYPE type, QByteArray data) //write to (QString &client)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_9);

    MSG_FLAG_TYPE flag = MSG_FLAG;
    MSG_SIZE_TYPE size = data.size();
    MSG_MD5_TYPE md5 = QByteArray();

    if (type == MT_TEXT || type == MT_CHECK)
    {
        md5 = QCryptographicHash::hash(data, QCryptographicHash::Md5);
        m_data = data;
        m_fileBytes = size + sizeof(flag) + sizeof(type) + sizeof(size) + md5.size() + sizeof(QByteArray);
    }

    out << flag << type << size << md5;
    write(block);
}

void ChatSocket::readClientData()
{
    static int got_size = 0;
    static MSG_TYPE type = MT_UNKNOW;
    static MSG_MD5_TYPE md5;

    if (m_data.size() == 0)  //必定为消息头
    {        
        QDataStream in(this);
        in.setVersion(QDataStream::Qt_5_9);

        MSG_FLAG_TYPE flag;
        in >> flag;
        if (flag != MSG_FLAG)
            return;

        in >> type;
        if (type == MT_HEARTBEAT)           //心跳检测，直接返回
            return;

        MSG_SIZE_TYPE size;
        in >> size >> md5;
        m_data.resize(size);
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
            QString str = QString::fromLocal8Bit(m_data.data());
            emit consoleMessage(QString("md5 一致，消息为：\"" + str + "\"，大小：" + QString::number(m_data.size())));
            switch (type)
            {
            case MT_CHECK:
            {
                QString login = QString::fromLocal8Bit(QByteArray::fromBase64(m_data));
                QStringList list = login.split("%%");
                qDebug() << "登录信息：" << list;
                if (list.at(0) == "843261040" && list.at(1) == "00000000000")
                    writeClientData(MT_CHECK, QString("1").toLocal8Bit());
                else writeClientData(MT_CHECK, QString("0").toLocal8Bit());
                break;
            }

            case MT_SHAKE:

                break;

            case MT_TEXT:

                break;

            case MT_UNKNOW:

                break;
            }
        }

        got_size = 0;           //重新开始
        type = MT_UNKNOW;
        md5.clear();
    }
}
