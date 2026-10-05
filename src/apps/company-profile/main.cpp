#include <QCoreApplication>
#include <QTextStream>
#include <QJsonObject>
#include <QJsonDocument>
#include <QSqlQuery>
#include <QProcessEnvironment>
#include "core-auth/dbmanager.hpp"

void sendJsonResponse(const QJsonObject& json) {
    QTextStream out(stdout);
    out << "Content-Type: application/json\r\n\r\n";
    out << QJsonDocument(json).toJson(QJsonDocument::Compact);
}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    
    QString configPath = "/usr/lib/cgi-bin/azzammar/config.ini";
    if (!DBManager::instance().initialize(configPath)) {
        QJsonObject err;
        err["status"] = "error";
        err["message"] = "Database initialization failed.";
        sendJsonResponse(err);
        return 1;
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString requestMethod = env.value("REQUEST_METHOD");
    QJsonObject response;

    if (requestMethod == "POST") {
        // MENERIMA PAYLOAD UPDATE TEMA DARI FRONTEND REACT
        QTextStream in(stdin);
        QString rawPostData = in.readAll();
        QJsonDocument postDoc = QJsonDocument::fromJson(rawPostData.toUtf8());
        
        if (postDoc.isObject()) {
            QJsonObject inputData = postDoc.object();
            QString companyName = inputData["company_name"].toString();
            
            // Bungkus konfigurasi visual menjadi string JSON terkompresi
            QJsonObject themeObj;
            themeObj["theme"] = inputData["theme"].toString();
            themeObj["primary_color"] = inputData["primary_color"].toString();
            QString themeConfigStr = QJsonDocument(themeObj).toJson(QJsonDocument::Compact);

            QSqlQuery query;
            query.prepare("UPDATE company_profiles SET company_name = :name, theme_config = :config WHERE is_default = 1");
            query.bindValue(":name", companyName);
            query.bindValue(":config", themeConfigStr);

            if (query.exec()) {
                response["status"] = "success";
                response["message"] = "Company Profile theme updated successfully.";
            } else {
                response["status"] = "error";
                response["message"] = "Failed to update SQLite record rows.";
            }
        } else {
            response["status"] = "error";
            response["message"] = "Invalid JSON structure payload.";
        }
        sendJsonResponse(response);
    } else {
        // METHOD GET: (Logika penarikan data yang sudah ada sebelumnya)
        QString queryString = env.value("QUERY_STRING");
        QString targetSlug = "";
        if (queryString.contains("slug=")) {
            targetSlug = queryString.split("slug=").last().split("&").first();
        }

        QSqlQuery query;
        if (targetSlug.isEmpty()) {
            query.prepare("SELECT company_name, theme_config, profile_slug FROM company_profiles WHERE is_default = 1 LIMIT 1");
        } else {
            query.prepare("SELECT company_name, theme_config, profile_slug FROM company_profiles WHERE profile_slug = :slug");
            query.bindValue(":slug", targetSlug);
        }

        if (query.exec() && query.next()) {
            response["status"] = "success";
            response["company_name"] = query.value("company_name").toString();
            response["profile_slug"] = query.value("profile_slug").toString();
            QString rawConfig = query.value("theme_config").toString();
            response["config"] = QJsonDocument::fromJson(rawConfig.toUtf8()).object();
        } else {
            response["status"] = "fallback";
            response["company_name"] = "Azzammar Cloud Fallback";
        }
        sendJsonResponse(response);
    }

    return 0;
}

