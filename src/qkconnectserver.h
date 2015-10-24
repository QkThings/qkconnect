#ifndef QKCONNECTSERVER_H
#define QKCONNECTSERVER_H

#include "qkserver.h"
#include "qkcore.h"
#include "qkutils.h"
#include <QObject>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>

#define ADD_RPC_METHOD(name)  _rpc_map.insert( "##name##", &QkConnectServer::rpc_##name )

class QkConnectServer : public QkServer
{
    Q_OBJECT
public:
    enum Options
    {
        joinFragments = (1<<0)
    };
    enum ParseMode
    {
        ParseModeRaw,
        ParseModeProtocol,
        ParseModeJSON
    };

    explicit QkConnectServer(QString ip, int port, QObject *parent = 0);

    void setParseMode(ParseMode mode);
    void setOptions(int options);

signals:
    void packetIn(QJsonDocument);

public slots:
    void sendData(QByteArray data);
    void sendPacket(QJsonDocument doc);

protected slots:
    void _slotClientConnected(int socketDesc);
    void _slotClientDisconnected(int socketDesc);
    void handleDataIn(int socketDesc, QByteArray data);
    void handleFrameIn(QByteArray frame, bool raw);
    void handleFrameOut(QByteArray frame, bool raw);
    void handleJsonIn(QJsonDocument doc);

protected:
    void run();

    //bool _parseMode;
    bool _frameReceived;
    QkUtils::JsonParser _jsonInParser;
    Qk::Protocol *_protocolIn;
    Qk::Protocol *_protocolOut;
    QQueue<QByteArray> _dataInQueue;
    QWaitCondition _waitInputFrame;
    QList<QByteArray> _fragments;
    int _options;
    ParseMode _parseMode;

    typedef QMap<QString,QVariant> RPCArgs;
    typedef int (QkConnectServer::*RPC)(RPCArgs *args);

    QMap<QString,RPC> _rpc_map;

    int rpc_quit(RPCArgs *args);

};

#endif // QKCONNECTSERVER_H
