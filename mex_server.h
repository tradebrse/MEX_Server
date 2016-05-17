#ifndef MEX_SERVER_H
#define MEX_SERVER_H

#include <QTcpServer>
#include <iostream>
#include <mex_serverthread.h>
#include <mex_order.h>
#include <mex_xmlprocessor.h>
#include <QDebug>
#include <QTcpSocket>
#include <QDataStream>


class MEX_Server : public QTcpServer
{
    Q_OBJECT
public:
    explicit MEX_Server(QObject *parent = 0);
    void startServer();

signals:
    void broadcastData(QList<MEX_Order>, QList<MEX_Order>);
    void broadcastRawData(bool);

public slots:
    void getOrder(MEX_Order newOrder);
    void requestUpdate();
    void openExchange(bool);
    void removeOrder(QString id);
    void removeGTDOrders();

private slots:
    bool checkForMatch(MEX_Order &order);
    void addOrder(MEX_Order order);
    bool writeToPerfMon(QString dataString);

protected:
    void incomingConnection(qintptr socketDescriptor);

private:
    int lastOrderID;
    //'Data' variables are QByteArrays
    QList<MEX_Order> orderbook;
    QVarLengthArray<int> ordersToDelete;
    QList<MEX_Order> matchedOrders;
    QTcpSocket* perfMonSocket;
    bool open;
    QDate serverDate;
};

#endif // MEX_SERVER_H
