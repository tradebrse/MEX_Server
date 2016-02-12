#ifndef MEX_SERVERTHREAD_H
#define MEX_SERVERTHREAD_H

#include <QThread>
#include <QTcpSocket>
#include <mex_xmlprocessor.h>
#include <mex_order.h>

class MEX_ServerThread : public QThread
{
    Q_OBJECT
public:
    explicit MEX_ServerThread(qintptr ID, QByteArray currentData, QObject *parent = 0);

    void run();
signals:
    void error(QTcpSocket::SocketError socketerror);
    void receivedOrder(MEX_Order);
    void updateRequest();

public slots:
    void readyRead();
    void disconnected();
    void writeData(QByteArray newData);
    void requestUpdate();
    void receiveOrder(MEX_Order);

private slots:

private:
    QTcpSocket* socket;
    qintptr socketDescriptor;
    QByteArray currentOrderbookData;
    QByteArray newData;
    bool abort;
};

#endif // MEX_SERVERTHREAD_H
