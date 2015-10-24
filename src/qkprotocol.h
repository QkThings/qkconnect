#ifndef QKPROTOCOL_H
#define QKPROTOCOL_H

#include <QObject>
#include <QJsonDocument>

class QkProtocol : public QObject
{
    Q_OBJECT
public:
    explicit QkProtocol(QObject *parent = 0);

    bool validatePacket(QJsonDocument packet);

public slots:
    virtual void translatePacket(QJsonDocument packet) = 0;
    virtual void generatePacket() = 0;

signals:
    void packetGenerated(QJsonDocument);

};

#endif // QKPROTOCOL_H
