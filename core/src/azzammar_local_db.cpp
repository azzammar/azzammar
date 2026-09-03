#include "azzammar_local_db.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

AzzammarLocalDb::AzzammarLocalDb(QObject *parent) : QObject(parent) {}

AzzammarLocalDb::~AzzammarLocalDb() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool AzzammarLocalDb::initDatabase(const QString &dbName) {
    // Dynamically choose the correct writable path regardless of OS (Android, iOS, macOS, Windows)
    m_dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + dbName;
    
    QFileInfo info(m_dbPath);
    info.dir().mkpath("."); // Force folder generation safely

    m_db = QSqlDatabase::addDatabase("QSQLITE", "AzzammarLocalConnection");
    m_db.setDatabaseName(m_dbPath);

    if (!m_db.open()) {
        qCritical() << "❌ Failed to open embedded SQLite engine:" << m_db.lastError().text();
        return false;
    }

    qDebug() << "💾 Embedded SQLite Engine initialized safely at path:" << m_dbPath;

    // Create a base journal matrix layout table if it doesn't exist
    QString createTableQuery = 
        "CREATE TABLE IF NOT EXISTS general_ledger ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "timestamp TEXT, "
        "account_tag TEXT, "
        "debit REAL, "
        "credit REAL, "
        "notes TEXT"
        ");";
        
    return executeNonQuery(createTableQuery);
}

bool AzzammarLocalDb::executeNonQuery(const QString &queryStr, const QVariantList &bindValues) {
    QSqlQuery query(m_db);
    query.prepare(queryStr);
    
    for (const QVariant &val : bindValues) {
        query.addBindValue(val);
    }

    if (!query.exec()) {
        qCritical() << "❌ SQL Execution Warning:" << query.lastError().text();
        return false;
    }
    return true;
}

QList<QStringList> AzzammarLocalDb::executeQuery(const QString &queryStr, const QVariantList &bindValues) {
    QList<QStringList> resultGrid;
    QSqlQuery query(m_db);
    query.prepare(queryStr);

    for (const QVariant &val : bindValues) {
        query.addBindValue(val);
    }

    if (!query.exec()) {
        qCritical() << "❌ SQL Query Selection Failure:" << query.lastError().text();
        return resultGrid;
    }

    int columnCount = query.record().count();
    while (query.next()) {
        QStringList row;
        for (int i = 0; i < columnCount; ++i) {
            row.append(query.value(i).toString());
        }
        resultGrid.append(row);
    }
    return resultGrid;
}

QString AzzammarLocalDb::getDatabaseFilePath() const {
    return m_dbPath;
}
