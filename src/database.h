#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QMutex>
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
    int unreadMessage;
    int level;
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
    UserInfo getUserInfo(const QString &username);

    bool addFriend(const QString &username, const QString &friendname);
    QMap<QString, QStringList> getUserFriends(const QString &username);

    bool addUnreadMessage(const QString &username);

private:
    bool tableExists();

private:
    QMutex m_mutex;
    QString m_connectionName;
    QSqlDatabase m_database;
};

#endif // DATABASE_H
