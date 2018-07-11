#ifndef MYMESSAGEDEF_H
#define MYMESSAGEDEF_H

#include <QtGlobal>
#include <QDataStream>

typedef quint32 MSG_FLAG_TYPE;
typedef quint8 MSG_TYPE;
typedef quint32 MSG_SIZE_TYPE;
typedef quint8 MSG_OPTION_TYPE;
typedef QByteArray MSG_ID_TYPE;
typedef QByteArray MSG_MD5_TYPE;

//消息头的标志
#define MSG_FLAG 0xF8AD951A

//消息类型
//系统类型  0x00 ~ 0x30
#define MSG_IS_SYSTEM(x) (x <= 0x30)
#define MT_CHECK      0x10        //验证
#define CHECK_SUCCESS "SUCCESS"   //验证成功
#define CHECK_FAIL    "FAILURE"   //验证失败
#define MT_HEARTBEAT  0x11        //心跳
#define MT_USERINFO   0x12        //用户信息
#define MT_UNKNOW     0x20        //未知

//用户类型 0x31 ~ 0xFF
#define MSG_IS_USER(x) (!(MSG_IS_SYSTEM(x)))
#define MT_SHAKE      0x31        //窗口抖动
#define MT_TEXT       0x32        //普通文本
#define MT_IMAGE      0x33        //图像
#define MT_HEADIMAGE  0x34        //头像


//选项类型
#define MO_NULL       0x10      //无类型
#define MO_UPLOAD     0x11      //上传
#define MO_DOWNLOAD   0x12      //下载

//消息大小
#define MSG_MAXSIZE UINT_MAX

/*
 *  \ 消息标志flag \\ 消息类型type \\ 消息大小size \\ 选项类型option \\ 发送者ID \\ 接收者ID \\ MD5验证 \
 *                                           { 消息头 }
 *                                 \ 数据data \ ... \ 数据data \
 *                                           { 数据 }
*/

struct MessageHeader
{
    MSG_FLAG_TYPE   flag;
    MSG_TYPE        type;
    MSG_SIZE_TYPE   size;
    MSG_OPTION_TYPE option;
    MSG_ID_TYPE     sender;
    MSG_ID_TYPE     receiver;
    MSG_MD5_TYPE    md5;
};

struct Message
{
    Message() {}
    Message(MessageHeader h, QByteArray d) : header(h), data(d) {}

    MessageHeader header;
    QByteArray data;
};

QDataStream& operator<<(QDataStream &out, const MessageHeader &header);
QDataStream& operator>>(QDataStream &in, MessageHeader &header);
QDataStream& operator<<(QDataStream &out, const Message &message);
QDataStream& operator>>(QDataStream &in, Message &message);


#endif // MYMESSAGEDEF_H
