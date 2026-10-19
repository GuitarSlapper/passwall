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
    id: root
    function clearForm() {
        site.clear();
        username.clear();
        password.clear();
        notes.clear();
        master.clear();
    }
    Connections {
        target: vaultManager
        function onLocked() {
            root.clearForm();
            confirmDelete.close();
        }
        function onEntryCreated() {
            root.clearForm();
        }
    }
    ColumnLayout {
        anchors.fill: parent
        spacing: 12
        RowLayout {
            Layout.fillWidth: true
            Label {
                text: vaultManager.unlocked ? qsTr("Your vault") : qsTr("Vault locked")
                font.pixelSize: 28
                font.bold: true
                Layout.fillWidth: true
            }
            BusyIndicator {
                running: vaultManager.busy
                implicitWidth: 32
                implicitHeight: 32
            }
            Button {
                text: qsTr("Refresh")
                visible: vaultManager.unlocked
                enabled: !vaultManager.busy && !authManager.busy
                onClicked: vaultManager.refresh()
            }
            Button {
                text: qsTr("Lock")
                visible: vaultManager.unlocked
                onClicked: vaultManager.lock()
            }
        }
        Label {
            text: vaultManager.message
            visible: text.length > 0
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            textFormat: Text.PlainText
        }
        ColumnLayout {
            visible: !vaultManager.unlocked
            Layout.alignment: Qt.AlignHCenter
            Layout.maximumWidth: 440
            Label {
                text: qsTr("Enter your master password. On a new device, import your encrypted key backup first.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            TextField {
                id: master
                Layout.fillWidth: true
                placeholderText: qsTr("Master password")
                echoMode: TextInput.Password
                maximumLength: 4096
                enabled: !vaultManager.busy && !authManager.busy
                onAccepted: unlockButton.clicked()
                Accessible.name: qsTr("Master password")
            }
            Button {
                id: unlockButton
                text: qsTr("Unlock vault")
                highlighted: true
                enabled: !vaultManager.busy && !authManager.busy
                onClicked: {
                    vaultManager.unlock(master.text);
                    master.clear();
                }
            }
        }
        RowLayout {
            visible: vaultManager.unlocked
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 20
            Frame {
                Layout.fillWidth: true
                Layout.fillHeight: true
                ListView {
                    id: list
                    anchors.fill: parent
                    clip: true
                    spacing: 8
                    model: vaultManager.entries
                    ScrollBar.vertical: ScrollBar {}
                    Label {
                        anchors.centerIn: parent
                        visible: list.count === 0
                        text: qsTr("No entries yet. Add your first password.")
                        width: parent.width
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                    }
                    delegate: Frame {
                        required property var modelData
                        width: ListView.view.width
                        property bool revealed: false
                        ColumnLayout {
                            width: parent.width
                            Label {
                                text: modelData.site
                                font.bold: true
                                font.pixelSize: 18
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                textFormat: Text.PlainText
                            }
                            Label {
                                text: modelData.username
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                textFormat: Text.PlainText
                            }
                            TextField {
                                text: revealed ? vaultManager.password(modelData.id) : "••••••••"
                                readOnly: true
                                Layout.fillWidth: true
                                Accessible.name: qsTr("Stored password")
                            }
                            Label {
                                text: modelData.notes
                                visible: text.length > 0
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                                textFormat: Text.PlainText
                            }
                            RowLayout {
                                Button {
                                    text: revealed ? qsTr("Hide") : qsTr("Reveal")
                                    onClicked: revealed = !revealed
                                }
                                Button {
                                    text: qsTr("Delete")
                                    enabled: !vaultManager.busy && !authManager.busy
                                    onClicked: {
                                        confirmDelete.entryId = modelData.id;
                                        confirmDelete.open();
                                    }
                                }
                            }
                        }
                    }
                }
            }
            ColumnLayout {
                Layout.preferredWidth: 280
                Layout.alignment: Qt.AlignTop
                enabled: !vaultManager.busy && !authManager.busy
                Label {
                    text: qsTr("New entry")
                    font.pixelSize: 20
                    font.bold: true
                }
                TextField {
                    id: site
                    placeholderText: qsTr("Site or title")
                    Layout.fillWidth: true
                    maximumLength: 2048
                    Accessible.name: qsTr("Site or title")
                }
                TextField {
                    id: username
                    placeholderText: qsTr("Username")
                    Layout.fillWidth: true
                    maximumLength: 2048
                    Accessible.name: qsTr("Username")
                }
                TextField {
                    id: password
                    placeholderText: qsTr("Password")
                    echoMode: TextInput.Password
                    Layout.fillWidth: true
                    maximumLength: 4096
                    Accessible.name: qsTr("Entry password")
                }
                ScrollView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 120
                    TextArea {
                        id: notes
                        placeholderText: qsTr("Notes")
                        wrapMode: TextEdit.Wrap
                        Accessible.name: qsTr("Notes")
                    }
                }
                Button {
                    text: qsTr("Save entry")
                    highlighted: true
                    Layout.fillWidth: true
                    onClicked: vaultManager.createEntry(site.text, username.text, password.text, notes.text)
                }
            }
        }
        Item {
            visible: !vaultManager.unlocked
            Layout.fillHeight: true
        }
    }
    Dialog {
        id: confirmDelete
        property string entryId
        anchors.centerIn: parent
        modal: true
        title: qsTr("Delete this entry?")
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: vaultManager.deleteEntry(entryId)
    }
}
