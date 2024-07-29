#ifndef WIDGET_H
#define WIDGET_H

#include <stdint.h>
#include <QWidget>
#include <QLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QLineEdit>
#include <QTableWidget>
#include <QSplitter>
#include <QMessageBox>
#include <QLabel>
#include <board_read.h>
#include <QByteArray>
#include <QThread>
#include "board_read.h"
#include "process.h"
// #include "QtLocation"
#include <QQuickWidget>
#include <QtWebEngineWidgets/QWebEngineView>
#include "QTimer"
#include "sharedsource.h"
#include "overall_control.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

enum adsb_header{
    ICAO=0,
    survive_time,
    Altitude,
    Longitude,
    DF,
    Latitude,
    Velocity,
    Time,
};
struct Aircraft {
    QString icao;
    QDateTime lastSeen;
};
class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);

    ~Widget();
    // splitter-->--
    QPushButton *select;

    QTextEdit *url;
    QLabel *url_label;
    QLineEdit *BW;
    QLabel *BW_label;
    QLineEdit *fs;
    QLabel *fs_label;
    QLineEdit *fc;
    QLabel *fc_label;

    // QWidget *map;


    // QTextEdit *map; //temporary occupied

    QTableWidget *adsb_frame_log;
    QSplitter *splitter1,*splitter2;

    //QtWebEngineWidgetUI *map;

    QGridLayout *overall_layout;
    QHBoxLayout *Horizon_display;
    QHBoxLayout *Horizon_connect;
    QVBoxLayout *v_layout;

    board_read *ad9361;
    process *adsb_process;
    QThread *read_thread, *process_thread;

    char ad9361_start_flag;


    QTableWidgetItem *icao;

    void addOrUpdateAircraft(struct adsb_frame *adsb_frame);
    void removeExpiredAircraft();

    sharedsources *sharedresource;
signals:
    void ad9361_read_start();

public slots:
    void pop_window();
    void table_update(struct adsb_frame *adsb_frame);
    void maploadFinished(bool);

private:
    // QQuickWidget *map;
    QTimer *updateTimer;
    QMap<QString, Aircraft> aircraftMap;
    QMap<QString,int> adsb_table_map;
    QMap<int,QString> adsb_frame_log_map;
    void addCustomMarker(const QString &id, double lng, double lat, double angle);
    void updateMarker(const QString &id, double lng, double lat, double angle);
    void removeMarker(const QString &id);
    QWebEngineView *map;
    Ui::Widget *ui;
};
#endif // WIDGET_H
