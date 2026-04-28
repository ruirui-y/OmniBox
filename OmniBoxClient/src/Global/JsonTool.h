#ifndef JSONTOOL_H
#define JSONTOOL_H

#include <QObject>
#include "singletion.h"

class JsonTool  : public QObject, public Singleton<JsonTool>
{
	Q_OBJECT

	friend class Singleton<JsonTool>;

public:
	~JsonTool();

public:
	bool readJsonFile(const QString& path, QJsonDocument& outDoc, QString* err = nullptr);
	bool writeJsonFile(const QString& path, const QJsonDocument& inDoc, QString* err = nullptr);

	bool ConversionJsonToUserInfo(const QJsonObject& jsonObj);

	bool clearJsonFile(const QString& path);

private:
	JsonTool(QObject* parent = 0);
};

#endif // JSONTOOL_H