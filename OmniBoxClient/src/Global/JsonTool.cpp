#include "JsonTool.h"
#include <QFile>
#include <QSaveFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QStringList>
#include "UserMgr.h"


JsonTool::JsonTool(QObject *parent)
	: QObject(parent)
{}


JsonTool::~JsonTool()
{
}

bool JsonTool::readJsonFile(const QString & path, QJsonDocument & outDoc, QString * err)
{
    QFile f(path);
    if (!f.exists()) 
    {
        // 文件不存在：返回空对象，视需求也可以返回 false
        outDoc = QJsonDocument(QJsonObject{});
        return true;
    }

    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) 
    {
        if (err) *err = QStringLiteral("Open failed: %1").arg(f.errorString());
        return false;
    }

    QByteArray data = f.readAll();
    f.close();

    QJsonParseError pe;
    outDoc = QJsonDocument::fromJson(data, &pe);
    if (pe.error != QJsonParseError::NoError) 
    {
        if (err) 
            *err = QStringLiteral("Parse error at %1: %2").arg(pe.offset).arg(pe.errorString());
        return false;
    }
    return true;
}

bool JsonTool::writeJsonFile(const QString& path, const QJsonDocument& inDoc, QString* err)
{
    // 确保目录存在
    const QString dir = QFileInfo(path).dir().absolutePath();
    if (!dir.isEmpty()) 
        QDir().mkpath(dir);

    QSaveFile sf(path);                             // 原子写入，避免半写文件
    if (!sf.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (err) 
            *err = QStringLiteral("Open for write failed: %1").arg(sf.errorString());
        return false;
    }

    const QByteArray json = inDoc.toJson(QJsonDocument::Compact);
    if (sf.write(json) != json.size()) 
    {
        if (err) 
            *err = QStringLiteral("Write failed");
        sf.cancelWriting();
        return false;
    }

    if (!sf.commit()) 
    {
        if (err) 
            *err = QStringLiteral("Commit failed: %1").arg(sf.errorString());
        return false;
    }
    return true;
}

bool JsonTool::ConversionJsonToUserInfo(const QJsonObject& jsonObj)
{
    // 解析基本用户信息
    if (jsonObj.contains("UserName")) {
        UserMgr::Instance()->SetUserName(jsonObj["UserName"].toString());
    }

    if (jsonObj.contains("Password")) {
        UserMgr::Instance()->SetPassword(jsonObj["Password"].toString());
    }
    return true;
}

bool JsonTool::clearJsonFile(const QString& path)
{
    QFile f(path);
    if (!f.exists())
    {
        return true;
    }

    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return false;
    }

    QByteArray data = f.readAll();
    f.close();

    QJsonParseError pe;
    QJsonDocument doc = QJsonDocument::fromJson(data, &pe);
    if (pe.error != QJsonParseError::NoError)
    {
        return false;
    }

    QJsonObject obj = doc.object();
    obj["Account"] = QString("");
    obj["Password"] = QString("");
    obj["icon"] = QString("");

    QJsonDocument jsonDoc = QJsonDocument(obj);
    return writeJsonFile(path, jsonDoc, nullptr);
}