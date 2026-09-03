import QtQuick
import QtQuick.Controls
import QtWebView

Item {
    width: parent.width
    height: parent.height

    // This container displays the secure Google workspace login page natively inside Azzammar
    Rectangle {
        id: webViewWrapper
        anchors.fill: parent
        visible: false // Hidden by default, becomes visible when login is requested

        Column {
            anchors.fill: parent
            
            // Top Bar navigation decoration
            Rectangle {
                width: parent.width
                height: 50
                color: "#1A73E8" // Google Blue brand color Accent
                Text {
                    anchors.centerIn: parent
                    text: "Link Google Account to Azzammar"
                    color: "white"
                    font.bold: true
                }
            }

            WebView {
                id: oauthWebView
                width: parent.width
                height: parent.height - 50
                
                // Monitor the URL changes dynamically inside the app canvas
                onUrlChanged: {
                    var currentUrl = url.toString();
                    console.log("Current WebView URL Navigation Target:", currentUrl);
                    
                    // The millisecond Google attempts to route to localhost with the token credentials
                    if (currentUrl.includes("http://localhost:8080") && currentUrl.includes("code=")) {
                        console.log("🎉 Captured Token parameters! Closing login overlay panel...");
                        webViewWrapper.visible = false; // Hide the browser view immediately
                        
                        // Azzammar core library handles the background token storage 
                        // automatically through the running m_networkManager loops!
                    }
                }
            }
        }
    }

    // High level visual setup function to call from your C++ controllers logic layer
    function launchEmbeddedLogin(authUrl) {
        oauthWebView.url = authUrl;
        webViewWrapper.visible = true; // Slide the secure login panel into view
    }
}

