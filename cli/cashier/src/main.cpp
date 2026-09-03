#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QDateTime>
#include "azzammar_google_service.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("azzammar-cashier");
    QCoreApplication::setApplicationVersion("1.0.0");

    AzzammarGoogleService dbEngine;

    // Connect functional signal handlers for generic output messaging
    QObject::connect(&dbEngine, &AzzammarGoogleService::rowsAppendedSuccessfully, [&app]() {
        qDebug() << "SUCCESS: Cashier transaction record successfully updated in database storage!";
        app.quit();
    });

    QObject::connect(&dbEngine, &AzzammarGoogleService::errorOccurred, [&app](const QString &err) {
        qCritical() << "CRITICAL DATABASE REJECTION ERROR:" << err;
        app.exit(1);
    });

    // Fire verification loop checks internally
    QObject::connect(&dbEngine, &AzzammarGoogleService::authenticated, [&dbEngine]() {
        qDebug() << "Azzammar Engine Live. Mapping cashier business variables into database arrays...";

        // Cashier Business Layer Variables (handled completely inside the application)
        QString currentTimestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        QString cashierId = "Slamet_Wongslam_01";
        QString totalIncome = "1250000.00";
        QString totalExpense = "0.00";
        QString memoLog = "End of Shift Validation Entry";

        // Map abstract transactional records into generic matrices
        QStringList dataRow;
        dataRow << currentTimestamp << cashierId << totalIncome << totalExpense << memoLog;

        QList<QStringList> bulkTransactionBatch;
        bulkTransactionBatch.append(dataRow);

        // Targeted remote storage coordinates (Production Sheet tracking ID)
        QString spreadsheetId = "16TprkzJoz-jrTNIB_ttFACEK8OE8Lz9wW_rlWmbBV64"; // Replace with your real spreadsheet ID
        QString range = "Sheet1!A1";

        dbEngine.appendRows(spreadsheetId, range, bulkTransactionBatch);
    });

    dbEngine.grantAccess(); // Quietly loads tokens and performs operations headlessly

    return app.exec();
}

