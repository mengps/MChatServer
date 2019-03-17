#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QMutex>
#include <QSqlDatabase>
#include <QDebug>

struct UserInfo
{
    QString username;
    QString password;
    QString nickname;
    QString headImage;
    QString gender;
    QString birthday;
    QString signature;
    int level;
};

struct FriendInfo
{
    QString friendname;
    int unreadMessage;  
};

QDebug operator<<(QDebug debug, const UserInfo &info);

class Database : public QObject
{
    Q_OBJECT

public:
    Database(const QString &connectionName, QObject *parent = nullptr);
    ~Database();

    bool openDatabase();
    void closeDatabase();

    bool createUser(const UserInfo &info);
    bool addFriend(const QString &username, const QString &friendname);

    UserInfo getUserInfo(const QString &username);
    QStringList getUserFriends(const QString &username);
    QMap<QString, QList<FriendInfo> > getUserFriendsInfo(const QString &username);

    //增加一条未读记录
    bool addUnreadMessage(const QString &sender, const QString &receiver);

private:
    bool tableExists();

private:
    QMutex m_mutex;
    QString m_connectionName;
    QSqlDatabase m_database;
};

#endif // DATABASE_H
