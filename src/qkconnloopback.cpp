#include "qkconnloopback.h"
#include "qkconnect_global.h"

#include <QDebug>

QkConnLoopback::QkConnLoopback(QObject *parent) :
    QkConn(parent)
{
}

bool QkConnLoopback::open()
{
    emit message(QKCONNECT_MESSAGE_INFO,
                 tr("Connection ready!"));
    _changeStatus(QkConn::Ready);
    return true;
}

void QkConnLoopback::close()
{

}

void QkConnLoopback::sendData(QByteArray data)
{
    emit dataIn(data);
}

void QkConnLoopback::sendPacket(QJsonDocument doc)
{
    emit packetIn(doc);
}
