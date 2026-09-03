#ifndef AZZAMMAR_GOOGLE_SERVICE_H
#define AZZAMMAR_GOOGLE_SERVICE_H

#include <QObject>
#include <QUrl>
#include <QStringList>
#include <QList>

class QOAuth2AuthorizationCodeFlow;
class QNetworkAccessManager;

class AzzammarGoogleService : public QObject {
    Q_OBJECT
public:
    explicit AzzammarGoogleService(QObject *parent = nullptr);
    
    void setupAuthentication(const QString &clientId, const QString &clientSecret);
    void grantAccess(); 
    void uploadToDrive(const QString &filePath, const QString &driveFolderId);

    void createSpreadsheet(const QString &title, const QStringList &sheetNames);
    void appendRows(const QString &spreadsheetId, const QString &range, const QList<QStringList> &rowsData);
    void readRows(const QString &spreadsheetId, const QString &range);

    void saveTokenToFile();
    bool loadTokenFromFile();
signals:
    void onAuthorizationUrlReady(const QUrl &url); 

    void spreadsheetCreated(const QString &spreadsheetId, const QString &url);
    void rowsAppendedSuccessfully();
    void dataRowsReceived(const QList<QStringList> &rows);

    void authenticated();
    void errorOccurred(const QString &error);

private:
    QOAuth2AuthorizationCodeFlow *m_googleOAuth;
    QNetworkAccessManager *m_networkManager;

    QString m_tokenFilePath;
};

#endif // AZZAMMAR_GOOGLE_SERVICE_H
