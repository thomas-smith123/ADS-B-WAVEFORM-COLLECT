#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QString>
#include <QMutex>
#include "QDebug"
// class filewriter_ : public QObject {
//     Q_OBJECT

// public:
//     explicit filewriter_(const QString &filePath, QObject *parent = nullptr)
//     {
//         QFile file(filePath);
//         stream = new QTextStream;
//         // QTextStream stream(&file);
//         if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
//             stream->setDevice(&file);
//         } else {
//             // qWarning() << "Failed to open file:" << filePath;
//             qDebug()<<"file open error";
//             delete stream;
//         }
//     }

//     ~filewriter_() {
//         file->close();
//     }

// public slots:
//     void writeBuffer(const QString &buffer) ;

// private:
//     QFile *file;
//     QTextStream *stream;
//     QMutex mutex; // 用于线程安全
// };
class filewriter_ : public QObject {
    Q_OBJECT

public:
    explicit filewriter_(const QString &filePath, QObject *parent = nullptr)
        : QObject(parent), file(filePath) {
        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
            stream.setDevice(&file);
        } else {
            qWarning() << "Failed to open file:" << filePath;
        }
    }

    ~filewriter_() {
        file.close();
    }

public slots:
    void writeBuffer(const QString &buffer) {
        QMutexLocker locker(&mutex); // 线程安全
        stream << buffer;
        stream.flush(); // 确保数据写入文件
    }

private:
    QFile file;
    QTextStream stream;
    QMutex mutex; // 用于线程安全
};
