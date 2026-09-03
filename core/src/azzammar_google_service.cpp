#include "azzammar_google_service.h"
#include <QOAuth2AuthorizationCodeFlow>
#include <QOAuthHttpServerReplyHandler>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QUrl>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDateTime>
#include <QDebug>

//#include <QWebEngineView>
//#include <QWebEnginePage>
//#include <QWebEngineProfile>

#include <cstdlib>
#include <iostream>

AzzammarGoogleService::AzzammarGoogleService(QObject *parent) : QObject(parent) {
    m_networkManager = new QNetworkAccessManager(this);
    m_googleOAuth = new QOAuth2AuthorizationCodeFlow(this);
    m_googleOAuth->setNetworkAccessManager(m_networkManager);

    //m_tokenFilePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/google_token.json";
    QString baseDataPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/azzammar";
    m_tokenFilePath = baseDataPath + "/google_token.json";

    const char* clientIdEnv = std::getenv("GOOGLE_CLIENT_ID");
    const char* clientSecretEnv = std::getenv("GOOGLE_CLIENT_SECRET");

    if (!clientIdEnv || !clientSecretEnv) {
        std::cerr << "Error: Google OAuth environment variables are not set!" << std::endl;
        return;
    }

    QString clientId = clientIdEnv;
    QString clientSecret = clientSecretEnv;

    m_googleOAuth->setClientIdentifier(clientId);
    m_googleOAuth->setClientIdentifierSharedKey(clientSecret);

    // Official Google OAuth 2.0 Base Routing Targets
    m_googleOAuth->setAuthorizationUrl(QUrl("https://accounts.google.com/o/oauth2/v2/auth"));
    m_googleOAuth->setAccessTokenUrl(QUrl("https://oauth2.googleapis.com/token"));

    QStringList scopes;
    scopes << "https://www.googleapis.com/auth/gmail.send"
           << "https://www.googleapis.com/auth/drive.file"
           << "https://www.googleapis.com/auth/spreadsheets"
           << "https://www.googleapis.com/auth/documents";
    m_googleOAuth->setScope(scopes.join(" "));

    // CREATING THE REPLY HANDLER: Tells Qt to expect the callback on a local web server
    auto *replyHandler = new QOAuthHttpServerReplyHandler(8080, this);
    m_googleOAuth->setReplyHandler(replyHandler);
    
    // Intercept the generated URL and forward it via our custom signal
    connect(m_googleOAuth, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, this, &AzzammarGoogleService::onAuthorizationUrlReady);
    connect(m_googleOAuth, &QOAuth2AuthorizationCodeFlow::granted, this, [this]() {
        saveTokenToFile();
        emit authenticated();
    });
    //connect(m_googleOAuth, &QOAuth2AuthorizationCodeFlow::granted, this, &AzzammarGoogleService::authenticated);    
}

/*
// 3. Implement the logic to open QWebEngineView internally
void AzzammarGoogleService::onAuthorizationUrlReady(const QUrl &url) {
    // Create an inline browser window
    QWebEngineView *webView = new QWebEngineView();
    
    // IMPORTANT: Bypass Google's 403 disallowed_useragent block by spoofing a standard browser agent
    QString desktopUserAgent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36";
    webView->page()->profile()->setHttpUserAgent(desktopUserAgent);

    // Optional window setup
    webView->setWindowTitle("Google Authentication");
    webView->resize(600, 700);

    // Auto-close the internal window once authorization succeeds and redirects back to localhost
    connect(m_googleOAuth, &QOAuth2AuthorizationCodeFlow::granted, webView, [webView]() {
        webView->close();
        webView->deleteLater();
    });

    // Load Google's Sign-in page inside the app
    webView->load(url);
    webView->show();
}
*/

void AzzammarGoogleService::saveTokenToFile() {
    QFile file(m_tokenFilePath);
    
    // Pastikan direktori folder pembungkusnya sudah dibuat
    QFileInfo info(m_tokenFilePath);
    info.dir().mkpath(".");

    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject json;
        json["access_token"] = m_googleOAuth->token();
        json["refresh_token"] = m_googleOAuth->refreshToken();
        json["expiration"] = m_googleOAuth->expirationAt().toString(Qt::ISODate);

        QJsonDocument doc(json);
        file.write(doc.toJson());
        file.close();
        qDebug() << "Token Google berhasil disimpan dengan aman di:" << m_tokenFilePath;
    }
}

bool AzzammarGoogleService::loadTokenFromFile() {
    QFile file(m_tokenFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false; // File tidak ditemukan, harus login pertama kali via browser
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject json = doc.object();

    QString accessToken = json["access_token"].toString();
    QString refreshToken = json["refresh_token"].toString();
    QDateTime expiration = QDateTime::fromString(json["expiration"].toString(), Qt::ISODate);

    if (refreshToken.isEmpty()) {
        return false;
    }

    // Suntikkan token lama kembali ke dalam objek state Qt Network Authorization
    m_googleOAuth->setToken(accessToken);
    m_googleOAuth->setRefreshToken(refreshToken);
    
    qDebug() << "Token lama ditemukan. Azzammar mencoba masuk menggunakan Refresh Token...";
    
    // Minta Qt memperbarui Access Token secara diam-diam di latar belakang
    m_googleOAuth->refreshAccessToken();
    return true;
}

void AzzammarGoogleService::setupAuthentication(const QString &clientId, const QString &clientSecret) {
    m_googleOAuth->setClientIdentifier(clientId);
    m_googleOAuth->setClientIdentifierSharedKey(clientSecret);
}

void AzzammarGoogleService::grantAccess() {
    if (loadTokenFromFile()) {
        qDebug() << "Token found. Skipping browser login.";
        return; 
    }

    qDebug() << "No token found. Preparing browser authorization flow...";

    m_googleOAuth->setModifyParametersFunction([](QAbstractOAuth::Stage stage, QMultiMap<QString, QVariant> *parameters) {
        if (stage == QAbstractOAuth::Stage::RequestingAuthorization) {
            parameters->insert("access_type", "offline");
            parameters->insert("prompt", "consent");
        }
    });

    m_googleOAuth->grant();
}

void AzzammarGoogleService::uploadToDrive(const QString &filePath, const QString &driveFolderId) {
    QFile *file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        qCritical() << "Cannot open file for Drive upload:" << filePath;
        delete file;
        return;
    }

    qDebug() << "Preparing Google Drive upload for:" << filePath;

    QUrl url("https://www.googleapis.com/upload/drive/v3/files?uploadType=multipart");
    QNetworkRequest request(url);

    QString bearerToken = "Bearer " + m_googleOAuth->token();
    request.setRawHeader("Authorization", bearerToken.toUtf8());

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart metadataPart;
    metadataPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json; charset=UTF-8"));
    
    QJsonObject metadata;
    QFileInfo fileInfo(filePath);
    metadata["name"] = fileInfo.fileName();
    if (!driveFolderId.isEmpty()) {
        QJsonArray parents;
        parents.append(driveFolderId);
        metadata["parents"] = parents;
    }
    
    QJsonDocument doc(metadata);
    metadataPart.setBody(doc.toJson());
    multiPart->append(metadataPart);

    // Media part: Binary payload data
    QHttpPart mediaPart;
    mediaPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));
    mediaPart.setBodyDevice(file);
    file->setParent(multiPart); // Ensure file is deleted alongside the multipart packet container
    multiPart->append(mediaPart);

    // 4. Send the payload over the network manager
    QNetworkReply *reply = m_networkManager->post(request, multiPart);
    multiPart->setParent(reply); // Memory clean up allocation mapping

    // 5. Track the network reply loop responses
    connect(reply, &QNetworkReply::finished, [reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument responseDoc = QJsonDocument::fromJson(response);
            qDebug() << "Success! File uploaded safely. Google File ID:" << responseDoc.object()["id"].toString();
        } else {
            qCritical() << "Google Drive Upload failed with error:" << reply->errorString();
            qDebug() << "Raw Server Response details:" << reply->readAll();
        }
        reply->deleteLater();
    });
}

void AzzammarGoogleService::createSpreadsheet(const QString &title, const QStringList &sheetNames) {
    QUrl url("https://googleapis.com");
    QNetworkRequest request(url);
    
    QString bearerToken = "Bearer " + m_googleOAuth->token();
    request.setRawHeader("Authorization", bearerToken.toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject root;
    QJsonObject properties;
    properties["title"] = title;
    root["properties"] = properties;

    QJsonArray sheetsArray;
    for (const QString &name : sheetNames) {
        QJsonObject sheetObj;
        QJsonObject sheetProperties;
        sheetProperties["title"] = name;
        sheetObj["properties"] = sheetProperties;
        sheetsArray.append(sheetObj);
    }
    root["sheets"] = sheetsArray;

    QJsonDocument doc(root);
    QNetworkReply *reply = m_networkManager->post(request, doc.toJson());

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonObject responseObj = QJsonDocument::fromJson(reply->readAll()).object();
            emit spreadsheetCreated(responseObj["spreadsheetId"].toString(), responseObj["spreadsheetUrl"].toString());
        } else {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
        }
    });
}

void AzzammarGoogleService::appendRows(const QString &spreadsheetId, const QString &range, const QList<QStringList> &rowsData) {
    QString baseDomain = "https://sheets.googleapis.com";
    QString apiPath = "/v4/spreadsheets/" + spreadsheetId + "/values/" + range + ":append?valueInputOption=USER_ENTERED";
    QString path = baseDomain + apiPath;

    QNetworkRequest request((QUrl(path)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QString bearerToken = "Bearer " + m_googleOAuth->token();
    request.setRawHeader("Authorization", bearerToken.toUtf8());

    QJsonObject payload;
    payload["range"] = range;
    payload["majorDimension"] = "ROWS";

    QJsonArray matrixContainer;
    for (const QStringList &row : rowsData) {
        QJsonArray jsonRow;
        for (const QString &cell : row) {
            jsonRow.append(cell);
        }
        matrixContainer.append(jsonRow);
    }
    payload["values"] = matrixContainer;

    QJsonDocument doc(payload);
    QNetworkReply *reply = m_networkManager->post(request, doc.toJson());

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            emit rowsAppendedSuccessfully();
        } else {
            emit errorOccurred(reply->errorString());
        }
        reply->deleteLater();
    });
}

void AzzammarGoogleService::readRows(const QString &spreadsheetId, const QString &range) {
    QString baseDomain = "https://sheets.googleapis.com";
    QString apiPath = "/v4/spreadsheets/" + spreadsheetId + "/values/" + range + ":append?valueInputOption=USER_ENTERED";
    QString path = baseDomain + apiPath;

    QNetworkRequest request((QUrl(path)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QString bearerToken = "Bearer " + m_googleOAuth->token();
    request.setRawHeader("Authorization", bearerToken.toUtf8());

    QNetworkReply *reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        QList<QStringList> outputGrid;
        if (reply->error() == QNetworkReply::NoError) {
            QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();
            QJsonArray values = root["values"].toArray();
            
            for (int i = 0; i < values.size(); ++i) {
                QStringList structuralRow;
                QJsonArray rowArray = values[i].toArray();
                for (int j = 0; j < rowArray.size(); ++j) {
                    structuralRow.append(rowArray[j].toString());
                }
                outputGrid.append(structuralRow);
            }
            emit dataRowsReceived(outputGrid);
        } else {
            emit errorOccurred(reply->errorString());
        }
        reply->deleteLater();
    });
}

