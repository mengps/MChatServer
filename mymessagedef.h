#ifndef MYMESSAGEDEF_H
#define MYMESSAGEDEF_H

#include <QtGlobal>

typedef quint32 MSG_FLAG_TYPE;
typedef quint8 MSG_TYPE;
typedef quint32 MSG_SIZE_TYPE;
typedef QByteArray MSG_MD5_TYPE;

//消息头的标志
#define MSG_FLAG 0xF8AD951A

//消息类型
#define MT_CHECK      0        //验证
#define MT_HEARTBEAT  1        //心跳
#define MT_SHAKE      2        //窗口抖动
#define MT_TEXT       3        //普通文本
//#define MT_IMAGE     4
#define MT_UNKNOW     5        //未知

//消息大小
#define MSG_MAXSIZE UINT_MAX

/*
 *  \ 消息标志flag \\ 消息类型type \\ 消息大小size \\ MD5验证 \   ...  \ 数据data \  ...  \ 数据data \
 *
*/
struct MessageHeader
{
    MSG_FLAG_TYPE   flag;
    MSG_SIZE_TYPE   size;
    MSG_TYPE        type;    
    MSG_MD5_TYPE    md5;
};

#endif // MYMESSAGEDEF_H
