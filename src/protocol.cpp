#include "protocol.h"
#include <QDebug>

extern QDebug operator<<(QDebug debug, const MessageHeader &header)
{
    debug << hex << header.flag
          << header.type
          << header.size
          << header.option
          << header.sender
          << header.receiver
          << header.md5;

    return debug;
}

extern QDebug operator<<(QDebug debug, const Message &message)
{
    debug << message.header << message.data;

    return debug;
}

extern QDataStream& operator<<(QDataStream &out, const MessageHeader &header)
{
    out << header.flag
        << header.type
        << header.size
        << header.option
        << header.sender
        << header.receiver
        << header.md5;

    return out;
}

extern QDataStream& operator>>(QDataStream &in, MessageHeader &header)
{
    in >> header.flag
       >> header.type
       >> header.size
       >> header.option
       >> header.sender
       >> header.receiver
       >> header.md5;

    return in;
}

extern QDataStream& operator<<(QDataStream &out, const Message &message)
{
    out << message.header
        << message.data;

    return out;
}

extern QDataStream& operator>>(QDataStream &in, Message &message)
{
    in >> message.header
       >> message.data;

    return in;
}
