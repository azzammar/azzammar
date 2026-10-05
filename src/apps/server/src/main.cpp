#include <QCoreApplication>
#include <QDebug>
#include "azzammar_core.h"
#include "azzammar_google_service.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    AzzammarCore coreEngine;
    qDebug() << "Starting Server background process for: " << coreEngine.getSystemVersion();
    
    if (!coreEngine.initializeEcosystem()) {
        qCritical() << "Failed to start engine dependency. Server shutting down.";
        return 1;
    }

    // Set up our Google API integration using your credentials
    AzzammarGoogleService googleService;

    // Connect to the signal to print the URL when it is ready
    QObject::connect(&googleService, &AzzammarGoogleService::onAuthorizationUrlReady, [](const QUrl &url) {
        qDebug() << "\n-> To authenticate, navigate to this URL in your web browser:\n" 
                 << url.toString() << "\n";
    });

    // Monitor when authentication succeeds completely
    QObject::connect(&googleService, &AzzammarGoogleService::authenticated, []() {
        qDebug() << "Yeah! Azzammar core successfully received and verified the Google Tokens!";
    });

    // Fire the process to generate the login token link
    googleService.grantAccess();

    qDebug() << "Test upload file test_azzammar.txt..";
    googleService.uploadToDrive("test_azzammar.txt", "");
    qDebug() << "Test upload file test_azzammar.txt.. done!";

    return app.exec();
}

