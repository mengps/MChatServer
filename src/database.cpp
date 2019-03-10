#include "database.h"
#include <QDir>
#include <QSqlQuery>
#include <QSqlError>

extern QDebug operator<<(QDebug debug, const UserInfo &info)
{
    QDebugStateSaver saver(debug);
    debug << "username: " << info.username  << "password: " << info.password  << "nickname: " << info.nickname
          << "headImage: " << info.headImage << "gende: " << info.gender  << "birthday: " << info.birthday
          << "signature: " << info.signature << "unreadMessage: " << info.unreadMessage  << "level: " << info.level;
    return debug;
}

Database::Database(const QString &connectionName, QObject *parent)
    :   QObject(parent), m_connectionName(connectionName)
{
    m_database = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    m_database.setDatabaseName("users/users.db");
    m_database.setHostName("localhost");
    m_database.setUserName("MChatServer");
    m_database.setPassword("123456");
}

Database::~Database()
{
    m_database.close();
    //QSqlDatabase::removeDatabase(m_connectionName);
}

bool Database::openDatabase()
{
    QMutexLocker locker(&m_mutex);
    if (!m_database.isOpen())
        return m_database.open();

    return true;
}

void Database::closeDatabase()
{
    QMutexLocker locker(&m_mutex);
    if (m_database.isOpen())
        m_database.close();
}

bool Database::createUser(const UserInfo &info)
{
    if (!tableExists())
        return false;

    QString user_dir = "users/" + info.username;
    if (!QFileInfo::exists(user_dir))               //如果该用户不存在,则创建一系列文件夹
    {
        QDir dir;
        dir.mkdir(user_dir);
        dir.mkdir(user_dir + "/headImage");
        dir.mkdir(user_dir + "/messageImage");
        dir.mkdir(user_dir + "/messageText");

        openDatabase();
        QString insert = "INSERT INTO info VALUES(?, ?, ?, ?, ?, ?, ?, ?);";
        QSqlQuery query(m_database);
        query.prepare(insert);
        query.addBindValue(info.username);
        query.addBindValue(info.password);
        query.addBindValue(info.nickname);
        query.addBindValue(info.headImage);
        query.addBindValue(info.gender);
        query.addBindValue(info.birthday);
        query.addBindValue(info.signature);
        query.addBindValue(info.unreadMessage);
        query.addBindValue(info.level);

        if (query.exec())
        {
            closeDatabase();
            return true;
        }
        else
        {
            qDebug() << __func__ << query.lastError().text();;
            closeDatabase();
            return false;
        }
    }

    return true;
}

UserInfo Database::getUserInfo(const QString &username)
{
    if (!tableExists())
        return UserInfo();

    openDatabase();
    QSqlQuery query(m_database);
    if (!query.exec("SELECT * FROM info WHERE user_username = '" + username + "';"))
         qDebug() << __func__ <<  query.lastError().text();
    query.next();
    UserInfo info;
    info.username = query.value(0).toString();
    info.password = query.value(1).toString();
    info.nickname = query.value(2).toString();
    info.headImage = query.value(3).toString();
    info.gender = query.value(4).toString();
    info.birthday = query.value(5).toString();
    info.signature = query.value(6).toString();
    info.unreadMessage = query.value(7).toInt();
    info.level = query.value(8).toInt();
    closeDatabase();

    return info;
}

bool Database::addFriend(const QString &username, const QString &friendname)
{

}

QMap<QString, QStringList> Database::getUserFriends(const QString &username)
{
    if (!tableExists())
        return QMap<QString, QStringList>();

    openDatabase();
    QString query_friends = "SELECT user_group AS user_group, user_friend AS user_friend "
                            "FROM users "
                            "WHERE user_username = '" + username + "' "
                            "UNION ALL "
                            "SELECT friend_group AS user_group, user_username AS user_friend "
                            "FROM users "
                            "WHERE user_friend = '" + username + "'; ";
    QSqlQuery query(m_database);
    QMap<QString, QStringList> friends;
    if (query.exec(query_friends))
    {
        while (query.next())
        {
            QString user_group = query.value(0).toString();
            QString user_friend = query.value(1).toString();
            friends[user_group].append(user_friend);
        }
    }
    else qDebug() << __func__ <<  query.lastError().text();
    closeDatabase();

    return friends;
}

bool  Database::addUnreadMessage(const QString &username)
{
    if (!openDatabase())
        return false;

    QString query_update = "UPDATE info "
                           "SET user_unreadMessage = (SELECT user_unreadMessage "
                           "                          FROM info "
                           "                          WHERE user_username = '" + username + "') + 1 "
                           "WHERE user_username = '" + username + "';";
    QSqlQuery query(m_database);
    if (query.exec(query_update))
    {
        closeDatabase();
        return true;
    }
    else
    {
        qDebug() << __func__ << query.lastError().text();
        closeDatabase();
        return false;
    }
}

bool Database::tableExists()
{
    if (!openDatabase())
        return false;

    QString query_create_users = "CREATE TABLE IF NOT EXISTS users"
                                 "("
                                 "    user_username varchar(10) NOT NULL,"
                                 "    user_friend   varchar(10) NOT NULL,"
                                 "    user_group    varchar(16) DEFAULT '我的好友',"
                                 "    friend_group  varchar(16) DEFAULT '我的好友'"
                                 ");";
    QString query_create_info = "CREATE TABLE IF NOT EXISTS info"
                                "("
                                "    user_username varchar(10) NOT NULL PRIMARY KEY,"
                                "    user_password varchar(32) NOT NULL,"
                                "    user_nickname varchar(32) DEFAULT 'USER'"
                                ");";
    QSqlQuery query(m_database);
    if (query.exec(query_create_users))
    {
        if (query.exec(query_create_info))
        {
            closeDatabase();
            return true;
        }
        else
        {
            qDebug() << __func__ << query.lastError().text();
            closeDatabase();
            return false;
        }
    }
    else
    {
        qDebug() << __func__ << query.lastError().text();
        closeDatabase();
        return false;
    }
}
