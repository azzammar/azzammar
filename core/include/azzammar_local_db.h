#ifndef AZZAMMAR_LOCAL_DB_H
#define AZZAMMAR_LOCAL_DB_H

#include <QObject>
#include <QSqlDatabase>
#include <QList>
#include <QStringList>

class AzzammarLocalDb : public QObject {
    Q_OBJECT
public:
    explicit AzzammarLocalDb(QObject *parent = nullptr);
    ~AzzammarLocalDb();

    bool initDatabase(const QString &dbName = "azzammar_local.db");
    
    // Generic Data Handlers
    bool executeNonQuery(const QString &queryStr, const QVariantList &bindValues = {});
    QList<QStringList> executeQuery(const QString &queryStr, const QVariantList &bindValues = {});

    QString getDatabaseFilePath() const;

private:
    QSqlDatabase m_db;
    QString m_dbPath;
};

#endif // AZZAMMAR_LOCAL_DB_H
