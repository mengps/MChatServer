import QtQuick 2.12
import QtQuick.Window 2.2
import QtQuick.Controls 1.4
import QtQuick.Controls 2.2
import an.framelessWindow 1.0

FramelessWindow
{
    id: root
    visible: true
    width: 640
    height: 480
    x: (Screen.desktopAvailableWidth - actualWidth) / 2
    y: (Screen.desktopAvailableHeight - actualHeight) / 2
    actualHeight: height
    actualWidth: width
    title: qsTr("MPS Chat 服务器")
    taskbarHint: true
    windowIcon: "qrc:/image/winIcon.png"

    property var usersArray: new Array;

    function displayServerInfo(ip, port)
    {
        serverIP.text = "服务器 IP : " + ip;
        serverPort.text = "服务器 端口 : " + port;
    }

    function addMessage(msg)
    {
        messageText.text += "\n" + msg;
    }

    function addNewClient(username, ip)
    {
        usersArray.push(username);
        myModel.append({ "ip": ip,
                         "username": username,
                         "nickname": "MPS",
                         "state": "在线"});
        addMessage(username + "已经连接...");
    }

    function removeClient(client)
    {
        var index = usersArray.indexOf(client);
        if (index != -1)
        {
            usersArray.splice(index, 1);
            myModel.remove(index);
        }
    }

    /*Image
    {
        id: background
        source: "qrc:/image/9.jpg"
    }*/

    Rectangle
    {
        anchors.fill: parent
        radius: consoleWindow.radius
        Rectangle
        {
            anchors.fill: parent
            radius: parent.radius
            gradient: Gradient
            {
                GradientStop
                {
                   position: 0.000
                   color: "#88BBEEFA"
                }
                GradientStop
                {
                   position: 0.500
                   color: "#8800EA75"
                }
                GradientStop
                {
                   position: 1.000
                   color: "#88BBEEFA"
                }
            }
        }
    }

    ResizeMouseArea
    {
        id: resizeMouseArea
        target: root
    }

    Row
    {
        width: 102
        height: 40
        anchors.right: parent.right
        anchors.rightMargin: 6
        anchors.top: parent.top
        anchors.topMargin: 6

        CusButton
        {
            id: menuButton
            width: 32
            height: 32

            onClicked:
            {
            }
            Component.onCompleted:
            {
                buttonNormalImage = "qrc:/image/ButtonImage/menu_normal.png";
                buttonPressedImage = "qrc:/image/ButtonImage/menu_down.png";
                buttonHoverImage = "qrc:/image/ButtonImage/menu_hover.png";
            }
        }

        CusButton
        {
            id: minButton
            width: 32
            height: 32

            onClicked:
            {
                root.showMinimized();
            }
            Component.onCompleted:
            {
                buttonNormalImage = "qrc:/image/ButtonImage/min_normal.png";
                buttonPressedImage = "qrc:/image/ButtonImage/min_down.png";
                buttonHoverImage = "qrc:/image/ButtonImage/min_hover.png";
            }
        }

        CusButton
        {
            id: closeButton
            width: 32
            height: 32

            onClicked:
            {
                root.close();
            }
            Component.onCompleted:
            {
                buttonNormalImage = "qrc:/image/ButtonImage/close_normal.png";
                buttonPressedImage = "qrc:/image/ButtonImage/close_down.png";
                buttonHoverImage = "qrc:/image/ButtonImage/close_hover.png";
            }
        }
    }

    ListModel
    {
        id: myModel
    }

    Row
    {
        id: serverRect
        anchors.horizontalCenter: parent.horizontalCenter
        width: 400
        height: 45

        Text
        {
            id: serverIP
            height: 45
            width: 200
            color: "red"
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            font.family: "微软雅黑"
            font.pointSize: 11
        }

        Text
        {
            id: serverPort
            height: 45
            width: 200
            color: "red"
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            font.family: "微软雅黑"
            font.pointSize: 11
        }
    }

    TableView
    {
        id: tableView
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: serverRect.bottom
        anchors.bottom: consoleWindow.top
        anchors.bottomMargin: 20
        model: myModel
        backgroundVisible: false
        headerDelegate: Component
        {
            id: headerDelegate

            Rectangle
            {
                id: headerRect;
                height: 30
                width: 100
                border.color: "#400040"
                color: styleData.selected ? "gray" : "transparent"
                radius: 3

                Text
                {
                    text: styleData.value
                    anchors.centerIn: parent
                    font.family: "微软雅黑"
                    font.pointSize: 10
                    color: "red"
                }
            }
        }
        rowDelegate: Component
        {
            id: rowDelegate
            Rectangle
            {
                color: "transparent"
                height: 42
            }
        }
        itemDelegate: Component
        {
            id: itemDelegate

            Rectangle
            {
                id: tableCell
                anchors.fill: parent
                anchors.topMargin: 4
                anchors.leftMargin: 1
                border.color: "#400040"
                radius: 3
                color: styleData.selected ? "#44EEEEEE" : "#66B5E61D"

                Text
                {
                    id: textID
                    text: styleData.value
                    font.family: "微软雅黑"
                    anchors.fill: parent
                    color: "black"
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }


        TableViewColumn
        {
            role: "username"
            title: "帐号"
            width: 150
        }

        TableViewColumn
        {
            role: "ip"
            title: "连接IP"
            width: 150
        }

        TableViewColumn
        {
            role: "nickname"
            title: "昵称"
            width: 150
        }

        TableViewColumn
        {
            role: "state"
            title: "状态"
            width: 100
        }
    }

    Rectangle
    {
        id: consoleWindow
        opacity: 0.9
        radius: 10
        clip: true
        height: 160
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        Flickable
        {
            id: flick
            //interactive: false
            anchors.fill: parent
            anchors.margins: 15
            contentHeight: messageText.contentHeight
            contentWidth: messageText.contentWidth

            TextEdit
            {
                id: messageText
                width: flick.width
                height: flick.height
                wrapMode: Text.WrapAnywhere
                font.family: "微软雅黑"
                //textFormat: Text.RichText
                text: "服务器运行中..."
                color: "#400040"
                onTextChanged: flick.contentY = Math.max(0, contentHeight - height);
            }

            ScrollBar.vertical: ScrollBar
            {
                width: 12
                policy: ScrollBar.AlwaysOn
            }
        }
    }
}
