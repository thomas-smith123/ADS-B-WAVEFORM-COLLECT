#ifndef BOARD_READ_H
#define BOARD_READ_H

#include <QObject>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "bsp/iio/iio.h"
#include "QString"
#include "QMessageBox"
#include "QMutex"
#include "QtCore/QWaitCondition"
// #include "QwaitCondition"
#include "iostream"
#include "sharedsource.h"
// #include "overall_control.h"

/* helper macros */
#define MHZ(x) ((long long)(x*1000000.0 + .5))
#define GHZ(x) ((long long)(x*1000000000.0 + .5))

// #define IIO_ENSURE(expr) { \
// if (!(expr)) { \
//         QMessageBox::warning(NULL, "warning", "Config Failed", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);\
//         (void) fprintf(stderr, "assertion failed (%s:%d)\n", __FILE__, __LINE__); \
//         (void) abort(); \
// } \
// }

/* RX is input, TX is output */
enum iodev { RX, TX };

/* common RX and TX streaming params */
struct stream_cfg {
    long long bw_hz; // Analog banwidth in Hz
    long long fs_hz; // Baseband sample rate in Hz
    long long lo_hz; // Local oscillator frequency in Hz
    const char* rfport; // Port name
};

class board_read : public QObject
{
    Q_OBJECT
public:
    bool emit_signal_to_process;
    QMutex *iq_lock;
    double buffer_size;
    char start_flag;
    char config_flag; //0 fail, 1 success
    char *ip;
    float bw;
    float fs;
    float lo;
    char tmpstr[64];
    bool stop_;
    struct iio_context* ctx;
    struct iio_channel* rx0_i;
    struct iio_channel* rx0_q;
    struct iio_channel* tx0_i;
    struct iio_channel* tx0_q;
    struct iio_channel* rx1_i;
    struct iio_channel* rx1_q;
    struct iio_channel* tx1_i;
    struct iio_channel* tx1_q;
    struct iio_buffer* rxbuf;
    struct iio_buffer* txbuf;
    struct iio_device* tx;
    struct iio_device* rx;
    struct stream_cfg rxcfg;
    struct stream_cfg txcfg;
    size_t nrx = 0;
    size_t ntx = 0;
    int16_t *I0;
    int16_t *Q0;
    int16_t *I1;
    int16_t *Q1;
    void config(float bw,float fs,float lo);
    explicit board_read(sharedsource* sharedresource, QObject *parent = nullptr, double buffer_siz=1024*1024);
    ~board_read();
    ptrdiff_t p_inc;
    char *p_dat, *p_end;
    ssize_t nbytes_rx;

signals:
    void read_onece_done(int16_t *I, int16_t *Q, long int length, long long fs);

private:
    void IIO_ENSURE(int expr, QString reason);
    void shutdownDevice();
    void handle_sig(int sig);
    /* check return value of attr_write function */
    void errchk(int v, const char* what);

    /* write attribute: long long int */
    void wr_ch_lli(struct iio_channel* chn, const char* what, long long val);

    /* write attribute: string */
    void wr_ch_str(struct iio_channel* chn, const char* what, const char* str);

    /* helper function generating channel names */
    char* get_ch_name(const char* type, int id);

    /* returns ad9361 phy device */
    struct iio_device* get_ad9361_phy(struct iio_context* ctx);

    /* finds AD9361 streaming IIO devices */
    bool get_ad9361_stream_dev(struct iio_context* ctx, enum iodev d, struct iio_device** dev);

    /* finds AD9361 streaming IIO channels */
    bool get_ad9361_stream_ch(__notused struct iio_context* ctx, enum iodev d, struct iio_device* dev, int chid, struct iio_channel** chn);

    /* finds AD9361 phy IIO configuration channel with id chid */
    bool get_phy_chan(struct iio_context* ctx, enum iodev d, int chid, struct iio_channel** chn);

    /* finds AD9361 local oscillator IIO configuration channels */
    bool get_lo_chan(struct iio_context* ctx, enum iodev d, struct iio_channel** chn);

    /* applies streaming configuration through IIO */
    bool cfg_ad9361_streaming_ch(struct iio_context* ctx, struct stream_cfg* cfg, enum iodev type, int chid);

    sharedsource *sharedresources;

public slots:
    void reset_to_emmit();
    void start_read();
    // void stop();
};


//DWORD WINAPI ThreadProc(LPVOID lpParam) {
//	printf("Hello, World! Thread ID: %lu\n", GetCurrentThreadId());
//	return 0;
//}



#endif // BOARD_READ_H
