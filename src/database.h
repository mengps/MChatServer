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
    QString background;
    QString gender;
    QString birthday;
    QString signature;
    int level;

    UserInfo()
        : username(""), password(""), nickname("") , headImage("")
        , background(""), gender(""), birthday(""), signature(""), level(0)
    {

    }

    bool isEmpty() const { return username.isEmpty(); }
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

    QString name() const { return m_database.connectionName(); }
    bool openDatabase();
    void closeDatabase();

    bool createUser(const UserInfo &info);
    bool addFriend(const QString &username, const QString &friendname);

    void setUserInfo(const UserInfo &info);
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
