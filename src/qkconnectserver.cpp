#include "qkconnectserver.h"
#include "qkclientthread.h"
#include "qksocket.h"
#include "qkconnect_global.h"

#include <QDebug>
#include <QQueue>
#include <QEventLoop>
#include <QTimer>
#include <QJsonObject>
#include <QJsonValue>

QkConnectServer::QkConnectServer(QString ip, int port, QObject *parent) :
    QkServer(ip, port, parent)
{
    _parseMode = ParseModeJSON;
    _protocolIn = new Qk::Protocol(this);
    _protocolOut = new Qk::Protocol(this);

    connect(_protocolIn, SIGNAL(parsedFrame(QByteArray,bool)),
            this, SLOT(handleFrameIn(QByteArray,bool)));
    connect(_protocolOut, SIGNAL(parsedFrame(QByteArray,bool)),
            this, SLOT(handleFrameOut(QByteArray,bool)));

    connect(&_jsonInParser, SIGNAL(parsed(QJsonDocument)),
            this, SLOT(handleJsonIn(QJsonDocument)));

    ADD_RPC_METHOD(quit);
    //_rpc_map.insert("quit", &QkConnectServer::rpc_quit);
}

void QkConnectServer::setParseMode(ParseMode mode)
{
    _parseMode = mode;
}

void QkConnectServer::setOptions(int options)
{
    _options = options;
}

void QkConnectServer::run()
{
    /*if(_parseMode)
    {
        QEventLoop eventLoop;
        QTimer timer;
        int framesInCount;
        bool frameReceived;
        bool alive;

        timer.setSingleShot(true);
        connect(&timer, SIGNAL(timeout()), &eventLoop, SLOT(quit()));
        connect(_protocolOut, SIGNAL(parsedFrame(QByteArray,bool)),
                &eventLoop, SLOT(quit()));

        while(1)
        {
            eventLoop.processEvents();

            _mutex.lock();
            framesInCount = _dataInQueue.count();
            alive = _alive;
            _mutex.unlock();

            if(!alive)
                break;

            if(framesInCount > 0)
            {
                _mutex.lock();
                QByteArray frame = _dataInQueue.dequeue();
                frameReceived = _frameReceived = false;
                _mutex.unlock();

                emit dataIn(frame);

                timer.start(5000);
                while(timer.isActive() && !frameReceived)
                {
                    eventLoop.exec();
                    _mutex.lock();
                    frameReceived = _frameReceived;
                    _mutex.unlock();
                }

                if(!frameReceived)
                    qDebug() << "timeout! can't get a frame from connection";
            }
        }
    }*/
}


void QkConnectServer::handleDataIn(int socketDesc, QByteArray data)
{
    switch(_parseMode)
    {
    case ParseModeRaw:
        emit dataIn(data);
        break;
    case ParseModeProtocol:
        _protocolIn->parseData(data, true);
        break;
    case ParseModeJSON:
        _jsonInParser.parseData(data);
        break;
    default:
        qFatal("ERROR: unknown parse mode");
    }

//    if(_parseMode)
//    {
//        _protocolIn->parseData(data, true);
//    }
//    else
//    {
//        emit dataIn(data);
//    }
}

void QkConnectServer::sendData(QByteArray data)
{
    switch(_parseMode)
    {
    case ParseModeRaw:
        emit dataOut(data);
        break;
    case ParseModeProtocol:
        _protocolOut->parseData(data, true);
        break;
    case ParseModeJSON:
        break;
    default:
        qFatal("ERROR: unknown parse mode");
    }

//    if(_parseMode)
//    {
//        _protocolOut->parseData(data, true);
//    }
//    else
//    {
//        emit dataOut(data);
//    }
}

void QkConnectServer::sendPacket(QJsonDocument doc)
{
    emit dataOut(doc.toJson(QJsonDocument::Compact));
}

void QkConnectServer::handleFrameIn(QByteArray frame, bool raw)
{
    _mutex.lock();
    _dataInQueue.enqueue(frame);
    _mutex.unlock();
}

void QkConnectServer::handleFrameOut(QByteArray frame, bool raw)
{
    _mutex.lock();
    _frameReceived = true;
    _mutex.unlock();

    int flags = Qk::Frame::flags(frame, raw);

    if((_options & joinFragments) && (flags & Qk::PACKET_FLAG_FRAG))
    {
        _fragments.append(frame);
        if(flags & Qk::PACKET_FLAG_LASTFRAG)
        {
            QByteArray fullFrame = Qk::Frame::join(_fragments, raw);
            emit dataOut(fullFrame);
            _fragments.clear();
        }
    }
    else
        emit dataOut(frame);

}

void QkConnectServer::handleJsonIn(QJsonDocument doc)
{
    QJsonObject obj = doc.object();
    QStringList obj_keys = obj.keys();
    if(obj_keys.contains("pkt"))
    {
        emit dataIn(doc.toJson(QJsonDocument::Compact));
        emit packetIn(doc);
    }
    else if(obj_keys.contains("rpc"))
    {
        emit message(QKCONNECT_MESSAGE_INFO, "RPC RECEIVED");
    }
    else
    {
        emit message(QKCONNECT_MESSAGE_ERROR, "Unknown JSON data");
    }
}

void QkConnectServer::_slotClientConnected(int socketDesc)
{
    QkServer::_slotClientConnected(socketDesc);

    QkSocket *socket = _threads.value(socketDesc)->socket();
    QString heyMsg = QString().sprintf("Hey: %s (client:%d)",
                                       socket->peerAddress().toString().toStdString().c_str(),
                                       socketDesc);

    emit message(QKCONNECT_MESSAGE_INFO, heyMsg);
}

void QkConnectServer::_slotClientDisconnected(int socketDesc)
{
    QkSocket *socket = _threads.value(socketDesc)->socket();
    QString byeMsg = QString().sprintf("Bye: %s (client:%d)",
                                       socket->peerAddress().toString().toStdString().c_str(),
                                       socketDesc);

    emit message(QKCONNECT_MESSAGE_INFO, byeMsg);

    QkServer::_slotClientDisconnected(socketDesc);
}

int QkConnectServer::rpc_quit(RPCArgs *args)
{
    qDebug() << __PRETTY_FUNCTION__;
}

