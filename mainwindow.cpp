#include "mainwindow.h"
#include "QGeoPositionInfoSource"
#include "filewriter.h"
#include "QNetworkProxyFactory"
#include <QWebEngineSettings>
#include <QWebEngineProfile>
double pre_lat,pre_lon;

QMap<int,std::string> planeCategory={
    {1,"Surface_emergency_vehicle"},
    {2,"Surface_service_vehicle"},
    {3,"Ground_obstruction"},
    {4,"Glider_sailplane"},
    {5,"Lighter_than_air"},
    {6,"Parachutist_skydiver"},
    {7,"Ultralight_handglider_paraglider"},
    {8,"uav"},
    {9,"Space_or_transatmospheric_vehicle"},
    {10,"light"},
    {11,"medium1"},
    {12,"medium2"},
    {13,"High_vortex_aircraft"},
    {14,"heavy"},
    {15,"High_performance_and_high_speed"},
    {16,"Rotorcraft"},

    };

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    /* data from https://opensky-network.org/datasets/#metadata/*/
    db.setDatabaseName("C:/jiangrd3/ADS-B-WAVEFORM-COLLECT/aircraft.db");
    if (!db.open()) {
        qDebug() << "Error: Could not open database.";
    }
    query_OPERATORCallsign = QSqlQuery("C:/jiangrd3/ADS-B-WAVEFORM-COLLECT/aircraft.db");
    query_owner = QSqlQuery("C:/jiangrd3/ADS-B-WAVEFORM-COLLECT/aircraft.db");
    query_model = QSqlQuery("C:/jiangrd3/ADS-B-WAVEFORM-COLLECT/aircraft.db");

    centralWidget = new QWidget;
    gridLayout = new QGridLayout;
    ConfigLayout = new QHBoxLayout;
    statistic = new QGroupBox;
    tab = new QTabWidget;

    //control widget
    {
        url_label = new QLabel;
        url_label->setText("url:");
        url = new QLineEdit;
        url->setText("ip:193.168.1.5");
        BW = new QLineEdit;
        BW->setFixedWidth(50);
        BW->setText("5");
        BW_label = new QLabel;
        BW_label->setText("BW(MHz):");
        fs = new QLineEdit;
        fs->setFixedWidth(50);
        fs->setText("10");
        // fs->setDisabled(true);

        fs_label = new QLabel;
        fs_label->setText("fs(MHz):");
        fc = new QLineEdit;
        fc->setFixedWidth(50);
        fc->setText("1091");
        fc_label = new QLabel;
        fc_label->setText("fc(MHz):");
        numthread = new QSpinBox;
        numthread->setRange(1,15);
        numthread->setValue(5);
        numthread_label = new QLabel;
        numthread_label->setText("Threads:");
        url->setEnabled(false);
        fs->setEnabled(false);
        fc->setEnabled(false);
        BW->setEnabled(false);
        select = new QPushButton;
        select->setText("Connect");
        ConfigLayout->addWidget(url_label,0);
        ConfigLayout->addWidget(url,10);
        ConfigLayout->addWidget(fs_label,0);
        ConfigLayout->addWidget(fs,1);
        ConfigLayout->addWidget(fc_label,1);
        ConfigLayout->addWidget(fc,1);
        ConfigLayout->addWidget(BW_label,1);
        ConfigLayout->addWidget(BW,1);
        ConfigLayout->addWidget(numthread_label,1);
        ConfigLayout->addWidget(numthread,4);
        ConfigLayout->addWidget(select,1);
    }

    // tab
    {
        {
            QWebEngineProfile::defaultProfile()->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);
            QWebEngineProfile::defaultProfile()->setHttpCacheMaximumSize(0);
            map = new QWebEngineView(this);
            QWebEngineSettings *settings = map->settings();
            settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, false);
            settings->setAttribute(QWebEngineSettings::WebGLEnabled, false);//false后ram占用减小很多
            // webView->setUrl(QUrl("qrc:/amap.html"));
            map->setUrl(QUrl("qrc:/leafletmap.html"));
            QNetworkProxyFactory::setUseSystemConfiguration(false);
            MapWidget = new QWidget;
            MapLayout = new QGridLayout;
            MapLayout->addWidget(map);
            MapWidget->setLayout(MapLayout);
            tab->addTab(MapWidget,"Map");

        }
        {//table
            TableWidget = new QWidget;
            TableLayout = new QGridLayout;
            table = new QTableWidget;
            // table->setSortingEnabled(true);

            {
                adsb_frame_log_map[adsb_header::ICAO] = "ICAO";
                adsb_frame_log_map[adsb_header::OPERATOR] = "Operator";
                adsb_frame_log_map[adsb_header::OPERATORCallsign] = "OperatorCallsign";
                adsb_frame_log_map[adsb_header::model] = "Model";
                adsb_frame_log_map[adsb_header::DF] = "DF";
                adsb_frame_log_map[adsb_header::survive_time] = tr("LastSeen");
                adsb_frame_log_map[adsb_header::Altitude] = tr("Altitude");
                adsb_frame_log_map[adsb_header::Longitude] = tr("Longitude");
                adsb_frame_log_map[adsb_header::Latitude] = tr("Latitude");
                adsb_frame_log_map[adsb_header::Velocity] = tr("Velocity");
                // adsb_frame_log_map[adsb_header::Time] = "Time";
                adsb_frame_log_map[adsb_header::VS] = "Vertical status";
                adsb_frame_log_map[adsb_header::CA] = "Plane";
                adsb_frame_log_map[adsb_header::Message] = "Message";
                adsb_frame_log_map[adsb_header::CALLSIGN] = "CallSign";
                adsb_frame_log_map[adsb_header::CNT] = "Count";
                table->setColumnCount(adsb_frame_log_map.count());
                QStringList headerText;
                for(char i=0;i<adsb_frame_log_map.count();i++)
                    headerText.append(adsb_frame_log_map[i]);
                table->setHorizontalHeaderLabels(headerText);
                table->setRowCount(0);
            }
            TableLayout->addWidget(table);
            TableWidget->setLayout(TableLayout);
            tab->addTab(TableWidget ,"Tabel");
        }


        tab->addTab(TableWidget,"LogTable");

        {
            PlotWidget = new QWidget;
            PlotLayout = new QGridLayout;
            real_label = new QLabel("Real");
            imag_label = new QLabel("Imag");
            moulde_label = new QLabel("ABS");
            real = new QChartView;
            imag = new QChartView;
            abs = new QChartView;
            real_chart = new QChart;
            imag_chart = new QChart;
            abs_chart = new QChart;
            // series0 = new QSplineSeries;

            real_chart->setTitle("simple function curve");
            real->setChart(real_chart);
            real_chart->createDefaultAxes();
            imag_chart->setTitle("simple function curve");
            imag->setChart(imag_chart);
            imag_chart->createDefaultAxes();
            abs_chart->setTitle("simple function curve");
            abs->setChart(abs_chart);
            abs_chart->createDefaultAxes();

            // series0->append(0,0);
            // series0->append(1,1.560);
            // series0->append(2,1.630);
            // series0->append(3,-1.540);
            // series0->append(4,1.200);
            // series0->append(5,-1.350);
            // series0->append(6,1.290);
            // real_chart->addSeries(series0);

            PlotLayout->addWidget(real_label);
            PlotLayout->addWidget(real);
            PlotLayout->addWidget(imag_label);
            PlotLayout->addWidget(imag);
            PlotLayout->addWidget(moulde_label);
            PlotLayout->addWidget(abs);
            PlotWidget->setLayout(PlotLayout);
            tab->addTab(PlotWidget,"Plot");
        }

        {
            FrameLogWidget = new QWidget;
            FrameLogLayout = new QGridLayout;
            log = new QTextBrowser;
            df0= new QCheckBox;
            df4= new QCheckBox;
            df5= new QCheckBox;
            df11= new QCheckBox;
            df16= new QCheckBox;
            df17= new QCheckBox;
            df18= new QCheckBox;
            df19= new QCheckBox;
            df20= new QCheckBox;
            df21= new QCheckBox;
            df24= new QCheckBox;

            df0->setChecked(true);
            df4->setChecked(true);
            df5->setChecked(true);
            df11->setChecked(true);
            df16->setChecked(true);
            df17->setChecked(true);
            df18->setChecked(true);
            df19->setChecked(true);
            df20->setChecked(true);
            df21->setChecked(true);
            df24->setChecked(true);

            df0_label = new QLabel;
            df4_label = new QLabel;
            df5_label = new QLabel;
            df11_label = new QLabel;
            df16_label = new QLabel;
            df17_label = new QLabel;
            df18_label = new QLabel;
            df19_label = new QLabel;
            df20_label = new QLabel;
            df21_label = new QLabel;
            df24_label = new QLabel;

            df0_label -> setText("df0");
            df4_label -> setText("df4");
            df5_label -> setText("df5");
            df11_label -> setText("df11");
            df16_label -> setText("df16");
            df17_label -> setText("df17");
            df18_label -> setText("df18");
            df19_label -> setText("df19");
            df20_label -> setText("df20");
            df21_label -> setText("df21");
            df24_label -> setText("df24");

            FrameLogLayout->addWidget(df0,0,0,1,1);
            FrameLogLayout->addWidget(df0_label,0,1,1,1);

            FrameLogLayout->addWidget(df4,0,2,1,1);
            FrameLogLayout->addWidget(df4_label,0,3,1,1);
            FrameLogLayout->addWidget(df5,0,4,1,1);
            FrameLogLayout->addWidget(df5_label,0,5,1,1);
            FrameLogLayout->addWidget(df11,0,6,1,1);
            FrameLogLayout->addWidget(df11_label,0,7,1,1);
            FrameLogLayout->addWidget(df16,0,8,1,1);
            FrameLogLayout->addWidget(df16_label,0,9,1,1);
            FrameLogLayout->addWidget(df17,0,10,1,1);
            FrameLogLayout->addWidget(df17_label,0,11,1,1);
            FrameLogLayout->addWidget(df18,0,12,1,1);
            FrameLogLayout->addWidget(df18_label,0,13,1,1);
            FrameLogLayout->addWidget(df19,0,14,1,1);
            FrameLogLayout->addWidget(df19_label,0,15,1,1);
            FrameLogLayout->addWidget(df20,0,16,1,1);
            FrameLogLayout->addWidget(df20_label,0,17,1,1);
            FrameLogLayout->addWidget(df21,0,18,1,1);
            FrameLogLayout->addWidget(df21_label,0,19,1,1);
            FrameLogLayout->addWidget(df24,0,20,1,1);
            FrameLogLayout->addWidget(df24_label,0,21,1,1);

            FrameLogLayout->addWidget(log,1,0,10,41);


            log->setText("Hello World");
            FrameLogWidget->setLayout(FrameLogLayout);
            tab->addTab(FrameLogWidget,"FrameLog");

        }

        {//TX stimulate
            TXWidget = new QWidget;
            TXLayout = new QGridLayout;
            QLabel *velocity_TX_label = new QLabel("Velocity:");
            QLabel *lat_TX_label = new QLabel("Latitude:");
            QLabel *lon_TX_label = new QLabel("Longitude:");
            TXLayout->addWidget(velocity_TX_label);
            TXLayout->addWidget(lon_TX_label);
            TXLayout->addWidget(lat_TX_label);
            TXWidget->setLayout(TXLayout);
            tab->addTab(TXWidget,"TX");
        }

    }

    //statistic
    {
        StaticticLayout = new QHBoxLayout;
        FrameCount = new QLabel("Frame Count:");
        Count = new QLabel("0");
        clear = new QPushButton("Clear");
        StaticticLayout->addWidget(FrameCount,1);
        StaticticLayout->addWidget(Count,1);
        StaticticLayout->addWidget(clear,1);
        statistic->setLayout(StaticticLayout);
    }
    //layout
    gridLayout->addLayout(ConfigLayout,0,0,1,1);
    gridLayout->addWidget(tab,1,0,12,1);
    gridLayout->addWidget(statistic,13,0,1,1);
    // this->setLayout(gridLayout);
    centralWidget->setLayout(gridLayout);
    this->setCentralWidget(centralWidget);

    query_OPERATORCallsign.prepare("SELECT \"operatorCallsign\" FROM aircraft WHERE \"icao24\" = :id");
    query_owner.prepare("SELECT \"owner\" FROM aircraft WHERE \"icao24\" = :id");
    query_model.prepare("SELECT \"model\" FROM aircraft WHERE \"icao24\" = :id");
    signal_connect();
    //variables
    ad9361_started_flag=false;

}
MainWindow::~MainWindow() {}

void MainWindow::signal_connect()
{
    connect(map, &QWebEngineView::loadFinished, this, &MainWindow::maploadFinished);
    connect(select, &QPushButton::pressed, this, &MainWindow::onPushselect);
    connect(clear, &QPushButton::pressed, this, &MainWindow::clearStatistic);
    connect(df0,&QCheckBox::stateChanged,this,&MainWindow::onPushdf0);
    connect(df4,&QCheckBox::stateChanged,this,&MainWindow::onPushdf4);
    connect(df5,&QCheckBox::stateChanged,this,&MainWindow::onPushdf5);
    connect(df11,&QCheckBox::stateChanged,this,&MainWindow::onPushdf11);
    connect(df16,&QCheckBox::stateChanged,this,&MainWindow::onPushdf16);
    connect(df17,&QCheckBox::stateChanged,this,&MainWindow::onPushdf17);
    connect(df18,&QCheckBox::stateChanged,this,&MainWindow::onPushdf18);
    connect(df19,&QCheckBox::stateChanged,this,&MainWindow::onPushdf19);
    connect(df20,&QCheckBox::stateChanged,this,&MainWindow::onPushdf20);
    connect(df21,&QCheckBox::stateChanged,this,&MainWindow::onPushdf21);
    connect(df24,&QCheckBox::stateChanged,this,&MainWindow::onPushdf24);
}
//slot
void MainWindow::onPushselect()
{
    QGeoPositionInfo gpsPos;
    if(!ad9361_started_flag)
    {
        if (gpsPos.isValid())
        {
            pre_lat = gpsPos.coordinate().latitude();
            pre_lon = gpsPos.coordinate().longitude();
        }
        else
        {
            pre_lat = NULL;
            pre_lon = NULL;
        }
        select->setText("Disconnect");
        //
        sharedresource = new sharedsource;
        QString url_content;
        url_content = url->text();
        QByteArray ba = url_content.toUtf8();
        QString BW_content = BW->text();
        QString fs_content = fs->text();
        QString fc_content = fc->text();
        ad9361 = new board_read(sharedresource);
        ad9361->ip = ba.data();
        ad9361->bw = BW_content.toFloat();
        ad9361->fs = fs_content.toFloat();
        ad9361->lo = fc_content.toFloat();

        ad9361->config(ad9361->bw,ad9361->fs,ad9361->lo);
        QString currentDateTime = QDateTime::currentDateTime().toString("yyyyMMdd_hhmm");
        QString fileName = currentDateTime + ".csv";
        filewriter = new filewriter_(fileName);

        read_thread = new QThread;
        // process_thread = new QThread;
        plot_thread = new QThread;

        filewriter_thread = new QThread;
        filewriter->moveToThread(filewriter_thread);
        manager = new processManager(this,sharedresource,filewriter,numthread->value());//放filewriter可能有问题
        ad9361->moveToThread(read_thread);

        plot_ = new plot;
        plot_->moveToThread(plot_thread);
        occupied = plot_->occupied;
        updateTimer = new QTimer(this);

        connect(this,SIGNAL(ad9361_read_start()),ad9361,SLOT(start_read()));
        // connect(adsb_process,&adsb_decoder::process_done,this,&MainWindow::table_update);
        connect(updateTimer, &QTimer::timeout, this, &MainWindow::removeExpiredAircraft);
        // connect(updateTimer, &QTimer::timeout, adsb_process, &adsb_decoder::removeExpiredmap);
        updateTimer->start(300*1000); // 每 10 秒检查一次
        //采集完成后进行处理
        connect(ad9361,&board_read::read_onece_done,manager,&processManager::addNewTask);
        connect(ad9361,&board_read::read_onece_done,plot_,&plot::dataUpdate);//FIXME
        connect(plot_,&plot::seriesPrepered,this,&MainWindow::plotChart);

        // 开启线程
        if (ad9361->config_flag)
        {
            plot_thread->start();
            filewriter_thread->start();
            manager->startprocessing();
            read_thread->start();
            ad9361->stop_ = false;
            emit ad9361_read_start();
            ad9361_started_flag = true;
            ad9361->start_flag = 1;
            select->setText(tr("Disconnect"));
            url->setEnabled(false);
            fs->setEnabled(false);
            fc->setEnabled(false);
            BW->setEnabled(false);
            numthread->setEnabled(false);
            // qDebug()<<"simulate output";
            // QThread::msleep(10000);
            // manager->stopprocessing();
        }
    }
    else
    {
        // if(ad9361->config_flag )
        {
            ad9361->stop_ = true;

            ad9361->start_flag = 0;
            ad9361_started_flag = false;

            manager->stopprocessing();
            read_thread->quit();

            read_thread->wait();
            filewriter_thread->quit();

            filewriter_thread->wait();

            ad9361->deleteLater();
            filewriter_thread->deleteLater();

            // process_thread1->deleteLater();
        }
        select->setText(("Connect"));
        url->setEnabled(true);
        fs->setEnabled(true);
        fc->setEnabled(true);
        BW->setEnabled(true);
        numthread->setEnabled(true);

        delete (filewriter);
        // delete(adsb_process1);
        delete manager;
        delete(updateTimer);
        delete(sharedresource);
    }
}
void MainWindow::clearStatistic()
{
    Count->setText("0");
    log->clear();
}

void MainWindow::writeFramelog(struct ADSBFrame a)
{
    currentTime = QDateTime::currentDateTime();
    switch(a.df)
    {
    case 0:
        if(df0_value)
        {
            log->append(currentTime.toString("yyyy-MM-dd hh:mm:ss")+QString("  ")+QString::fromStdString(a.msg));
            Count->setText(QString::number(Count->text().toLong()+1));
        }
        break;
    case 4:
        if(df4_value)
        {
            log->append(currentTime.toString("yyyy-MM-dd hh:mm:ss")+QString("  ")+QString::fromStdString(a.msg));
            Count->setText(QString::number(Count->text().toLong()+1));
        }
        break;
    case 5:
        if(df5_value)
        {
            log->append(currentTime.toString("yyyy-MM-dd hh:mm:ss")+QString("  ")+QString::fromStdString(a.msg));
            Count->setText(QString::number(Count->text().toLong()+1));
        }
        break;
    case 11:
        if(df11_value)
        {
            log->append(currentTime.toString("yyyy-MM-dd hh:mm:ss")+QString("  ")+QString::fromStdString(a.msg));
            Count->setText(QString::number(Count->text().toLong()+1));
        }
        break;
    case 16:
        if(df16_value)
        {
            log->append(currentTime.toString("yyyy-MM-dd hh:mm:ss")+QString("  ")+QString::fromStdString(a.msg));
            Count->setText(QString::number(Count->text().toLong()+1));
        }
        break;
    case 17:
        if(df17_value)
        {
            log->append(currentTime.toString("yyyy-MM-dd hh:mm:ss")+QString("  ")+QString::fromStdString(a.msg));
            Count->setText(QString::number(Count->text().toLong()+1));
        }
        break;
    case 18:
        if(df18_value)
        {
            log->append(currentTime.toString("yyyy-MM-dd hh:mm:ss")+QString("  ")+QString::fromStdString(a.msg));
            Count->setText(QString::number(Count->text().toLong()+1));
        }
        break;
    case 19:
        if(df19_value)
        {
            log->append(currentTime.toString("yyyy-MM-dd hh:mm:ss")+QString("  ")+QString::fromStdString(a.msg));
            Count->setText(QString::number(Count->text().toLong()+1));
        }
        break;
    case 20:
        if(df20_value)
        {
            log->append(currentTime.toString("yyyy-MM-dd hh:mm:ss")+QString("  ")+QString::fromStdString(a.msg));
            Count->setText(QString::number(Count->text().toLong()+1));
        }
        break;
    case 21:
        if(df21_value)
        {
            log->append(currentTime.toString("yyyy-MM-dd hh:mm:ss")+QString("  ")+QString::fromStdString(a.msg));
            Count->setText(QString::number(Count->text().toLong()+1));
        }
        break;
    case 24:
        if(df24_value)
        {
            log->append(currentTime.toString("yyyy-MM-dd hh:mm:ss")+QString("  ")+QString::fromStdString(a.msg));
            Count->setText(QString::number(Count->text().toLong()+1));
        }
        break;
    }

}

void MainWindow::table_update(struct ADSBFrame adsb_frame)
{
    if(tablemap.contains(adsb_frame.ICAO))
    {
        for(int i=0;i<table->rowCount();i++)
        {
            if(table->item(i,adsb_header::ICAO)->text().toStdString() == adsb_frame.ICAO)
            {
                tmpItem = table->item(i,adsb_header::CNT);
                tmpItem->setText(QString::number(tmpItem->text().toFloat() + 1));

                tmpItem = table->item(i, adsb_header::Message);
                tmpItem->setText(QString::fromStdString( adsb_frame.msg));
                // table->setItem(i, adsb_header::Message, new QTableWidgetItem(QString::fromStdString( adsb_frame.msg)));
                //update
                int df = adsb_frame.df;

                tmpItem = table->item(i, adsb_header::survive_time);
                tmpItem->setText(adsb_frame.lastSeen.toString());
                tmpItem = table->item(i, adsb_header::DF);
                tmpItem->setText(QString::number(adsb_frame.df));
                // table->setItem(i, adsb_header::survive_time, new QTableWidgetItem(adsb_frame.lastSeen.toString()));
                // table->setItem(i, adsb_header::DF, new QTableWidgetItem(QString::number(adsb_frame.df)));
                switch (df) {
                case 0:

                    tmpItem = table->item(i, adsb_header::VS);
                    if (tmpItem)
                        tmpItem->setText(adsb_frame.vs==0?QString("Airborne"):QString("Ground"));
                    else
                        table->setItem(i, adsb_header::VS, new QTableWidgetItem(adsb_frame.vs==0?QString("Airborne"):QString("Ground")));
                    tmpItem = table->item(i, adsb_header::Altitude);
                    if (tmpItem)
                        tmpItem->setText(QString::number(adsb_frame.alt,'f', 2));
                    else
                        table->setItem(i, adsb_header::Altitude, new QTableWidgetItem(QString::number(adsb_frame.alt,'f', 2)));
                    break;
                case 4:

                    tmpItem = table->item(i, adsb_header::Altitude);
                    if (tmpItem)
                        tmpItem->setText(QString::number(adsb_frame.alt,'f', 2));
                    else
                        table->setItem(i, adsb_header::Altitude, new QTableWidgetItem(QString::number(adsb_frame.alt,'f', 2)));
                    //Utility Message
                    break;
                case 5:
                    tmpItem = table->item(i, adsb_header::Altitude);
                    if (tmpItem)
                        tmpItem->setText(QString::number(adsb_frame.alt,'f', 2));
                    else
                        table->setItem(i, adsb_header::Altitude, new QTableWidgetItem(QString::number(adsb_frame.alt,'f', 2)));

                    break;
                case 11:
                    tmpItem = table->item(i, adsb_header::CA);
                    if (tmpItem)
                        tmpItem->setText(QString::fromStdString(planeCategory[adsb_frame.category]));
                    else
                        table->setItem(i, adsb_header::CA, new QTableWidgetItem(QString::fromStdString(planeCategory[adsb_frame.category])));
                    break;
                case 16:
                    tmpItem = table->item(i, adsb_header::Altitude);
                    if (tmpItem)
                        tmpItem->setText(QString::number(adsb_frame.alt,'f', 2));
                    else
                        table->setItem(i, adsb_header::Altitude, new QTableWidgetItem(QString::number(adsb_frame.alt,'f', 2)));
                    tmpItem = table->item(i, adsb_header::VS);
                    if (tmpItem)
                        tmpItem->setText(adsb_frame.vs==0?QString("Airborne"):QString("Ground"));
                    else
                        table->setItem(i, adsb_header::VS, new QTableWidgetItem(adsb_frame.vs==0?QString("Airborne"):QString("Ground")));
                    break;
                case 17:
                    if(adsb_frame.tc <=4 && adsb_frame.tc>=1)
                    {
                        tmpItem = table->item(i, adsb_header::CALLSIGN);
                        if (tmpItem)
                            tmpItem->setText(QString::fromStdString(adsb_frame.callsign));
                        else
                            table->setItem(i, adsb_header::CALLSIGN, new QTableWidgetItem(QString::fromStdString(adsb_frame.callsign)));
                    }
                    tmpItem = table->item(i, adsb_header::CA);
                    if (tmpItem)
                        tmpItem->setText(QString::fromStdString(planeCategory[adsb_frame.category]));
                    else
                        table->setItem(i, adsb_header::CA, new QTableWidgetItem(QString::fromStdString(planeCategory[adsb_frame.category])));
                    break;
                default:
                    break;
                }
                tmpItem = table->item(i, adsb_header::Velocity);
                if (tmpItem)
                    tmpItem->setText(QString::number(adsb_frame.velocity,'f', 2));
                else
                    table->setItem(i, adsb_header::Velocity, new QTableWidgetItem(QString::number(adsb_frame.velocity,'f', 2)));
                if(adsb_frame.lat != 999 && adsb_frame.lon != 999)
                {
                    tmpItem = table->item(i, adsb_header::Longitude);
                    if (tmpItem)
                        tmpItem->setText(QString::number(adsb_frame.lon));
                    else
                        table->setItem(i, adsb_header::Longitude, new QTableWidgetItem(QString::number(adsb_frame.lon,'f', 4)));
                    tmpItem = table->item(i, adsb_header::Latitude);
                    if (tmpItem)
                        tmpItem->setText(QString::number(adsb_frame.lat));
                    else
                        table->setItem(i, adsb_header::Latitude, new QTableWidgetItem(QString::number(adsb_frame.lat,'f', 4)));

                    if (aircraftMap.contains(adsb_frame.ICAO))
                        updateMarker(QString::fromStdString(adsb_frame.ICAO),adsb_frame.lon,adsb_frame.lat,adsb_frame.heading);
                    else
                    {
                        addCustomMarker(QString::fromStdString(adsb_frame.ICAO),adsb_frame.lon,adsb_frame.lat,adsb_frame.heading);
                        aircraftMap.insert(adsb_frame.ICAO,0);
                    }
                }
                tmpItem = table->item(i, adsb_header::Altitude);
                if (tmpItem)
                    tmpItem->setText(QString::number(adsb_frame.alt,'f', 2));
                else
                    table->setItem(i, adsb_header::Altitude, new QTableWidgetItem(QString::number(adsb_frame.alt,'f', 4)));
                //
                break;
            }
        }
    }
    else
    {
        int row = table->rowCount();//这里可以试着用map
        table->insertRow(row);
        tablemap.insert(adsb_frame.ICAO, adsb_frame);
        query_owner.bindValue(":id", "'"+QString::fromStdString(adsb_frame.ICAO)+"'");
        query_OPERATORCallsign.bindValue(":id", "'"+QString::fromStdString(adsb_frame.ICAO)+"'");
        query_model.bindValue(":id", "'"+QString::fromStdString(adsb_frame.ICAO)+"'");
        if (query_owner.exec() && query_owner.next()) {
            QString airline = query_owner.value(0).toString();
            // qDebug() << "Airline:" << airline;
            table->setItem(row, adsb_header::OPERATOR,new QTableWidgetItem(airline));
        }
        if (query_model.exec() && query_model.next()) {
            QString airline = query_model.value(0).toString();
            // qDebug() << "Manu:" << airline;
            table->setItem(row, adsb_header::model,new QTableWidgetItem(airline));
        }
        if (query_OPERATORCallsign.exec() && query_OPERATORCallsign.next()) {
            QString airline = query_OPERATORCallsign.value(0).toString();
            // qDebug() << "Manu:" << airline;
            table->setItem(row, adsb_header::OPERATORCallsign,new QTableWidgetItem(airline));
        }



        table->setItem(row, adsb_header::Message, new QTableWidgetItem(QString::fromStdString( adsb_frame.msg)));
        table->setItem(row, adsb_header::ICAO, new QTableWidgetItem(QString::fromStdString(adsb_frame.ICAO)));
        table->setItem(row, adsb_header::CNT, new QTableWidgetItem(QString::number(1)));
        table->setItem(row, adsb_header::survive_time, new QTableWidgetItem(adsb_frame.lastSeen.toString()));
        int df = adsb_frame.df;
        table->setItem(row, adsb_header::DF, new QTableWidgetItem(QString::number(adsb_frame.df)));
        switch (df) {
        case 0:
            table->setItem(row, adsb_header::VS, new QTableWidgetItem(adsb_frame.vs==0?QString("Airborne"):QString("Ground")));
            table->setItem(row, adsb_header::Altitude, new QTableWidgetItem(QString::number(adsb_frame.alt,'f', 2)));
            break;
        case 4:
            table->setItem(row, adsb_header::Altitude, new QTableWidgetItem(QString::number(adsb_frame.alt,'f', 2)));
            //Utility Message
            break;
        case 5:

            table->setItem(row, adsb_header::Altitude, new QTableWidgetItem(QString::number(adsb_frame.alt,'f', 2)));
            break;
        case 11:
            table->setItem(row, adsb_header::CA, new QTableWidgetItem(QString::fromStdString(planeCategory[adsb_frame.category])));
            break;
        case 16:
            table->setItem(row, adsb_header::Altitude, new QTableWidgetItem(QString::number(adsb_frame.alt)));
            table->setItem(row, adsb_header::VS, new QTableWidgetItem(adsb_frame.vs==0?QString("Airborne"):QString("Ground")));
            break;
        case 17:
            table->setItem(row, adsb_header::CA, new QTableWidgetItem(QString::fromStdString(planeCategory[adsb_frame.category])));
            break;
        case 18:
            table->setItem(row, adsb_header::CA, new QTableWidgetItem(QString::fromStdString(planeCategory[adsb_frame.category])));
            break;
        default:
            break;
        }
        if(adsb_frame.lat != 999 && adsb_frame.lon != 999)
        {
            //QString("%1").arg(num, 0, 'f', 2);
            table->setItem(row, adsb_header::Latitude, new QTableWidgetItem(QString::number(adsb_frame.lat,'f', 4)));
            table->setItem(row, adsb_header::Longitude, new QTableWidgetItem(QString::number(adsb_frame.lon,'f', 4)));
            addCustomMarker(QString::fromStdString(adsb_frame.ICAO),adsb_frame.lon,adsb_frame.lat,adsb_frame.heading);
            aircraftMap.insert(adsb_frame.ICAO,0);
            // addCustomMarker(adsb_frame->ICAO,adsb_frame->lon,adsb_frame->lat,adsb_frame->heading);
        }
        if(adsb_frame.velocity != NULL)
            table->setItem(row, adsb_header::Velocity, new QTableWidgetItem(QString::number(adsb_frame.velocity,10,5)));
        if(adsb_frame.alt != NULL)
            table->setItem(row, adsb_header::Altitude, new QTableWidgetItem(QString::number(adsb_frame.alt,10,5)));
    }
}
void MainWindow::removeExpiredAircraft(void)
{
    // 获取当前时间
    // updateTimer.
    std::string ddd;
    QDateTime currentTime = QDateTime::currentDateTime();

    // 遍历表的所有行（从最后一行开始，以避免删除行导致的索引变化）
    for (int row = table->rowCount() - 1; row >= 0; --row) {
        // 获取第二列的时间（假设格式为 "yyyy-MM-dd HH:mm:ss"）
        QTableWidgetItem *item = table->item(row, adsb_header::survive_time); // 第二列的索引为1
        QTableWidgetItem *icao = table->item(row, adsb_header::ICAO);
        if (item && icao) {
            QDateTime itemTime = QDateTime::fromString(item->text());

            // 检查转换是否成功，并且比较时间差
            if (itemTime.isValid() && itemTime.secsTo(currentTime) > 240) { // 300秒 = 5分钟
                ddd = icao->text().toStdString();
                tablemap.remove((icao->text()).toStdString());
                manager->buffer.remove((icao->text()).toStdString());
                // adsb_process->buff.remove((icao->text()).toStdString());
                if (aircraftMap.contains(icao->text().toStdString()))
                {
                    QString icaoText = icao->text(); // 保存ICAO标识符

                    // 先从缓存和地图中删除相关数据
                    // adsb_process->buff.remove(icaoText.toStdString());
                    if (aircraftMap.contains(icaoText.toStdString()))
                    {
                        aircraftMap.remove(icaoText.toStdString());
                        removeMarker(icao->text());
                    }
                    // aircraftMap.remove(icao->text().toStdString());
                    // removeMarker(icao->text());
                }
                // adsb_process1->buff.remove((icao->text()).toStdString());
                // for (int col = 0; col < table->columnCount(); ++col) {
                //     QTableWidgetItem* item = table->takeItem(row, col); // 获取并移除单元格中的item
                //     delete item; // 释放item的内存
                // }
                table->removeRow(row);
                // adsb_process->buff.remove();
            }
        }
    }
    // adsb_process->out->flush();
}
void MainWindow::plotChart(QSplineSeries *a,QSplineSeries *b,QSplineSeries *c)
{
    // *occupied =1;
    // series0->clear();
    // real_chart->removeAllSeries();
    // real_chart->addSeries(a);
    // real_chart->removeSeries(a);
    real_chart->removeSeries(a);
    imag_chart->removeSeries(b);
    abs_chart->removeSeries(c);
    real_chart->addSeries(a);
    imag_chart->addSeries(b);
    abs_chart->addSeries(c);
    // *occupied = 0;
}

void MainWindow::onPushdf0(){
    df0_value = df0->isChecked()?1:0;
};
void MainWindow::onPushdf4(){
    df4_value = df4->isChecked()?1:0;
};
void MainWindow::onPushdf5(){
    df5_value = df5->isChecked()?1:0;
};
void MainWindow::onPushdf11(){
    df11_value = df11->isChecked()?1:0;
};
void MainWindow::onPushdf16(){
    df16_value = df16->isChecked()?1:0;
};
void MainWindow::onPushdf17(){
    df17_value = df17->isChecked()?1:0;
};
void MainWindow::onPushdf18(){
    df18_value = df18->isChecked()?1:0;
};
void MainWindow::onPushdf19(){
    df19_value = df19->isChecked()?1:0;
};
void MainWindow::onPushdf20(){
    df20_value = df20->isChecked()?1:0;
};
void MainWindow::onPushdf21(){
    df21_value = df21->isChecked()?1:0;
};
void MainWindow::onPushdf24(){
    df24_value = df24->isChecked()?1:0;
};
void MainWindow::addCustomMarker(const QString &id, double lng, double lat, double angle) {
    QString script = QString(
                         "addCustomMarker('%1', %2, %3, %4);"
                         ).arg(id).arg(lng).arg(lat).arg(angle);
    map->page()->runJavaScript(script);
}

void MainWindow::updateMarker(const QString &id, double lng, double lat, double angle) {
    QString script = QString(
                         "updateMarker('%1', %2, %3, %4 );"
                         ).arg(id).arg(lng).arg(lat).arg(angle);
    map->page()->runJavaScript(script);
}
void MainWindow::removeMarker(const QString &id) {
    QString script = QString(
                         "removeMarker('%1');"
                         ).arg(id);
    map->page()->runJavaScript(script);
}
void MainWindow::initMap() {
    QString script = QString(
                         "initMap();"
        );
    map->page()->runJavaScript(script);
}
void MainWindow::maploadFinished(bool success)
{
    if (success)
    {
        select->setEnabled(true);
        addCustomMarker("test", 116.4, 39.91, 0.0);
    }
}
