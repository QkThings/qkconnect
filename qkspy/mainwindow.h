#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAbstractSocket>
#include "qkutils.h"
#include "qkcore.h"

using namespace QkUtils;

class QTcpSocket;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void slotConnect();
    void slotConnected();
    void slotDisconnected();
    void slotReadyRead();
    void slotSocketError(QAbstractSocket::SocketError error);
    void slotClear();
    void _parseJson(QJsonDocument json);
    void _handleFrame(QByteArray frame, bool raw);
    void _handleFormatChanged(QString format);


private:
    void _parseData(QByteArray data);

    Ui::MainWindow *ui;
    QTcpSocket *_socket;

    QColor _colorClient;
    QColor _colorConn;

    JsonParser *_jsonParser;
    Qk::Protocol *_protocol;
    bool _lastPrintFromClient;
};

#endif // MAINWINDOW_H
