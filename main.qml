import QtQuick 2.15
import QtQuick.Controls 2.15
import QtLocation 5.15
import QtPositioning 5.15

Item {
    width: 640
    height: 480

    Map {
        id: map
        anchors.fill: parent
        plugin: Plugin {
            name: "osm"  // 使用 OpenStreetMap 插件
        }
        center: QtPositioning.coordinate(39.74, 116.4)  // 中心坐标，例如巴黎
        zoomLevel: 10  // 缩放级别
        // 标记模型，用于存储和显示标记
            // MapQuickItem {
            //     id: marker
            //     coordinate: QtPositioning.coordinate(39.90923, 116.397428)
            //     anchorPoint.x: marker.width / 2
            //     anchorPoint.y: marker.height
            //     sourceItem: Image {
            //         width: 32
            //         height: 32
            //         source: "qrc:/marker.png" // 图标文件的路径
            //     }
            // }

        // 鼠标区域，用于处理拖动事件
        MouseArea {
                id: mouseArea
                anchors.fill: parent
                drag.target: map
                property var lastPoint: Qt.point(0, 0)
                property bool dragging: false

                onPressed: {
                    lastPoint = map.toCoordinate(Qt.point(mouse.x, mouse.y))
                    dragging = true
                }
                onReleased: {
                    dragging = false
                }
                onPositionChanged: {
                    if (dragging) {
                        var currentPoint = map.toCoordinate(Qt.point(mouse.x, mouse.y))
                        var deltaLat = lastPoint.latitude - currentPoint.latitude
                        var deltaLon = lastPoint.longitude - currentPoint.longitude
                        map.center = QtPositioning.coordinate(map.center.latitude + deltaLat, map.center.longitude + deltaLon)
                        lastPoint = currentPoint
                    }
                }
                onWheel: {
                    if (wheel.angleDelta.y > 0) {
                        map.zoomLevel += 1
                    } else {
                        map.zoomLevel -= 1
                    }
                }
            }
        // 每个标记的定义
            MapQuickItem {
                id: mapMarker
                coordinate: QtPositioning.coordinate(0, 0) // 初始坐标
                sourceItem: Image {
                    width: 32
                    height: 32
                    source: "./plane.png" // 图标文件的路径
                    rotation: mapMarker.rotation // 应用旋转
                    transformOrigin: Item.Center // 旋转中心
                }
                visible: false
            }

                Component.onCompleted: {
                    // 通过 QML 方法添加标记
                    function addMarker(lat, lon, angle) {
                        var marker = mapMarker.clone(); // 克隆标记组件
                        marker.coordinate = QtPositioning.coordinate(lat, lon);
                        marker.rotation = angle; // 设置旋转角度
                        marker.visible = true;
                        map.addMapQuickItem(marker);
                    }
            }


    }
}
