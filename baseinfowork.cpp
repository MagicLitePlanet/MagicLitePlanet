#include "baseinfowork.h"

baseInfoWork::baseInfoWork(QObject *parent) : QObject(parent)
{

}

void baseInfoWork::loadBaseInfoWorking()
{
    curDateTime = QDateTime::currentDateTime();
    this->uid = uid;
    QSqlQuery query(DB);
    QThread::msleep(30);    //等待GUI相应的时间
    query.exec("SELECT name, gender, telephone, mail, user_group, user_dpt, user_avatar FROM magic_users WHERE uid = " + uid);
    if(query.next())
    {
        name = query.value("name").toString();
        gender = query.value("gender").toString();
        telephone = query.value("telephone").toString();
        mail = query.value("mail").toString();
        avatarUrl = query.value("user_avatar").toString();
    }
    group = loadGroup(uid);
    department = loadDepartment(uid);
    avatar = loadAvatar(avatarUrl);

    query.exec("SELECT * FROM magic_attendance WHERE a_uid='" + uid + "' AND today='" + curDateTime.date().toString("yyyy-MM-dd") + "';");
    if(query.next())
    {
        isAttend = true;
        attendBeginTime = query.value("begin_date").toString();
        attendEndTime = query.value("end_date").toString();
    }
    else
        isAttend = false;
    query.clear();
    emit baseInfoFinished();
    qDebug() << "baseInfoLoad线程：" << this->thread();
}

void baseInfoWork::refreshBaseInfo()
{
    loadBaseInfoWorking();
}

void baseInfoWork::setUid(QString uid)
{
    this->uid = uid;
}

void baseInfoWork::setDB(const QSqlDatabase &DB)
{
    this->DB = DB;
}

bool baseInfoWork::getAttendToday()
{
    return isAttend;
}

QString baseInfoWork::getBeginTime()
{
    return attendBeginTime;
}

QString baseInfoWork::getEndTime()
{
    return attendEndTime;
}

QString baseInfoWork::getLoginUid()
{
    return loginUid;
}

QString baseInfoWork::getLastSignupUid()
{
    return lastSignupUid;
}

QString baseInfoWork::loadGroup(const QString& uid)
{
    QSqlQuery query(DB);
    query.exec("SELECT user_group FROM magic_users WHERE uid = " + uid);
    if(!query.next())
        return "--";
    query.exec("SELECT group_name FROM magic_group WHERE group_id = " + query.value("user_group").toString());
    if(!query.next())
        return "--";
    return query.value("group_name").toString();
}

QString baseInfoWork::loadDepartment(const QString& uid)
{
    QSqlQuery query(DB);
    query.exec("SELECT user_dpt FROM magic_users WHERE uid = " + uid);
    if(!query.next())
        return "--";
    query.exec("SELECT dpt_name FROM magic_department WHERE dpt_id = " + query.value("user_dpt").toString());
    if(!query.next())
        return "--";
    return query.value("dpt_name").toString();
}

void baseInfoWork::autoAuthAccount(const long long account, const QString &pwd)
{
    if(service::authAccount(DB, loginUid, account, pwd))
        emit autoAuthRes(true);
    else
        emit autoAuthRes(false);
}

void baseInfoWork::signUp(const QString& pwd, const QString& name, const QString& tel)
{
    QSqlQuery query(DB);
    QString creatQueryStr;
    creatQueryStr =
            "INSERT INTO magic_users"
            "(password, name, user_group, user_dpt, telephone )"
            "VALUES                        "
            "(:pwd, :name, 2, 1, :phone) ";
    query.prepare(creatQueryStr);
    query.bindValue(0, service::pwdEncrypt(pwd));
    query.bindValue(1, name);
    query.bindValue(2, tel);
    emit signupRes(query.exec());
    lastSignupUid = query.lastInsertId().toString();
}

void baseInfoWork::editPersonalInfo(const QString& oldPwd, const QString &tel, const QString &mail, const QString &avatar, const QString &pwd)
{
    QSqlQuery query(DB);
    if(service::authAccount(DB, uid, uid.toLongLong(), service::pwdEncrypt(oldPwd)))
    {
        if(!tel.isEmpty())
            query.exec("UPDATE magic_users SET telephone='" + tel +"' WHERE uid='" + uid +"';");
        if(!mail.isEmpty())
            query.exec("UPDATE magic_users SET mail='" + mail +"' WHERE uid='" + uid +"';");
        if(!avatar.isEmpty())
            query.exec("UPDATE magic_users SET user_avatar='" + avatar +"' WHERE uid='" + uid +"';");
        if(!pwd.isEmpty())
        {
            query.exec("UPDATE magic_users SET password='" + service::pwdEncrypt(pwd) +"' WHERE uid='" + uid +"';");
            emit editPersonalInfoRes(2);
        }
        else emit editPersonalInfoRes(1);
    }
    else
        emit editPersonalInfoRes(-1);
}

void baseInfoWork::authAccount(const long long account, const QString &pwd, const QString& editPwd)
{
    if(service::authAccount(DB, loginUid, account, pwd) || service::authAccount(DB, loginUid, account, editPwd))
        emit authRes(true);
    else
        emit authRes(false);
}

void baseInfoWork::setAuthority(const QString &uid, const QVector<QAction *> &vector)
{
    emit authorityRes(service::setAuthority(DB, uid, vector));
}

QString baseInfoWork::getName()
{
    return name;
}

QString baseInfoWork::getGender()
{
    return gender;
}

QString baseInfoWork::getTel()
{
    return telephone;
}

QString baseInfoWork::getMail()
{
    return mail;
}

QString baseInfoWork::getGroup()
{
    return group;
}

QString baseInfoWork::getDepartment()
{
    return department;
}

QPixmap baseInfoWork::getAvatar()
{
    return avatar;
}

QPixmap baseInfoWork::loadAvatar(const QString &url)
{
    QUrl picUrl(url);
    QNetworkAccessManager manager;
    QEventLoop loop;
    QNetworkReply *reply = manager.get(QNetworkRequest(picUrl));
    //请求结束并下载完成后，退出子事件循环
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    //开启子事件循环
    loop.exec();
    QByteArray jpegData = reply->readAll();
    QPixmap pixmap;
    pixmap.loadFromData(jpegData);
    return pixmap;
}
