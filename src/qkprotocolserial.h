#ifndef QKPROTOCOLSERIAL_H
#define QKPROTOCOLSERIAL_H

#include <QObject>
#include <QMap>
#include "qkprotocol.h"

class QkProtocolSerial : public QkProtocol
{
    Q_OBJECT
public:
    explicit QkProtocolSerial(QObject *parent = 0);

public slots:
    void translatePacket(QJsonDocument packet);
    void generatePacket();
    void parseData(QByteArray data);

signals:
    void serialOut(QByteArray);

private:
    static const quint8 PROTOCOL_BEP = 0x55;
    static const quint8 PROTOCOL_DLE = 0xDD;

    static const quint16 PACKET_FLAG_FRAG = 0x0004;
    static const quint16 PACKET_FLAG_LASTFRAG = 0x0002;

    static const quint8 PACKET_CODE_OK = 0x01;
    static const quint8 PACKET_CODE_ACK = 0x03;
    static const quint8 PACKET_CODE_ERR = 0xFF;
    static const quint8 PACKET_CODE_TIMEOUT = 0xFE;
    static const quint8 PACKET_CODE_SAVE = 0x04;
    static const quint8 PACKET_CODE_RESTORE = 0x05;
    static const quint8 PACKET_CODE_SEARCH = 0x06;
    static const quint8 PACKET_CODE_START = 0x0A;
    static const quint8 PACKET_CODE_STOP = 0x0F;
    static const quint8 PACKET_CODE_READY = 0x0D;
    static const quint8 PACKET_CODE_GETNODE = 0x10;
    static const quint8 PACKET_CODE_SETNAME = 0x34;
    static const quint8 PACKET_CODE_SETCONFIG = 0x3C;
    static const quint8 PACKET_CODE_SETSAMP = 0x36;
    static const quint8 PACKET_CODE_ACTION = 0x35;
    static const quint8 PACKET_CODE_INFOQK = 0xB1;
    static const quint8 PACKET_CODE_INFOSAMP = 0xB2;
    static const quint8 PACKET_CODE_INFOBOARD = 0xB5;
    static const quint8 PACKET_CODE_INFOCOMM = 0xB6;
    static const quint8 PACKET_CODE_INFODEVICE = 0xB7;
    static const quint8 PACKET_CODE_INFOACTION = 0xBA;
    static const quint8 PACKET_CODE_INFODATA = 0xBD;
    static const quint8 PACKET_CODE_INFOEVENT = 0xBE;
    static const quint8 PACKET_CODE_INFOCONFIG = 0xBC;
    static const quint8 PACKET_CODE_STATUS = 0xD5;
    static const quint8 PACKET_CODE_DATA = 0xD0;
    static const quint8 PACKET_CODE_EVENT = 0xDE;
    static const quint8 PACKET_CODE_STRING = 0xDF;

    static const QMap<quint8, QString> _packetCodeMap;

    static int _frameFlags(const QByteArray &frame);
    static int _frameID(const QByteArray &frame);
    static int _frameCode(const QByteArray &frame);
    static QByteArray _framePayload(const QByteArray &frame);

    static QByteArray _frameJoin(QList<QByteArray> fragments);
    static QString _codeFriendlyName(int code);
    static int _codeFromName(const QString &name);

    void _handleFrame();

    bool _receiving;
    bool _escape;
    bool _valid;
    QByteArray _frame;
    QByteArray _rawFrame;
    QList<QByteArray> _fragments;
    QList<QByteArray> _fullFrames;

};

#endif // QKPROTOCOLSERIAL_H
