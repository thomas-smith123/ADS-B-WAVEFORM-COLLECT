#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "QGridLayout"
#include "QHBoxLayout"
#include "QGroupBox"
#include "QTabWidget"
#include "QLabel"
#include "QLineEdit"
#include "QPushButton"
#include "QTableWidget"
#include "QTextBrowser"
#include "QtCharts/QChartView"
#include "board_read.h"
#include <QThread>
#include "sharedsource.h"
#include "QTimer"
#include <QDateTime>
#include "QSplineSeries"
#include "plot.h"
#include "QSpinBox"
#include "process_.h"
#include "QCheckBox"
#include <QQuickWidget>
#include <QtWebEngineWidgets/QWebEngineView>
// #include "filewriter.h"
#include "processmanager.h"
#include "predefine.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include "process_.h"
    
    class filewriter_;
class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QWidget *centralWidget, *MapWidget, *PlotWidget, *TableWidget, *FrameLogWidget, *TXWidget;
    //layout
    QGridLayout *gridLayout, *MapLayout, *PlotLayout, *TableLayout, *FrameLogLayout, *TXLayout;
    QHBoxLayout *ConfigLayout, *StaticticLayout;
    QGroupBox *statistic;
    QTabWidget *tab;
    //widget
    QLabel *url_label, *fs_label, *fc_label, *BW_label, *real_label, *imag_label, *moulde_label, *numthread_label;
    QLabel *FrameCount, *Count;
    QLineEdit *url, *fs, *fc, *BW;
    QSpinBox *numthread;
    QPushButton *select, *clear;
    QTableWidget *table;
    QTextBrowser *log;
    //tab
    QWidget *tableWidget;
    QMap<int,QString> adsb_frame_log_map;
    QChartView *real, *imag, *abs;
    QChart *real_chart,*imag_chart,*abs_chart;
    QSplineSeries *series0;
    plot *plot_;
    int *occupied;
    QMap<std::string, struct ADSBFrame> buff;
    QLabel *df0_label,*df4_label,*df5_label,*df11_label,*df16_label,*df17_label,*df18_label,*df19_label,*df20_label,*df21_label,*df24_label;
    QCheckBox *df0,*df4,*df5,*df11,*df16,*df17,*df18,*df19,*df20,*df21,*df24;
    int df0_value=1,df4_value=1,df5_value=1,df11_value=1,df16_value=1,df17_value=1,df18_value=1,df19_value=1,df20_value=1,df21_value=1,df24_value=1;
    
    void signal_connect();
    
    //variables
    bool ad9361_started_flag;
    
    board_read *ad9361;
    adsb_decoder *adsb_process;
    QThread *read_thread, *process_thread, *plot_thread, *filewriter_thread;
    sharedsource *sharedresource;
    filewriter_ *filewriter;
    processManager *manager;
    
    QTableWidgetItem *tmpItem;
    QSqlDatabase db;
    QSqlQuery query_OOPERATORCallsign,query_operator,query_model;

signals:
    void ad9361_read_start();
    void file_selected(QString FilePath);
    
public slots:
    void onPushselect();
    void clearStatistic();
    void writeFramelog(struct ADSBFrame a);
    void removeExpiredAircraft(void);
    void table_update(struct ADSBFrame a);
    void plotChart(QSplineSeries *a,QSplineSeries *b,QSplineSeries *c);
    void addCustomMarker(const QString &id, double lng, double lat, double angle);
    void updateMarker(const QString &id, double lng, double lat, double angle);
    void removeMarker(const QString &id);
    void maploadFinished(bool success);
    void initMap();
    void onPushdf0();
    void onPushdf4();
    void onPushdf5();
    void onPushdf11();
    void onPushdf16();
    void onPushdf17();
    void onPushdf18();
    void onPushdf19();
    void onPushdf20();
    void onPushdf21();
    void onPushdf24();
private:
    QTimer *updateTimer;
    QDateTime currentTime;
    QWebEngineView *map;
    QMap<std::string, int> aircraftMap;
    QMap<std::string, struct ADSBFrame> tablemap;
};
enum adsb_header{
    ICAO=0,
    DF,
    OPERATOR,
    OOPERATORCallsign,
    model,
    survive_time,
    Altitude,
    Longitude,
    Latitude,
    Velocity,
    // Time,
    VS,
    CA,
    Message,
    CALLSIGN,
    CNT,
};

#endif // MAINWINDOW_H
