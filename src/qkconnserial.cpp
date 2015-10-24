#include "qkconnserial.h"
#include "qkconnect_global.h"
#include "qkprotocolserial.h"

#include <QDebug>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QEventLoop>

QkConnSerial::QkConnSerial(const Descriptor &desc, QObject *parent) :
    QkConn(parent)
{
    _desc = desc;
    _dtr = false;
    _sp = new QSerialPort(this);
    _protocol = new QkProtocolSerial(this);
    connect(_protocol, SIGNAL(serialOut(QByteArray)),
            this, SLOT(sendData(QByteArray)));
    connect(_protocol, SIGNAL(packetGenerated(QJsonDocument)),
            this, SIGNAL(packetIn(QJsonDocument)));
}

void QkConnSerial::listAvailable()
{
    qDebug("* Available serial ports:");
    qDebug("* Name       PID  VID  Description");
    foreach(QSerialPortInfo info, QSerialPortInfo::availablePorts())
    {

        if(!info.portName().contains("ACM") &&
           !info.portName().contains("USB") &&
           !info.portName().contains("COM"))
            continue;

        qDebug("  %-10s %04X %04X %s",
               info.portName().toStdString().c_str(),
               info.productIdentifier(),
               info.vendorIdentifier(),
               info.description().toStdString().c_str());
    }
}

bool QkConnSerial::open()
{
    connect(_sp, SIGNAL(readyRead()), this, SLOT(_slotReadyRead()));

    QString portName = _desc.params["portname"].toString();
    int baudRate = _desc.params["baudrate"].toInt();
    bool dtr = _desc.params["dtr"].toBool();

    _dtr = dtr;
    _sp->setPortName(portName);
    _sp->setBaudRate(baudRate);

    if(_sp->open(QIODevice::ReadWrite))
    {
        _sp->setBaudRate(baudRate);
        _sp->setParity(QSerialPort::NoParity);
        _sp->setFlowControl(QSerialPort::NoFlowControl);
        _sp->setDataBits(QSerialPort::Data8);

        if(_dtr == true)
            _sp->setDataTerminalReady(false);
        else
            _sp->setDataTerminalReady(true);

        _sp->clear();

        emit message(QKCONNECT_MESSAGE_INFO,
                     tr("Connection ready!"));
        _changeStatus(QkConn::Ready);
        return true;
    }
    else
    {
        emit message(QKCONNECT_MESSAGE_ERROR,
                     tr("Connection failed: ") + QString().sprintf("%s. %s", portName.toStdString().c_str(),
                                                                                     _sp->errorString().toStdString().c_str()));
        _changeStatus(QkConn::FailedToOpen);
        return false;
    }
}

void QkConnSerial::close()
{
    if(_sp->isOpen())
        _sp->close();
}

void QkConnSerial::sendData(QByteArray data)
{
    _sp->write(data);
}

void QkConnSerial::sendPacket(QJsonDocument doc)
{
    qDebug() << __PRETTY_FUNCTION__;
    _protocol->translatePacket(doc);
}

void QkConnSerial::_slotReadyRead()
{
    QByteArray data = _sp->readAll();
    _protocol->parseData(data);

    emit dataIn(data);
}
