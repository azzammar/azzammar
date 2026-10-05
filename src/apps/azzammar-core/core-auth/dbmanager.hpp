#ifndef DBMANAGER_HPP
#define DBMANAGER_HPP

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSettings>
#include <QDebug>
#include <QCoreApplication>

class DBManager {
public:
    static DBManager& instance() {
        static DBManager instance;
        return instance;
    }

    bool initialize(const QString& configPath) {
        // Membaca file konfigurasi eksternal
        QSettings settings(configPath, QSettings::IniFormat);
        
        // Mengambil tipe driver: 'QSQLITE', 'QMYSQL', atau 'QPSQL'
        QString driver = settings.value("Database/Driver", "QSQLITE").toString();
        m_db = QSqlDatabase::addDatabase(driver);

        if (driver == "QSQLITE") {
            QString dbPath = settings.value("Database/Path", "data/azzammar.db").toString();
            m_db.setDatabaseName(dbPath);
        } else {
            // Konfigurasi untuk DB Server eksternal jika user merubahnya
            m_db.setHostName(settings.value("Database/Host", "localhost").toString());
            m_db.setDatabaseName(settings.value("Database/Name", "azzammar").toString());
            m_db.setUserName(settings.value("Database/User", "root").toString());
            m_db.setPassword(settings.value("Database/Password", "").toString());
            m_db.setPort(settings.value("Database/Port", 3306).toInt());
        }

        if (!m_db.open()) {
            qCritical() << "Gagal membuka database:" << m_db.lastError().text();
            return false;
        }

        return true;
    }

    QSqlDatabase database() const { return m_db; }

private:
    DBManager() = default;
    QSqlDatabase m_db;
};

#endif // DBMANAGER_HPP

