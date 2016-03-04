#ifndef MEX_SERVERTHREAD_H
#define MEX_SERVERTHREAD_H

#include <QThread>
#include <QTcpSocket>
#include <mex_xmlprocessor.h>
#include <mex_order.h>
#include <QElapsedTimer>

class MEX_ServerThread : public QThread
{
    Q_OBJECT
public:
    explicit MEX_ServerThread(qintptr ID, QObject *parent = 0);

    void run();
signals:
    void error(QTcpSocket::SocketError socketerror);
    void receivedOrder(MEX_Order);
    void updateRequest();

public slots:
    void readyRead();
    void disconnected();
    void writeData(QList<MEX_Order> orderbook, QList<MEX_Order> matchedOrders);
    void requestUpdate();
    void receiveOrder(MEX_Order);

private slots:

    void setCurrentOrderbookData(QList<MEX_Order> orderbook, QList<MEX_Order> matchedOrders);

private:
    QTcpSocket* socket;
    qintptr socketDescriptor;
    QByteArray currentOrderbookData;
    QByteArray newData;
    bool abort;
    QElapsedTimer timer;
};

#endif // MEX_SERVERTHREAD_H
