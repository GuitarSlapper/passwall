import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    signal loginRequested
    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width, 440)
        spacing: 10
        enabled: !authManager.busy
        Label {
            text: qsTr("Create your wallet")
            font.pixelSize: 28
            font.bold: true
        }
        Label {
            text: qsTr("Your master password stays on this device and unlocks your vault. Keep it safe: it cannot be recovered.")
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        TextField {
            id: server
            Layout.fillWidth: true
            text: authManager.serverUrl
            placeholderText: qsTr("Server URL")
            Accessible.name: qsTr("Server URL")
        }
        TextField {
            id: email
            Layout.fillWidth: true
            placeholderText: qsTr("Email")
            maximumLength: 254
            Accessible.name: qsTr("Email")
        }
        TextField {
            id: password
            Layout.fillWidth: true
            placeholderText: qsTr("Authentication password (8+ characters)")
            echoMode: TextInput.Password
            maximumLength: 4096
            Accessible.name: qsTr("Authentication password")
        }
        TextField {
            id: master
            Layout.fillWidth: true
            placeholderText: qsTr("Master password (8+ characters)")
            echoMode: TextInput.Password
            maximumLength: 4096
            Accessible.name: qsTr("Master password")
        }
        TextField {
            id: confirm
            Layout.fillWidth: true
            placeholderText: qsTr("Repeat master password")
            echoMode: TextInput.Password
            maximumLength: 4096
            Accessible.name: qsTr("Repeat master password")
        }
        Label {
            id: validation
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }
        Button {
            text: qsTr("Create account")
            highlighted: true
            Layout.fillWidth: true
            onClicked: {
                if (master.text !== confirm.text) {
                    validation.text = qsTr("Master passwords do not match.");
                    return;
                }
                validation.text = "";
                if (authManager.setServerUrl(server.text)) {
                    authManager.registerUser(email.text, password.text, master.text);
                    password.clear();
                    master.clear();
                    confirm.clear();
                }
            }
        }
        Button {
            text: qsTr("Back to sign in")
            flat: true
            Layout.fillWidth: true
            onClicked: loginRequested()
        }
    }
    BusyIndicator {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        running: authManager.busy
    }
}
