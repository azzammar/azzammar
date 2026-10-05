#include <QCoreApplication>
#include <QTextStream>
#include <QJsonObject>
#include <QJsonDocument>
#include <QSqlQuery>
#include <QProcessEnvironment>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include "core-auth/dbmanager.hpp"

void sendResponse(const QJsonObject& json) {
    QTextStream out(stdout);
    out << "Content-Type: application/json\r\n\r\n";
    out << QJsonDocument(json).toJson(QJsonDocument::Compact);
}

// Tambahkan fungsi penanganan request token di dalam main.cpp modul core-auth Anda
void processGoogleTokenExchange(const QJsonObject& jsonInput) {
    QString authCode = jsonInput["authorization_code"].toString();
    QString clientEmail = jsonInput["google_email"].toString();

    // Di sini sistem Qt NetworkAuth bertugas menukar Code menjadi Refresh Token ke Google OAuth Endpoint
    // Untuk kebutuhan modularitas container, setelah divalidasi oleh cloud handler azzammar.com,
    // data disimpan ke SQLite lokal agar kontainer bisa berjalan offline mandiri.
    
    QSqlQuery query;
    query.prepare("UPDATE users SET google_email = :gemails, google_refresh_token = :gref "
                  "WHERE id = 1");
    query.bindValue(":gemails", clientEmail);
    query.bindValue(":gref", jsonInput["refresh_token"].toString());

    QJsonObject response;
    if (query.exec()) {
        response["status"] = "success";
        response["message"] = "Google API Access Pipeline successfully bound to Local SQLite.";
    } else {
        response["status"] = "error";
        response["message"] = "Failed to write token configurations.";
    }
    
    QTextStream out(stdout);
    out << "Content-Type: application/json\r\n\r\n";
    out << QJsonDocument(response).toJson(QJsonDocument::Compact);
}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    
    QString configPath = "/usr/lib/cgi-bin/azzammar/config.ini";
    if (!DBManager::instance().initialize(configPath)) {
        QJsonObject res;
        res["status"] = "error";
        res["message"] = "Database binding failed in auth module.";
        sendResponse(res);
        return 1;
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString requestMethod = env.value("REQUEST_METHOD");
    QJsonObject response;

    if (requestMethod == "POST") {
        // TAHAP REGISTRASI/UPDATE TOKEN BARU (Menerima payload dari setup wizard / cloud bridging)
        QTextStream in(stdin);
        QString rawPostData = in.readAll();
        QJsonDocument postDoc = QJsonDocument::fromJson(rawPostData.toUtf8());
        
        if (postDoc.isObject()) {
            QJsonObject inputData = postDoc.object();
            QString email = inputData["email"].toString();
            QString gEmail = inputData["google_email"].toString();
            QString gId = inputData["google_id"].toString();
            QString refreshToken = inputData["google_refresh_token"].toString();
            QString bridgeKey = inputData["bridge_key"].toString();
            QString accType = inputData["account_type"].toString();

            QSqlQuery query;
            query.prepare("INSERT OR REPLACE INTO users (id, email, google_email, google_id, google_refresh_token, private_key_bridge, account_type) VALUES (1, :email, :gemails, :gid, :gref, :gbridge, :acctype)");
            query.bindValue(":email", email);
            query.bindValue(":gemails", gEmail);
            query.bindValue(":gid", gId);
            query.bindValue(":gref", refreshToken);
            query.bindValue(":gbridge", bridgeKey);
            query.bindValue(":acctype", accType);

            if (query.exec()) {
                response["status"] = "success";
                response["message"] = "Google integration and license key bound successfully.";
            } else {
                response["status"] = "error";
                response["message"] = "Failed to update database profile rows.";
            }
        } else {
            response["status"] = "error";
            response["message"] = "Invalid JSON payload structure.";
        }
        sendResponse(response);
    } else {
        // TAHAP GET (Membaca info status kesiapan autentikasi Google)
        QSqlQuery query("SELECT google_email, account_type FROM users WHERE id = 1");
        if (query.exec() && query.next()) {
            response["status"] = "authenticated";
            response["google_email"] = query.value("google_email").toString();
            response["account_type"] = query.value("account_type").toString();
            response["api_gateway"] = "Google API OAuth 2.0 Engine Active";
        } else {
            response["status"] = "unauthenticated";
            response["message"] = "Ecosystem has not been tied to any Google Account.";
        }
        sendResponse(response);
    }

    return 0;
}

