#include "qkprotocolserial.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QVariantMap>
#include <QTime>

const QMap<quint8, QString> QkProtocolSerial::_packetCodeMap{
    {QkProtocolSerial::PACKET_CODE_ACK, "ack"},
    {QkProtocolSerial::PACKET_CODE_SEARCH, "search"},
    {QkProtocolSerial::PACKET_CODE_INFOQK, "info_qk"},
    {QkProtocolSerial::PACKET_CODE_INFOBOARD, "info_board"},
    {QkProtocolSerial::PACKET_CODE_INFOCOMM, "info_comm"},
    {QkProtocolSerial::PACKET_CODE_INFODEVICE, "info_device"},
    {QkProtocolSerial::PACKET_CODE_INFOCONFIG, "info_config"},
    {QkProtocolSerial::PACKET_CODE_INFODATA, "info_data"},
    {QkProtocolSerial::PACKET_CODE_INFOEVENT, "info_event"},
    {QkProtocolSerial::PACKET_CODE_INFOACTION, "info_action"},
    {QkProtocolSerial::PACKET_CODE_DATA, "data"},
    {QkProtocolSerial::PACKET_CODE_EVENT, "event"},
    {QkProtocolSerial::PACKET_CODE_STRING, "string"},
    {QkProtocolSerial::PACKET_CODE_SETNAME, "set_name"},
};

QkProtocolSerial::QkProtocolSerial(QObject *parent) : QkProtocol(parent)
{

}

void QkProtocolSerial::translatePacket(QJsonDocument packet)
{
    if(!validatePacket(packet))
    {
        qDebug() << "ERROR: invalid packet " << packet.toJson(QJsonDocument::Compact);
        return;
    }

    QJsonObject pkt_obj = packet.object().value("pkt").toObject();

    int pkt_id = pkt_obj["id"].toInt();
    QString code_name = pkt_obj["code"].toString();
    QJsonObject code_obj = pkt_obj[code_name].toObject();

    int pkt_code = _codeFromName(code_name);
    int pkt_flags = 0;

    QByteArray pkt_bindata;
    pkt_bindata.append(PROTOCOL_BEP);
    pkt_bindata.append(pkt_flags >> 8);
    pkt_bindata.append(pkt_flags & 0xFF);
    pkt_bindata.append(pkt_id & 0xFF);
    pkt_bindata.append(pkt_code);
    pkt_bindata.append(PROTOCOL_BEP);

    emit serialOut(pkt_bindata);
}

void QkProtocolSerial::parseData(QByteArray data)
{
    //    qDebug() << __PRETTY_FUNCTION__;
        int i;
        quint8 *p_data = (quint8*)data.data();

        for(i = 0; i < data.count(); i++, p_data+=1)
        {
            qDebug("rx: %02X", *p_data & 0xFF);

            if(*p_data == PROTOCOL_BEP)
            {
                if(!_escape)
                {
                    if(!_receiving)
                    {
                        _receiving = true;
                        _valid = true;
                    }
                    else
                    {
                        if(_valid && _frame.count() > 0)
                        {
                            //emit parsedFrame(_frame, raw);
                            _handleFrame();
                            _frame.clear();
                            _receiving = false;
                            _valid = false;
                        }
                    }
                    continue;
                }
            }
            if(*p_data == PROTOCOL_DLE)
            {
                if(_valid)
                {
                    if(!_escape)
                    {
                        _escape = true;
                        continue;
                    }
                }
            }

            if(_valid)
                _frame.append(*p_data);

            _escape = false;
        }
}

void QkProtocolSerial::_handleFrame()
{
    bool fullFrameReceived = false;
    int flags = _frameFlags(_frame);
    int code = _frameCode(_frame);

    if(flags & QkProtocolSerial::PACKET_FLAG_FRAG)
    {
        _fragments.append(_frame);
        if(flags & QkProtocolSerial::PACKET_FLAG_LASTFRAG)
        {
            _frame = _frameJoin(_fragments);
            _fragments.clear();
            fullFrameReceived = true;
        }
    }
    else
    {
        fullFrameReceived = true;
    }

    if(fullFrameReceived)
    {
        qDebug() << _codeFriendlyName(code);
        _fullFrames.append(_frame);
        switch(code)
        {
        case PACKET_CODE_ACK:
        case PACKET_CODE_READY:
        case PACKET_CODE_DATA:
        case PACKET_CODE_EVENT:
            generatePacket();
            _fullFrames.clear();
            break;
        default: ;
        }
    }
}

void QkProtocolSerial::generatePacket()
{
    qDebug() << "GENERATE PACKET";

    int id = _frameID(_fullFrames.last());

    QJsonObject main_obj;
    main_obj.insert("id", QJsonValue(id));
    main_obj.insert("ts", QJsonValue(QTime::currentTime().toString("hh:mm:ss:z")));

    foreach(QByteArray frame, _fullFrames)
    {
        int code = _frameCode(frame);

        QJsonObject code_obj;

        switch(code)
        {
        case PACKET_CODE_READY:
            break;
        default: ;
        }

        QString code_name = _codeFriendlyName(code).toLower();
        main_obj.insert(code_name, QJsonValue(code_obj));
    }

    QJsonObject pkt_obj;
    pkt_obj.insert("pkt", QJsonValue(main_obj));

    QJsonDocument pkt_doc(pkt_obj);
    qDebug() << pkt_doc.toJson(QJsonDocument::Compact);
    emit packetGenerated(pkt_doc);
}

int QkProtocolSerial::_frameFlags(const QByteArray &frame)
{
    int flags = 0;

    flags = (frame[0] & 0xFF) +
            ((frame[1] & 0xFF) << 8);

    return flags;
}

int QkProtocolSerial::_frameID(const QByteArray &frame)
{
    return (int) (frame[2] & 0xFF);
}

int QkProtocolSerial::_frameCode(const QByteArray &frame)
{
    return (int) (frame[3] & 0xFF);
}

QByteArray QkProtocolSerial::_framePayload(const QByteArray &frame)
{
    return frame.right(frame.count() - 4);
}

QByteArray QkProtocolSerial::_frameJoin(QList<QByteArray> fragments)
{
    Q_ASSERT(fragments.count() > 0);
    QByteArray frame;

    frame += fragments.takeFirst();

    while(!fragments.isEmpty())
    {
        QByteArray fragment = fragments.takeFirst();
        const int headerByteCount = 4;

        frame += fragment.right(fragment.count() - headerByteCount);
    }

    return frame;
}

QString QkProtocolSerial::_codeFriendlyName(int code)
{
    if(_packetCodeMap.contains(code))
    {
        return _packetCodeMap.value(code);
    }
    else
    {
        return QString("???");
    }
}

int QkProtocolSerial::_codeFromName(const QString &name)
{
    if(_packetCodeMap.values().contains(name))
    {
        return (int) _packetCodeMap.key(name);
    }
    else
    {
        return -1;
    }
}




