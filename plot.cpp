#include "plot.h"
#include "math.h"
plot::plot(QObject *parent)
    : QObject{parent}
{
    occupied = new int(0);
    series0 = new QSplineSeries;
    series0->setUseOpenGL(true);
    series1 = new QSplineSeries;
    series1->setUseOpenGL(true);
    series2 = new QSplineSeries;
    series2->setUseOpenGL(true);
    QPen pen;
    pen.setStyle(Qt::DotLine);//Qt::SolidLine, Qt::DashLine, Qt::DotLine, Qt::DashDotLine
    pen.setWidth(2);
    pen.setColor(Qt::red);
    series0->setPen(pen);
    series1->setPen(pen);
    series2->setPen(pen);
}
void plot::dataUpdate(int16_t *I, int16_t *Q, long int length, long long fs)
{
    // if(*occupied==0)
    {
        // series0->clear();
        ps1.clear();
        ps2.clear();
        ps3.clear();
        int j,cnt=0;
        for(int i=0;i<length;i+=2000)
        {
            ps1.push_back(QPoint((qreal) i, I[i]));
            ps2.push_back(QPoint((qreal) i, Q[i]));
            // ps3.push_back(QPoint((qreal) i, (I[i]*I[i]+Q[i]*Q[i])));
        }
        series0->replace(ps1);
        series1->replace(ps2);
        // series2->replace(ps3);
        emit seriesPrepered(series0,series1,series2);
    }
}

