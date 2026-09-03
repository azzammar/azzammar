#ifndef AZZAMMAR_CRYPTO_H
#define AZZAMMAR_CRYPTO_H

#include <QObject>
#include <QByteArray>
#include <QString>

class AzzammarCrypto : public QObject {
    Q_OBJECT
public:
    explicit AzzammarCrypto(QObject *parent = nullptr);

    // Core Encription Operations
    static QByteArray encryptData(const QByteArray &rawData, const QString &secretKey);
    static QByteArray decryptData(const QByteArray &encryptedData, const QString &secretKey);

    // Specialized File Helpers for Cloud Syncing
    static bool encryptFile(const QString &sourcePath, const QString &targetPath, const QString &secretKey);
    static bool decryptFile(const QString &sourcePath, const QString &targetPath, const QString &secretKey);
};

#endif // AZZAMMAR_CRYPTO_H

