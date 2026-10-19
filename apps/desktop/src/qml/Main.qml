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
import QtQuick.Dialogs

ApplicationWindow {
    id: window
    width: 1060
    height: 740
    minimumWidth: 760
    minimumHeight: 600
    visible: true
    title: qsTr("Passwall")
    property bool registering: false
    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.margins: 8
            Label {
                text: qsTr("PASSWALL")
                font.bold: true
                font.pixelSize: 20
            }
            Label {
                text: authManager.authenticated ? authManager.email : qsTr("Your private password wallet")
                Layout.fillWidth: true
                elide: Text.ElideRight
                textFormat: Text.PlainText
            }
            Button {
                text: qsTr("Export key backup")
                enabled: authManager.email.length > 0 && !authManager.busy
                onClicked: exportDialog.open()
            }
            Button {
                text: qsTr("Import key")
                visible: authManager.authenticated
                enabled: !authManager.busy
                onClicked: importDialog.open()
            }
            Button {
                text: qsTr("Sign out")
                visible: authManager.authenticated
                enabled: !authManager.busy
                onClicked: authManager.logout()
            }
        }
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16
        Label {
            text: authManager.message
            visible: text.length > 0
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            textFormat: Text.PlainText
        }
        Loader {
            Layout.fillWidth: true
            Layout.fillHeight: true
            sourceComponent: authManager.authenticated ? vaultPage : (window.registering ? registerPage : loginPage)
        }
    }
    Component {
        id: loginPage
        LoginPage {
            onRegisterRequested: window.registering = true
        }
    }
    Component {
        id: registerPage
        RegisterPage {
            onLoginRequested: window.registering = false
        }
    }
    Component {
        id: vaultPage
        VaultPage {}
    }
    Connections {
        target: authManager
        function onRegistered() {
            window.registering = false;
        }
    }
    FileDialog {
        id: exportDialog
        title: qsTr("Export encrypted vault key")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: ["JSON backups (*.json)"]
        onAccepted: authManager.exportKey(selectedFile)
    }
    FileDialog {
        id: importDialog
        title: qsTr("Import encrypted vault key")
        fileMode: FileDialog.OpenFile
        nameFilters: ["JSON backups (*.json)"]
        onAccepted: authManager.importKey(selectedFile)
    }
}
