#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTcpSocket>
#include <QAbstractSocket>
#include <QHostAddress>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QVariantMap>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

#ifdef Q_OS_UNIX
    ui->textWindow->setFont(QFont("Monospace", 9));
#endif
#ifdef Q_OS_WIN
    ui->textWindow->setFont(QFont("Consolas", 9));
#endif

    ui->textWindow->setUndoRedoEnabled(false);
    ui->textWindow->setReadOnly(true);

    _lastPrintFromClient = true;

    _jsonParser = new JsonParser(this);
    connect(_jsonParser, SIGNAL(parsed(QJsonDocument)),
            this, SLOT(_parseJson(QJsonDocument)));

    _protocol = new Qk::Protocol(this);
    connect(_protocol, SIGNAL(parsedFrame(QByteArray,bool)),
            this, SLOT(_handleFrame(QByteArray,bool)));

    _colorClient = QColor("#53DE9D");
    _colorConn = QColor("#83D6F2");

    _socket = new QTcpSocket(this);

    connect(_socket, SIGNAL(readyRead()),
            this, SLOT(slotReadyRead()));
    connect(_socket, SIGNAL(connected()), this, SLOT(slotConnected()));
    connect(_socket, SIGNAL(disconnected()), this, SLOT(slotDisconnected()));
    connect(_socket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(slotSocketError(QAbstractSocket::SocketError)));

    connect(ui->buttonConnect, SIGNAL(clicked()),
            this, SLOT(slotConnect()));
    connect(ui->buttonClear, SIGNAL(clicked()),
            this, SLOT(slotClear()));
    connect(ui->comboFormat, SIGNAL(currentIndexChanged(QString)),
            this, SLOT(_handleFormatChanged(QString)));

    setWindowTitle("QkSpy");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::slotConnect()
{
    if(ui->buttonConnect->text() == "Connect")
    {
        QString ip = ui->lineIP->text();
        int port = ui->spinPort->value();
        QHostAddress host;
        if(ip.toLower() == "localhost")
            host = QHostAddress::LocalHost;
        else
            host.setAddress(ip);

        _socket->connectToHost(host, port);
    }
    else
    {
        _socket->disconnectFromHost();
    }
}

void MainWindow::slotConnected()
{
    ui->buttonConnect->setText("Disconnect");
    ui->lineIP->setEnabled(false);
    ui->spinPort->setEnabled(false);
}

void MainWindow::slotDisconnected()
{
    ui->buttonConnect->setText("Connect");
    ui->lineIP->setEnabled(true);
    ui->spinPort->setEnabled(true);
}

void MainWindow::slotReadyRead()
{
    QByteArray data = _socket->readAll();
    qDebug() << "data rx:" << data;
    _parseData(data);
}

void MainWindow::slotSocketError(QAbstractSocket::SocketError error)
{
    QMessageBox::critical(this, "Error", _socket->errorString());
}

void MainWindow::slotClear()
{
    ui->textWindow->clear();
}

void MainWindow::_parseData(QByteArray data)
{
    _jsonParser->parseData(data);
}

void MainWindow::_parseJson(QJsonDocument json)
{
    qDebug() << __FUNCTION__;
    QJsonObject obj = json.object();


    QString src = obj.value("src").toString();
    QJsonObject data_obj = obj.value("data").toObject();
//    QString data_type = data_obj.value("type").toString();
    QString data_format = data_obj.value("format").toString();

    QByteArray data_serial;
    if(data_format == "serial")
    {
        data_serial = QByteArray::fromBase64(data_obj.value("data").toString().toUtf8());
    }

    QString text = "";

    if(src == "client")
    {
        if(!_lastPrintFromClient) ui->textWindow->insertPlainText("\n");
        ui->textWindow->setTextColor(_colorClient);
        _lastPrintFromClient = true;
    }
    else
    {
        if(_lastPrintFromClient) ui->textWindow->insertPlainText("\n");
        ui->textWindow->setTextColor(_colorConn);
        _lastPrintFromClient = false;
    }

    if(ui->comboFormat->currentText() == "HEX")
    {
        if(data_format == "serial")
        {
            foreach(char c, data_serial)
            {
                text += QString().sprintf("%02X ", c & 0xFF);
            }
            ui->textWindow->insertPlainText(text);
        }
    }
    else if(ui->comboFormat->currentText() == "ASCII")
    {
        if(data_format == "serial")
        {
            ui->textWindow->insertPlainText(QString(data_serial));
        }
    }
    else if(ui->comboFormat->currentText() == "JSON")
    {
        ui->textWindow->insertPlainText(QString(json.toJson()));
    }
    else if(ui->comboFormat->currentText() == "PACKET")
    {
        if(data_format == "serial")
        {
            _protocol->parseData(data_serial, true);
        }
    }

    ui->textWindow->ensureCursorVisible();
}

void MainWindow::_handleFrame(QByteArray frame, bool raw)
{
    Qk::Packet packet;
    Qk::Packet::fromFrame(frame, raw, &packet);
    ui->textWindow->append(Qk::Packet::friendlyName(packet.code()));
    ui->textWindow->append(" Flags:   " + QString().sprintf("%04X", packet.flags()));
    ui->textWindow->append(" ID:      " + QString().sprintf("%d", packet.id()));
    ui->textWindow->append(" Code:    " + QString().sprintf("%02X", packet.code()));
    ui->textWindow->append(" Payload: " + QString().sprintf("[size:%d]", packet.payload().count()));

    //TODO discard DLE bytes if raw == true

    QString payloadHex;
    foreach(char c, packet.payload())
    {
        payloadHex += QString().sprintf("%02X ", c & 0xFF);
    }
    if(!payloadHex.isEmpty())
        ui->textWindow->append(payloadHex);
    ui->textWindow->append("-------------");
}

void MainWindow::_handleFormatChanged(QString format)
{
    if(format == "PACKET")
        _protocol->init();
}
