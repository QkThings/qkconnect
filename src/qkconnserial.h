#ifndef QKCONNSERIAL_H
#define QKCONNSERIAL_H

#include "qkconn.h"
#include <QObject>

class QSerialPort;
class QkProtocolSerial;

class QkConnSerial : public QkConn
{
    Q_OBJECT
public:
    explicit QkConnSerial(const Descriptor &desc, QObject *parent = 0);

    static void listAvailable();

signals:

public slots:
    bool open();
    void close();
    void sendData(QByteArray data);
    void sendPacket(QJsonDocument doc);

private slots:
    void _slotReadyRead();

private:
    QkProtocolSerial *_protocol;
    QSerialPort *_sp;
    QString _portName;
    int _baudRate;
    bool _dtr;
};

#endif // QKCONNSERIAL_H
