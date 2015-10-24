#ifndef QKCONN_H
#define QKCONN_H

#include <QObject>
#include <QMap>
#include <QVariant>
#include <QJsonDocument>

class QkConn : public QObject
{
    Q_OBJECT
public:
    enum Status
    {
        Closed,
        Ready,
        FailedToOpen
    };

    class Descriptor
    {
    public:
        QMap<QString,QVariant> params;
        bool operator==(Descriptor &other);
    };

    explicit QkConn(QObject *parent = 0);
    Status currentStatus();

    static void listAvailable();

signals:
    void statusChanged();
    void message(int, QString);
    void dataIn(QByteArray);
    void packetIn(QJsonDocument);

public slots:
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual void sendData(QByteArray data) = 0;
    virtual void sendPacket(QJsonDocument doc) = 0;

protected:
    void _changeStatus(Status status);

    Descriptor _desc;
    Status _status;

};

#endif // QKCONN_H
