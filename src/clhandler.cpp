#include "clhandler.h"
#include "qkconnect_global.h"
#include "qkconnectserver.h"
#include "qkspyserver.h"
#include "qkconn.h"
#include "qkconnloopback.h"
#include "qkconnserial.h"
#include "qkconnthread.h"

#include <QDebug>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTextStream>
#include <QHostAddress>
#include <QEventLoop>
#include <QThread>
#include <QTime>
#include <QTimer>
#include <QMutexLocker>

CLHandler::CLHandler(QCoreApplication *app, QObject *parent) :
    QObject(parent),
    _app(app)
{
    connThread = 0;
    connectServerThread = 0;
    spyServerThread = 0;
    _verbose = false;
}

void CLHandler::run()
{
    QCommandLineParser parser;
    //parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("ip", QCoreApplication::translate("main", "Server's IP"));
    parser.addPositionalArgument("port", QCoreApplication::translate("main", "Server's port"));
    parser.addPositionalArgument("conn", QCoreApplication::translate("main", "Connection type"));
    parser.addPositionalArgument("params", QCoreApplication::translate("main", "Connection parameters"));

    QCommandLineOption optionHelp(QStringList() << "h" << "help",
                                  tr("Show help."));
    parser.addOption(optionHelp);

    QCommandLineOption optionVerbose(QStringList() << "verbose",
                                   tr("Verbose mode"));
    parser.addOption(optionVerbose);

    QCommandLineOption optionParse(QStringList() << "p" << "parse",
                                   tr("Parse bytestream into packets."));
    parser.addOption(optionParse);

    QCommandLineOption optionJoinFragments(QStringList() << "j" << "join",
                                   tr("Join fragments (only if parse mode is enabled)."));
    parser.addOption(optionJoinFragments);

    QCommandLineOption optionListSerial(QStringList() << "list-serial",
                                        QCoreApplication::translate("main", "List available serial ports."));
    parser.addOption(optionListSerial);

    parser.process(*_app);

    _verbose = parser.isSet(optionVerbose);

    if(parser.isSet(optionHelp)) { _showHelp(parser); _exit(0); }

    if(parser.isSet(optionListSerial))
    {
        QkConnSerial::listAvailable();
        _exit(0);
    }

    const QStringList args = parser.positionalArguments();

    if(args.count() < 3)
    {
        _slotMessage(QKCONNECT_MESSAGE_ERROR, "Invalid number of arguments", false);
        _exit(1);
    }

    QString serverIP = args.at(0);
    int serverPort = args.at(1).toInt();
    int spyServerPort = serverPort + 1;
    QString connType = args.at(2);
    QStringList connParams = args.mid(3);
    bool parseMode = parser.isSet(optionParse);
    bool joinFragments = parser.isSet(optionJoinFragments);

    qDebug("Server:     %s %d (spy:%d)",
            serverIP.toStdString().c_str(),
            serverPort, spyServerPort);
    qDebug("Connection: %s", connType.toStdString().c_str());
    qDebug() << "Parameters:" << connParams;
    qDebug() << "Parse? " << parseMode;
    qDebug() << "Join?  " << joinFragments;
    qDebug() << "Verbose" << _verbose;

    connThread = new QThread(this);
    QkConn::Descriptor connDesc;
    if(connType == "loopback")
    {
        conn = new QkConnLoopback();
    }
    else if(connType == "serial")
    {
        if(connParams.count() != 3)
        {
            _slotMessage(QKCONNECT_MESSAGE_ERROR, "Invalid number of parameters", false);
            _exit(1);
        }
        connDesc.params["portname"] = connParams.at(0);
        connDesc.params["baudrate"] = connParams.at(1);
        connDesc.params["dtr"] = connParams.at(2);
        conn = new QkConnSerial(connDesc);
    }
    else
    {
        _slotMessage(QKCONNECT_MESSAGE_ERROR, "Invalid connection type", false);
        _exit(1);
    }

    qDebug() << "Need help? Type 'help'";

    conn->moveToThread(connThread);
    connect(connThread, SIGNAL(started()), conn, SLOT(open()));
    connect(connThread, SIGNAL(finished()), conn, SLOT(deleteLater()));
    connect(connThread, SIGNAL(finished()), connThread, SLOT(deleteLater()));
    connect(conn, SIGNAL(message(int,QString)), this, SLOT(_slotMessage(int,QString)), Qt::DirectConnection);

    connThread->start();
    if(_waitConnReady(conn) != QkConn::Ready)
    {
        _quitThreads();
        _exit(1);
    }

    connectServerThread = new QThread(this);
    connectServer = new QkConnectServer(serverIP, serverPort);
    //connectServer->setParseMode(parseMode);
    int options;
    if(joinFragments)
        options |= QkConnectServer::joinFragments;
    connectServer->setOptions(options);

    connectServer->moveToThread(connectServerThread);

    connect(connectServerThread, SIGNAL(started()), connectServer, SLOT(create()));
    connect(connectServerThread, SIGNAL(finished()), connectServer, SLOT(deleteLater()));
    connect(connectServerThread, SIGNAL(finished()), connectServerThread, SLOT(deleteLater()));

    connect(connectServer, SIGNAL(message(int,QString)), this, SLOT(_slotMessage(int,QString)), Qt::DirectConnection);
    //connect(connectServer, SIGNAL(dataIn(QByteArray)), conn, SLOT(sendData(QByteArray)));
    //connect(conn, SIGNAL(dataIn(QByteArray)), connectServer, SLOT(sendData(QByteArray)));
    connect(connectServer, SIGNAL(packetIn(QJsonDocument)), conn, SLOT(sendPacket(QJsonDocument)));
    connect(conn, SIGNAL(packetIn(QJsonDocument)), connectServer, SLOT(sendPacket(QJsonDocument)));

    connectServerThread->start();
    if(_waitServerReady(connectServer) != QkServer::Connected)
    {
        _quitThreads();
        _exit(1);
    }

    spyServerThread = new QThread(this);
    spyServer = new QkSpyServer(serverIP, spyServerPort);

    spyServer->moveToThread(spyServerThread);

    connect(spyServerThread, SIGNAL(started()), spyServer, SLOT(create()));
    connect(spyServerThread, SIGNAL(finished()), spyServer, SLOT(deleteLater()));
    connect(spyServerThread, SIGNAL(finished()), spyServerThread, SLOT(deleteLater()));
    connect(spyServer, SIGNAL(message(int,QString)), this, SLOT(_slotMessage(int,QString)), Qt::DirectConnection);

    connect(connectServer, SIGNAL(dataIn(QByteArray)), spyServer, SLOT(sendFromClient(QByteArray)));
    connect(connectServer, SIGNAL(dataOut(QByteArray)), spyServer, SLOT(sendFromConn(QByteArray)));
    connect(connectServer, SIGNAL(dataIn(QByteArray)), this, SLOT(_slotDataToConn(QByteArray)), Qt::DirectConnection);
    connect(connectServer, SIGNAL(dataOut(QByteArray)), this, SLOT(_slotDataToClient(QByteArray)),Qt::DirectConnection);

    spyServerThread->start();
    if(_waitServerReady(spyServer) != QkServer::Connected)
    {
        _quitThreads();
        _exit(1);
    }


    while(true)
    {
//        cout << "> ";
//        cout.flush();
        QString inputText = cin.readLine();

        if(!inputText.isEmpty())
        {
            if(inputText == "quit")
                break;
            else if(inputText == "help")
            {
                _clHelp();
            }
            else
            {
                cout << "Unknown command: " << inputText << "\n";
                cout.flush();
            }
        }
    }

    _quitThreads();
    _exit(0);
}

void CLHandler::_quitThreads()
{
    if(connThread != 0 &&
       connThread->isRunning())
    {
        connThread->quit();
        connThread->wait();
    }

    if(connectServerThread != 0 &&
       connectServerThread->isRunning())
    {
        connectServer->kill();
        connectServerThread->quit();
        connectServerThread->wait();
    }

    if(spyServerThread != 0 &&
       spyServerThread->isRunning())
    {
        spyServerThread->quit();
        spyServerThread->wait();
    }
}

QkConn::Status CLHandler::_waitConnReady(QkConn *conn)
{
    QEventLoop eventLoop;
    QTimer timer;
    connect(&timer, SIGNAL(timeout()), &eventLoop, SLOT(quit()));
    connect(conn, SIGNAL(statusChanged()), &eventLoop, SLOT(quit()));
    timer.start(5000);
    eventLoop.exec();
    disconnect(conn, SIGNAL(statusChanged()), &eventLoop, SLOT(quit()));
    return conn->currentStatus();
}

QkServer::Status CLHandler::_waitServerReady(QkServer *server)
{
    QEventLoop eventLoop;
    QTimer timer;
    connect(&timer, SIGNAL(timeout()), &eventLoop, SLOT(quit()));
    connect(server, SIGNAL(statusChanged()), &eventLoop, SLOT(quit()));
    timer.start(5000);
    eventLoop.exec();
    disconnect(server, SIGNAL(statusChanged()), &eventLoop, SLOT(quit()));
    return server->currentStatus();
}

void CLHandler::_slotDataToConn(QByteArray data)
{
    if(_verbose)
    {
        cout << "--> " <<  data << "\n";
        cout.flush();
    }
}

void CLHandler::_slotDataToClient(QByteArray data)
{
    if(_verbose)
    {
        cout << "<-- " <<  data << "\n";
        cout.flush();
    }
}

void CLHandler::_slotMessage(int type, QString message, bool timestamp)
{
    QMutexLocker locker(&_mutex);

    QString timeStr = "(" + QTime::currentTime().toString("hh:mm:ss") + ") ";
    QString typeStr;
    switch(type)
    {
    case QKCONNECT_MESSAGE_INFO: typeStr ="[i] "; break;
    case QKCONNECT_MESSAGE_ERROR: typeStr = "[e] "; break;
    }

    if(timestamp)
        cout << timeStr;
    cout << typeStr << message << "\n";
    cout.flush();
}

void CLHandler::_showHelp(const QCommandLineParser &parser)
{
    qDebug("%s", parser.helpText().toStdString().c_str());
    qDebug() << "[conn]           [params]";
    qDebug() << "loopback";
    qDebug() << "serial           <portname> <baudrate> <dtr>";
    qDebug() << "";
}

void CLHandler::_clHelp()
{
    cout << "quit      Quit" << "\n";
    cout.flush();
}

