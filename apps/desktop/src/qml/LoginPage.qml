// MIT License
//
// Copyright (c) 2026 GuitarSlapper
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

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
