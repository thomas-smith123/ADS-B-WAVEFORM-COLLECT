#include "widget.h"
#include "ui_widget.h"
#include "QNetworkProxyFactory"
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include "sharedsource.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ad9361_start_flag = 0;
    sharedresource = new sharedsources;
    splitter1 = new QSplitter;
    splitter2 = new QSplitter;
    QWebEngineProfile::defaultProfile()->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);
    QWebEngineProfile::defaultProfile()->setHttpCacheMaximumSize(0);



    map = new QWebEngineView(this);
    QWebEngineSettings *settings = map->settings();
    settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, false);
    settings->setAttribute(QWebEngineSettings::WebGLEnabled, false);//false后ram占用减小很多
    // webView->setUrl(QUrl("qrc:/amap.html"));
    map->setUrl(QUrl("qrc:/leafletmap.html"));
    QNetworkProxyFactory::setUseSystemConfiguration(false);
    // QWebEngineSettings::WebAttribute::
    // QWebEngineSettings::setAttribute(QWebEngineSettings::WebAttribute::DeveloperExtrasEnabled, true);

    //     QWebEngineSettings::setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, false);
    // QWebEngineSettings::setAttribute(QWebEngineSettings::WebGLEnabled, false);

    // map = new QQuickWidget;
    // map->setSource(QUrl::fromLocalFile(QStringLiteral("./main.qml"))); // 设置地图的QML文件路径
    // map->setResizeMode(QQuickWidget::SizeRootObjectToView);
    // if (map->status() != QQuickWidget::Ready) {
    //     qWarning() << "QQuickWidget loading error:" << map->status();
    //     qWarning() << map->errors();
    // }
    // map = new QWidget;
    // map = QWidget::createWindowContainer(map_,this);

    overall_layout = new QGridLayout;
    Horizon_connect = new QHBoxLayout;
    Horizon_display = new QHBoxLayout;
    BW = new QLineEdit;
    BW->setFixedWidth(50);
    BW->setText("5");
    BW_label = new QLabel;
    BW_label->setText("BW(MHz):");
    fs = new QLineEdit;
    fs->setFixedWidth(50);
    fs->setText("10");
    fs_label = new QLabel;
    fs_label->setText("fs(MHz):");
    fc = new QLineEdit;
    fc->setFixedWidth(50);
    fc->setText("1092");
    fc_label = new QLabel;
    fc_label->setText("fc(MHz):");


    url = new QTextEdit;
    url_label = new QLabel;
    url_label->setText("url:");
    select = new QPushButton;
    select->setText("Connect");
    select->setEnabled(false);
    // url
    url->setText("ip:192.168.2.1");
    // log widget

    adsb_frame_log = new QTableWidget;

    adsb_frame_log_map[adsb_header::ICAO] = "ICAO";
    adsb_frame_log_map[adsb_header::DF] = "DF";
    adsb_frame_log_map[adsb_header::survive_time] = "LastSeen";
    adsb_frame_log_map[adsb_header::Altitude] = "Altitude";
    adsb_frame_log_map[adsb_header::Longitude] = "Longitude";
    adsb_frame_log_map[adsb_header::Latitude] = "Latitude";
    adsb_frame_log_map[adsb_header::Velocity] = "Velocity";
    adsb_frame_log_map[adsb_header::Time] = "Time";
    adsb_frame_log->setColumnCount(adsb_frame_log_map.count());
    QStringList headerText;
    for(char i=0;i<adsb_frame_log_map.count();i++)
        headerText.append(adsb_frame_log_map[i]);
    adsb_frame_log->setHorizontalHeaderLabels(headerText);
    adsb_frame_log->setRowCount(0);

    // adsb_frame_log->setItem()
    //adsb_frame_log->setHorizontalHeaderItem(0,)
    // layout
    connect(map, &QWebEngineView::loadFinished, this, &Widget::maploadFinished);
    url->setFixedHeight(20);
    BW->setFixedHeight(20);
    fs->setFixedHeight(20);
    fc->setFixedHeight(20);
    Horizon_connect->addWidget(url_label);
    Horizon_connect->addWidget(url);
    Horizon_connect->addWidget(BW_label);
    Horizon_connect->addWidget(BW);
    Horizon_connect->addWidget(fs_label);
    Horizon_connect->addWidget(fs);
    Horizon_connect->addWidget(fc_label);
    Horizon_connect->addWidget(fc);
    Horizon_connect->addWidget(select);


    Horizon_display->addWidget(splitter2);
    splitter2->addWidget(adsb_frame_log);
    splitter2->addWidget(map);

    overall_layout->addLayout(Horizon_connect,0,0,1,1);
    overall_layout->setHorizontalSpacing(10);
    overall_layout->addLayout(Horizon_display,1,0,6,1);
    overall_layout->setRowStretch(1,10);

    // signal && slot
    connect(this->select,SIGNAL(clicked()),this,SLOT(pop_window()));
    this->setLayout(overall_layout);
    ui->setupUi(this);
}

Widget::~Widget()
{
    delete ui;
}




void Widget::pop_window()
{

#ifndef simulate
    if (!ad9361_start_flag) //flag=0 stopped
    {
        QString url_content;
        url_content = url->toPlainText();
        QByteArray ba = url_content.toUtf8();
        QString BW_content = BW->text();
        QString fs_content = fs->text();
        QString fc_content = fc->text();

        // QMessageBox msgBox;
        // msgBox.setText("The document has been modified.");
        // msgBox.exec();

        ad9361 = new board_read(sharedresource);
        ad9361->ip = ba.data();
        ad9361->bw = BW_content.toFloat();
        ad9361->fs = fs_content.toFloat();
        ad9361->lo = fc_content.toFloat();
        ad9361->config();
        adsb_process = new process(sharedresource);
        adsb_process->fs = this->fs->text().toFloat()*1e6;
        read_thread = new QThread;
        process_thread = new QThread;
        ad9361->moveToThread(read_thread);
        adsb_process->moveToThread(process_thread);
        updateTimer = new QTimer(this);
        connect(updateTimer, &QTimer::timeout, this, &Widget::removeExpiredAircraft);
        updateTimer->start(10000); // 每 10 秒检查一次
        connect(this,SIGNAL(ad9361_read_start()),ad9361,SLOT(start_read()));
        connect(adsb_process,&process::process_done,this,&Widget::table_update);


        //采集完成后进行处理
        connect(adsb_process,&process::process_whole_done,ad9361,&board_read::reset_to_emmit,Qt::BlockingQueuedConnection);
        connect(ad9361,&board_read::read_onece_done,adsb_process,&process::do_process);//FIXME
        // connect(process_thread,&QThread::finished, adsb_process, &QObject::deleteLater);
        // 开启线程
        if (ad9361->config_flag)
        {
            process_thread->start();
            read_thread->start();
            ad9361->stop_ = false;
            emit ad9361_read_start();
            ad9361_start_flag = 1;
            ad9361->start_flag = 1;
            select->setText("Disconnect");
        }
    }
    else
    {   if(ad9361->config_flag && ad9361_start_flag)
        {
            ad9361->start_flag = 0;
            ad9361_start_flag = 0;
            ad9361->stop_ = true;

            read_thread->quit();
            process_thread->quit();

            read_thread->wait();
            // read_thread->exit();

            process_thread->wait();
            // process_thread->exit();

            ad9361->deleteLater();
            process_thread->deleteLater();
        }
        select->setText("Connect");
    }
#else
    if (!ad9361_start_flag) //flag=0 stopped
    {
        QString url_content;
        url_content = url->toPlainText();
        QByteArray ba = url_content.toUtf8();
        QString BW_content = BW->text();
        QString fs_content = fs->text();
        QString fc_content = fc->text();

        // QMessageBox msgBox;
        // msgBox.setText("The document has been modified.");
        // msgBox.exec();

        ad9361 = new board_read(sharedresource);
        ad9361->ip = ba.data();
        ad9361->bw = BW_content.toFloat();
        ad9361->fs = fs_content.toFloat();
        ad9361->lo = fc_content.toFloat();
        ad9361->config();
        adsb_process = new process(sharedresource);
        adsb_process->fs = this->fs->text().toFloat()*1e6;
        read_thread = new QThread;
        process_thread = new QThread;
        ad9361->moveToThread(read_thread);
        adsb_process->moveToThread(process_thread);
        updateTimer = new QTimer(this);
        connect(updateTimer, &QTimer::timeout, this, &Widget::removeExpiredAircraft);
        updateTimer->start(10000); // 每 10 秒检查一次
        connect(this,SIGNAL(ad9361_read_start()),ad9361,SLOT(start_read()));

        // connect(read_thread, &QThread::finished, ad9361, &QObject::deleteLater);
        connect(adsb_process,&process::process_done,this,&Widget::table_update);

        //采集完成后进行处理
        connect(adsb_process,&process::process_whole_done,ad9361,&board_read::reset_to_emmit,Qt::BlockingQueuedConnection);
        connect(ad9361,&board_read::read_onece_done,adsb_process,&process::do_process);//FIXME
        // connect(process_thread,&QThread::finished, adsb_process, &QObject::deleteLater);


        // 开启线程
        if (ad9361->config_flag)
        {
            process_thread->start();
            read_thread->start();
            ad9361->stop_ = false;
            emit ad9361_read_start();
            ad9361_start_flag = 1;
            ad9361->start_flag = 1;
            select->setText("Disconnect");
        }
    }
    else
    {   if(ad9361->config_flag && ad9361_start_flag)
        {
            ad9361->start_flag = 0;
            ad9361_start_flag = 0;
            ad9361->stop_ = true;

            read_thread->quit();
            process_thread->quit();

            read_thread->wait();
            // read_thread->exit();

            process_thread->wait();
            // process_thread->exit();

            ad9361->deleteLater();
            process_thread->deleteLater();
        }
        select->setText("Connect");
    }
#endif
}

void Widget::addOrUpdateAircraft(struct adsb_frame *adsb_frame)
{
    QDateTime now = QDateTime::currentDateTime();
    if (aircraftMap.contains(adsb_frame->ICAO)) {
        for(int i=0;i<adsb_frame_log->rowCount();i++)
        {
            if(adsb_frame_log->item(i,adsb_header::ICAO)->text() == adsb_frame->ICAO)
            {
                adsb_frame_log->setItem(i, adsb_header::survive_time, new QTableWidgetItem(now.toString()));
                if(adsb_frame->latitude != NULL && adsb_frame->logitude != NULL)
                {
                    //QString("%1").arg(num, 0, 'f', 2);
                    adsb_frame_log->setItem(i, adsb_header::Latitude, new QTableWidgetItem(QString::number(adsb_frame->latitude,10,5)));
                    adsb_frame_log->setItem(i, adsb_header::Longitude, new QTableWidgetItem(QString::number(adsb_frame->logitude,10,5)));
                }
                adsb_frame_log->setItem(i, adsb_header::DF, new QTableWidgetItem(adsb_frame->df));
                if(adsb_frame->velocity != NULL)
                    adsb_frame_log->setItem(i, adsb_header::Velocity, new QTableWidgetItem(QString::number(adsb_frame->velocity,10,5)));
                if(adsb_frame->altitude != NULL)
                    adsb_frame_log->setItem(i, adsb_header::Altitude, new QTableWidgetItem(QString::number(adsb_frame->altitude,10,5)));

                break;
            }
        }
        aircraftMap[adsb_frame->ICAO].lastSeen = now;
        if(adsb_frame->latitude != NULL && adsb_frame->logitude != NULL)
        {
            updateMarker(adsb_frame->ICAO,adsb_frame->logitude,adsb_frame->latitude,adsb_frame->heading);
        }

    }
    else
    {
        Aircraft newAircraft{adsb_frame->ICAO, now};
        aircraftMap.insert(adsb_frame->ICAO, newAircraft);

        int row = adsb_frame_log->rowCount();//这里可以试着用map
        adsb_frame_log->insertRow(row);
        adsb_frame_log->setItem(row, adsb_header::ICAO, new QTableWidgetItem(adsb_frame->ICAO));
        adsb_frame_log->setItem(row, adsb_header::survive_time, new QTableWidgetItem(now.toString()));
        if(adsb_frame->latitude != NULL && adsb_frame->logitude != NULL)
        {
            //QString("%1").arg(num, 0, 'f', 2);
            adsb_frame_log->setItem(row, adsb_header::Latitude, new QTableWidgetItem(QString::number(adsb_frame->latitude,10,5)));
            adsb_frame_log->setItem(row, adsb_header::Longitude, new QTableWidgetItem(QString::number(adsb_frame->logitude,10,5)));
            addCustomMarker(adsb_frame->ICAO,adsb_frame->logitude,adsb_frame->latitude,adsb_frame->heading);
        }
        adsb_frame_log->setItem(row, adsb_header::DF, new QTableWidgetItem(adsb_frame->df));
        if(adsb_frame->velocity != NULL)
            adsb_frame_log->setItem(row, adsb_header::Velocity, new QTableWidgetItem(QString::number(adsb_frame->velocity,10,5)));
        if(adsb_frame->altitude != NULL)
            adsb_frame_log->setItem(row, adsb_header::Altitude, new QTableWidgetItem(QString::number(adsb_frame->altitude,10,5)));
    }
}

void Widget::removeExpiredAircraft()
{
    QDateTime now = QDateTime::currentDateTime();
    int row = 0;

    for (auto it = aircraftMap.begin(); it != aircraftMap.end(); ) {
        if (it.value().lastSeen.secsTo(now) > 300) { // 超过 60 秒未更新
            adsb_frame_log->removeRow(row);
            it = aircraftMap.erase(it);
            --row;
        } else {
            adsb_frame_log->setItem(row, 1, new QTableWidgetItem(it.value().lastSeen.toString()));
            ++it;
            ++row;
        }
    }
}

void Widget::addCustomMarker(const QString &id, double lng, double lat, double angle) {
    QString script = QString(
                         "addCustomMarker('%1', %2, %3, %4);"
                         ).arg(id).arg(lng).arg(lat).arg(angle);
    map->page()->runJavaScript(script);
}

void Widget::updateMarker(const QString &id, double lng, double lat, double angle) {
    QString script = QString(
                         "updateMarker('%1', %2, %3, %4 );"
                         ).arg(id).arg(lng).arg(lat).arg(angle);
    map->page()->runJavaScript(script);
}
void Widget::removeMarker(const QString &id) {
    QString script = QString(
                         "removeMarker('%1');"
                         ).arg(id);
    map->page()->runJavaScript(script);
}
void Widget::table_update(struct adsb_frame *adsb_frame)
{
    addOrUpdateAircraft(adsb_frame);
}
void Widget::maploadFinished(bool success)
{
    if (success)
    {
        select->setEnabled(true);
        addCustomMarker("test", 116.4, 39.91, 0.0);
    }
}
