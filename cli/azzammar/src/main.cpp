#include <QDesktopServices>
#include <QGuiApplication> 
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QEventLoop>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>
#include <iostream>
#include "azzammar_core.h"
#include "azzammar_google_service.h"

#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineProfile>

int main(int argc, char *argv[]) {
//#ifdef HAS_GUI
    QCoreApplication::setAttribute(Qt::AA_UseOpenGLES, false); 
    
    QGuiApplication app(argc, argv);
//#else
//    QCoreApplication app(argc, argv);
//#endif

    QCoreApplication::setApplicationName("azzammar");
    QCoreApplication::setApplicationVersion("1.0.0");

    // Instantiating the shared library logic
    AzzammarCore coreEngine;

    QCommandLineParser parser;
    parser.setApplicationDescription("Azzammar Ecosystem Command Line Interface");
    parser.addHelpOption();
    parser.addVersionOption();

/*
    // Define a sample command argument
    QCommandLineOption initOption(QStringList() << "i" << "init", "Initialize the system environment.");
    parser.addOption(initOption);

    parser.process(app);

    if (parser.isSet(initOption)) {
        if (coreEngine.initializeEcosystem()) {
            std::cout << "Success: System configured." << std::endl;
        } else {
            std::cerr << "Error: Initialization failed." << std::endl;
            return 1;
        }
        return 0; // Exit successfully after running the CLI command
    }
*/

    // Define commands for setting up environment or forcing login validation
    QCommandLineOption authOption(QStringList() << "a" << "auth", "Authorize and bind your Google Cloud Account storage bridge.");
    parser.addOption(authOption);

    parser.process(app);

    // If the user runs: azzammar_cli --auth
    if (parser.isSet(authOption)) {
        std::cout << "--------------------------------------------------" << std::endl;
        std::cout << " Azzammar Security Platform: Initializing Google Sync Setup..." << std::endl;
        std::cout << "--------------------------------------------------" << std::endl;

        AzzammarGoogleService googleService;
        QEventLoop loop; // Keeps the CLI alive while waiting for the async network response

        // 1. Listen for the secure URL link to print onto the terminal screen
        QObject::connect(&googleService, &AzzammarGoogleService::onAuthorizationUrlReady, [](const QUrl &url) {
            //std::cout << "\n COPY AND PASTE THIS URL INTO YOUR WEB BROWSER TO LOG IN:\n" << std::endl;
            //std::cout << url.toString().toStdString() << std::endl;
            //std::cout << "\nWaiting for Google authentication callback on port 8080..." << std::endl;

            std::cout << "\n Launching your default web browser for secure login..." << std::endl;
    
            // Automatically launches Safari/Chrome/Firefox right on the user's desktop!
            QDesktopServices::openUrl(url); 
            //onAuthorizationUrlReady(url);
    
            std::cout << "\nIf the browser window did not appear, copy and paste this link manually:\n" << std::endl;
            std::cout << url.toString().toStdString() << std::endl;
            std::cout << "\nWaiting for Google authentication callback on port 8080..." << std::endl;

        });

        // 2. Listen for the successful login token capture event
        QObject::connect(&googleService, &AzzammarGoogleService::authenticated, [&loop]() {
            std::cout << "\n SUCCESS: Google Account successfully linked to your Azzammar profile!" << std::endl;
            std::cout << "Tokens saved securely inside your application data home path folder." << std::endl;
            loop.quit(); // Exit the event loop cleanly once authenticated
        });

        // 3. Jumpstart the authentication grant validation check pass
        googleService.grantAccess();

        return loop.exec(); // Execute the terminal block loop
    }

    // Default response if no arguments are passed
    std::cout << coreEngine.getSystemVersion().toStdString() << std::endl;
    std::cout << "Run 'azzammar_cli --auth' to configure your cloud synchronization profile." << std::endl;
    std::cout << "Use --help to see available commands." << std::endl;

    return 0;
}

/*
void main::onAuthorizationUrlReady(const QUrl &url) {
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
