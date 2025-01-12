#ifndef PLOT_H
#define PLOT_H

#include <QObject>
#include "QSplineSeries"

class plot : public QObject
{
    Q_OBJECT
public:
    explicit plot(QObject *parent = nullptr);
    QSplineSeries *series0,*series1,*series2;
    int *occupied;
    QList<QPointF> ps1,ps2,ps3;
signals:
    void seriesPrepered(QSplineSeries* series0,QSplineSeries* series1,QSplineSeries* series3);
public slots:
    void dataUpdate(int16_t *I, int16_t *Q, long int length, long long fs);
};

#endif // PLOT_H
