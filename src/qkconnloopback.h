#ifndef QKCONNLOOPBACK_H
#define QKCONNLOOPBACK_H

#include "qkconn.h"

class QkConnLoopback : public QkConn
{
    Q_OBJECT
public:
    explicit QkConnLoopback(QObject *parent = 0);

signals:


public slots:
    bool open();
    void close();
    void sendData(QByteArray data);
    void sendPacket(QJsonDocument doc);


};

#endif // QKCONNLOOPBACK_H
