#ifndef JSONTOOL_H
#define JSONTOOL_H

#include <QObject>
#include "singletion.h"
#include "Struct.h"

class JsonTool  : public QObject, public Singleton<JsonTool>
{
	Q_OBJECT

	friend class Singleton<JsonTool>;

public:
	~JsonTool();

public:
	bool readJsonFile(const QString& path, QJsonDocument& outDoc, QString* err = nullptr);
	bool writeJsonFile(const QString& path, const QJsonDocument& inDoc, QString* err = nullptr);

	bool ConversionJsonToUserInfo(const QJsonObject& jsonObj, UserInfo& outInfo);				// 转换json到UserInfo

	bool clearJsonFile(const QString& path);

private:
	bool ParseGameObject(const QJsonObject& obj, GameData& outGame, QString* warn = nullptr);   // 解析单个游戏

private:
	JsonTool(QObject* parent = 0);
};

#endif // JSONTOOL_H