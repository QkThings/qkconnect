#include "qkprotocol.h"

#include <QDebug>
#include <QJsonObject>

QkProtocol::QkProtocol(QObject *parent) : QObject(parent)
{

}

bool QkProtocol::validatePacket(QJsonDocument packet)
{
    QJsonObject pkt_obj = packet.object();
    if(!pkt_obj.contains("pkt"))
        return false;
    pkt_obj = pkt_obj["pkt"].toObject();
    if(!pkt_obj.contains("id"))
        return false;
    if(!pkt_obj.contains("code"))
        return false;

    QString code_name = pkt_obj["code"].toString();
    if(!pkt_obj.contains(code_name))
        return false;

    return true;
}

