#ifndef CLHANDLER_H
#define CLHANDLER_H

#include <QObject>
#include <QCommandLineParser>
#include <QMutex>

#include "qkconn.h"
#include "qkserver.h"

#define _return() {emit done(); return;}
#define _exit(code) {qApp->exit(code); return;}

class QCoreApplication;
class QkConnectServer;
class QkSpyServer;
class QkConn;
class QkConnThread;
class QMutex;

class CLHandler : public QObject
{
    Q_OBJECT
public:
    explicit CLHandler(QCoreApplication *app, QObject *parent = 0);

signals:
    void done();

public slots:
    void run();
    void _slotMessage(int type, QString message, bool timestamp = true);
    void _slotDataToClient(QByteArray data);
    void _slotDataToConn(QByteArray data);

private:
    void _clHelp();
    void _showHelp(const QCommandLineParser &parser);
    void _quitThreads();
    QkConn::Status _waitConnReady(QkConn *conn);
    QkServer::Status _waitServerReady(QkServer *server);

    QCoreApplication *_app;
    QThread *connectServerThread;
    QThread *spyServerThread;
    QThread *connThread;
    QkConnectServer *connectServer;
    QkSpyServer *spyServer;
    QkConn *conn;
    QMutex _mutex;
    bool _verbose;

};

#endif // CLHANDLER_H
