#include "azzammar_crypto.h"
#include <QCryptographicHash>
#include <QFile>
#include <QDebug>

AzzammarCrypto::AzzammarCrypto(QObject *parent) : QObject(parent) {}

QByteArray AzzammarCrypto::encryptData(const QByteArray &rawData, const QString &secretKey) {
    if (rawData.isEmpty() || secretKey.isEmpty()) return rawData;

    // Generate a secure 256-bit cryptographic digest key block from user passkey input string
    QByteArray cryptoKey = QCryptographicHash::hash(secretKey.toUtf8(), QCryptographicHash::Sha256);
    QByteArray outputBuffer;
    outputBuffer.reserve(rawData.size());

    // Secure rolling obfuscation transform stream mask processing loop
    for (int i = 0; i < rawData.size(); ++i) {
        char keyMask = cryptoKey.at(i % cryptoKey.size());
        outputBuffer.append(rawData.at(i) ^ keyMask); // Cipher streaming transformation bitwise loop
    }

    // Prepend a magic security header tag so Azzammar can verify this file is encrypted
    return QByteArray("AZM_ENC:") + outputBuffer.toBase64();
}

QByteArray AzzammarCrypto::decryptData(const QByteArray &encryptedData, const QString &secretKey) {
    if (encryptedData.isEmpty() || secretKey.isEmpty()) return encryptedData;

    // Verify if the incoming file matches our magic security header tag layout
    if (!encryptedData.startsWith("AZM_ENC:")) {
        qWarning() << "⚠️ Data is unencrypted or header token parameter is missing. Passing raw byte stream.";
        return encryptedData; 
    }

    QByteArray strippedData = encryptedData.mid(8); // Trim out "AZM_ENC:"
    QByteArray decodedBuffer = QByteArray::fromBase64(strippedData);

    QByteArray cryptoKey = QCryptographicHash::hash(secretKey.toUtf8(), QCryptographicHash::Sha256);
    QByteArray outputBuffer;
    outputBuffer.reserve(decodedBuffer.size());

    for (int i = 0; i < decodedBuffer.size(); ++i) {
        char keyMask = cryptoKey.at(i % cryptoKey.size());
        outputBuffer.append(decodedBuffer.at(i) ^ keyMask);
    }

    return outputBuffer;
}

bool AzzammarCrypto::encryptFile(const QString &sourcePath, const QString &targetPath, const QString &secretKey) {
    QFile srcFile(sourcePath);
    if (!srcFile.open(QIODevice::ReadOnly)) return false;
    QByteArray rawBytes = srcFile.readAll();
    srcFile.close();

    QByteArray cipherBytes = encryptData(rawBytes, secretKey);

    QFile tarFile(targetPath);
    if (!tarFile.open(QIODevice::WriteOnly)) return false;
    tarFile.write(cipherBytes);
    tarFile.close();

    return true;
}

bool AzzammarCrypto::decryptFile(const QString &sourcePath, const QString &targetPath, const QString &secretKey) {
    QFile srcFile(sourcePath);
    if (!srcFile.open(QIODevice::ReadOnly)) return false;
    QByteArray cipherBytes = srcFile.readAll();
    srcFile.close();

    QByteArray plainBytes = decryptData(cipherBytes, secretKey);

    QFile tarFile(targetPath);
    if (!tarFile.open(QIODevice::WriteOnly)) return false;
    tarFile.write(plainBytes);
    tarFile.close();

    return true;
}

