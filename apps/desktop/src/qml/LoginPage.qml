import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    signal registerRequested
    function submit() {
        if (authManager.setServerUrl(server.text)) {
            authManager.login(email.text, password.text);
            password.clear();
        }
    }
    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width, 420)
        spacing: 14
        enabled: !authManager.busy
        Label {
            text: qsTr("Welcome back")
            font.pixelSize: 30
            font.bold: true
        }
        Label {
            text: qsTr("Sign in with your authentication password.")
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
            text: authManager.email
            placeholderText: qsTr("Email")
            inputMethodHints: Qt.ImhEmailCharactersOnly
            maximumLength: 254
            Accessible.name: qsTr("Email")
        }
        TextField {
            id: password
            Layout.fillWidth: true
            placeholderText: qsTr("Authentication password")
            echoMode: TextInput.Password
            maximumLength: 4096
            onAccepted: submit()
            Accessible.name: qsTr("Authentication password")
        }
        Button {
            text: qsTr("Sign in")
            highlighted: true
            Layout.fillWidth: true
            onClicked: submit()
        }
        Button {
            text: qsTr("Create an account")
            flat: true
            Layout.fillWidth: true
            onClicked: registerRequested()
        }
    }
    BusyIndicator {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        running: authManager.busy
    }
}
