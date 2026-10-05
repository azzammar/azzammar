#include <QCoreApplication>
#include <QTextStream>
#include <QJsonObject>
#include <QJsonDocument>
#include <QSqlQuery>
#include <QFile>
#include "core-auth/dbmanager.hpp"

void sendResponse(const QJsonObject& json) {
    QTextStream out(stdout);
    out << "Content-Type: application/json\r\n\r\n";
    out << QJsonDocument(json).toJson(QJsonDocument::Compact);
}

bool runInitSql(const QString& sqlPath) {
    QFile file(sqlPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream in(&file);
    QString sqlContent = in.readAll();
    file.close();

    // Pisahkan query berdasarkan titik koma (;) untuk dieksekusi satu per satu
    QStringList queries = sqlContent.split(";", Qt::SkipEmptyParts);
    for (const QString& queryStr : queries) {
        QString trimmed = queryStr.trimmed();
        if (trimmed.isEmpty()) continue;
        
        QSqlQuery query;
        if (!query.exec(trimmed)) {
            return false;
        }
    }
    return true;
}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    
    QString configPath = "/usr/lib/cgi-bin/azzammar/config.ini";
    QString sqlPath = "/usr/lib/cgi-bin/azzammar/data/init.sql";

    if (!DBManager::instance().initialize(configPath)) {
        QJsonObject res;
        res["status"] = "error";
        res["message"] = "Database initialization failed via config.ini.";
        sendResponse(res);
        return 1;
    }

    QJsonObject response;
    QSqlQuery query("SELECT COUNT(*) FROM users");
    
    // JIKA TABEL BELUM ADA / QUERY ERROR, LAKUKAN RUN INIT SQL
    if (!query.exec()) {
        if (runInitSql(sqlPath)) {
            // Cek ulang setelah init
            QSqlQuery retryQuery("SELECT COUNT(*) FROM users");
            if (retryQuery.exec() && retryQuery.next() && retryQuery.value(0).toInt() == 0) {
                response["status"] = "setup_required";
                response["message"] = "Database initialized. No accounts found.";
            } else {
                response["status"] = "ready";
                response["message"] = "Database initialized with default seed data.";
            }
        } else {
            response["status"] = "error";
            response["message"] = "Failed to execute init.sql structures.";
        }
    } else {
        // Jika tabel sudah ada sejak awal
        if (query.next() && query.value(0).toInt() == 0) {
            response["status"] = "setup_required";
            response["message"] = "No accounts found. Please register.";
        } else {
            response["status"] = "ready";
            response["message"] = "System is configured and ready.";
        }
    }

    sendResponse(response);
    return 0;
}

