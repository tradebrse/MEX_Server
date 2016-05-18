#include "mex_server.h"
#include "QDebug"
static inline QByteArray IntToArray(qint32 source);

using namespace std;

MEX_Server::MEX_Server(QObject *parent) :
    QTcpServer(parent)
{
    //Register MEX_Order so that we can use it with connect()
    qRegisterMetaType<MEX_Order>("MEX_Order");
    qRegisterMetaType< QList<MEX_Order> >("QList<MEX_Order>");
    lastOrderID = 0;
    perfMonSocket = new QTcpSocket(this);

    perfMonSocket->connectToHost("127.0.0.1", 4444);

    if(perfMonSocket->waitForConnected()){
        cout << "Performance Monitor connected" << endl;
    }
    else
    {
        cout << "Could not connect to Performance Monitor" << endl;
    }

    //Set tradingstatus to closed
    open = false;
    //Set server date
    serverDate = QDate::currentDate();
}


void MEX_Server::startServer()
{
    int port = 1234;

    if(!this->listen(QHostAddress::Any, port))
    {
        cout << "Could not start server" << endl;
    }
    else
    {
        cout << "Listening to port " << port << "...." << endl;
    }
}

void MEX_Server::openExchange(bool open)
{
    this->open = open;

    //Send current orderbook if SOD
    if(open)
    {
        cout << serverDate.toString(Qt::ISODate).toStdString() << endl;

        emit broadcastRawData(open);
        emit broadcastData(orderbook, matchedOrders);
    }

    //Delete GTD Orders if EOD
    if(!open)
    {
        //End of Processing
        removeGTDOrders();
        serverDate = serverDate.addDays(1);
        emit broadcastRawData(open);
    }
}


//This function is called by QTcpServer when a new connection is aviable.
void MEX_Server::incomingConnection(qintptr socketDescriptor)
{
    //We have a new connection
    cout << socketDescriptor << " Connecting..." << endl;
    //Every new connection weill be run in a newly created thread
    MEX_ServerThread *thread = new MEX_ServerThread(socketDescriptor);
    //Seperate the thread from the current(myserver) one
    thread->moveToThread(thread);
    //Connect incoming Data from thread to other threads
    connect(thread,SIGNAL(receivedOrder(MEX_Order)),this,SLOT(getOrder(MEX_Order)));
    connect(this, SIGNAL(broadcastData(QList<MEX_Order>, QList<MEX_Order>)),thread,SLOT(writeData(QList<MEX_Order>, QList<MEX_Order>)));
    connect(this, SIGNAL(broadcastRawData(bool)),thread,SLOT(writeRawData(bool)));
    connect(thread, SIGNAL(updateRequest()),this,SLOT(requestUpdate()));
    connect(thread, SIGNAL(exchangeOpen(bool)), this, SLOT(openExchange(bool)));
    connect(thread, SIGNAL(disconnect()), thread, SLOT(deleteLater()));
    connect(thread, SIGNAL(removeOrder(QString)), this, SLOT(removeOrder(QString)));

    //Once a thread is not needed, it will be deleted later
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    //Send current exchange status to new clients
    emit broadcastRawData(open);
    //Start the thread
    thread->start();
}

void MEX_Server::getOrder(MEX_Order newOrder)
{
    newOrder.setOrderID(++lastOrderID);

    //GetOrder//

    //Write current ID and Timestamp to Performance Monitor Sockets
    writeToPerfMon("ID:"+QString::number(lastOrderID)+"-TG:"+QDateTime::currentDateTime().toString("hh:mm:ss.zzz"));

    addOrder(newOrder);

    //Broadcast//

    //Write current ID and Timestamp to Performance Monitor Socket
    writeToPerfMon("ID:"+QString::number(lastOrderID)+"-TB:"+QDateTime::currentDateTime().toString("hh:mm:ss.zzz"));

    emit broadcastData(orderbook, matchedOrders);

    //EndOfProcess//

    //Write current ID and Timestamp to Performance Monitor Socket
    writeToPerfMon("ID:"+QString::number(lastOrderID)+"-TE:"+QDateTime::currentDateTime().toString("hh:mm:ss.zzz"));

}

void MEX_Server::requestUpdate()
{
    if(!orderbook.isEmpty())
    {
        emit broadcastData(orderbook, matchedOrders);
    }
}

//Try to match the order and update the orderbook
void MEX_Server::addOrder(MEX_Order order)
{
    bool match = false;
    //Check if orderbook is not empty and did not already (partly) match
    if (orderbook.size() > 0)
    {
        match = checkForMatch(order);
    }

    //Add Order//

    //Write current ID and Timestamp to Performance Monitor Socket
    writeToPerfMon("ID:"+QString::number(lastOrderID)+"-TA:"+QDateTime::currentDateTime().toString("hh:mm:ss.zzz"));

    //If there is no match for the order, then add it to the orderbook
    if(match == false)
    {
        //Set update highlight for complete new order book order
        order.setUpdated(7);
        orderbook.append(order);
    }
}

void MEX_Server::removeOrder(QString id)
{
    //Iterate orderbook list and delete order with matching 'ID'
    bool removed = false;
    QList<MEX_Order>::iterator removeIterator;
    for(removeIterator = orderbook.begin(); removeIterator != orderbook.end() && !removed; ++removeIterator)
    {
        if((*removeIterator).getOrderID() == id.toInt())
        {
            removeIterator = orderbook.erase(removeIterator);
            removed = true;
        }
        //If the iterator reached end of list, set it back
        if(removeIterator == orderbook.end())
        {
            removeIterator--;
        }
    }
    emit broadcastData(orderbook,matchedOrders);
}

void MEX_Server::removeGTDOrders()
{
    QList<MEX_Order>::iterator it = orderbook.begin();
    while (it != orderbook.end()) {
      if (QDate::fromString((*it).getGTD(),Qt::ISODate)  <= serverDate && (*it).getGTD() != "")
        it = orderbook.erase(it);
      else
        ++it;
    }
}

bool orderLessThan(const MEX_Order &o1, const MEX_Order &o2)
{
    if(o1.getValue() == o2.getValue())
    {
        return o1.getTime() < o2.getTime();
    }
    else
    {
        return o1.getValue() < o2.getValue();
    }
}

//Look for matching orders in orderbook
bool MEX_Server::checkForMatch(MEX_Order &order)
{
    bool match = false;
    QList<MEX_Order>::iterator i;

    //SortOB//

    //Write current ID and Timestamp to Performance Monitor Socket
    writeToPerfMon("ID:"+QString::number(lastOrderID)+"-TS:"+QDateTime::currentDateTime().toString("hh:mm:ss.zzz"));

    if(order.getOrdertype() == "BUY")
    {
        qSort(orderbook.begin(),orderbook.end(),orderLessThan);
    }
    else if (order.getOrdertype() == "SELL")
    {
        qSort(orderbook.begin(),orderbook.end(),orderLessThan);
    }
    for( i = orderbook.begin(); i != orderbook.end(); ++i)
    {
        //Reset all update status values to 0
        (*i).setUpdated(0);
    }

    //Match//

    //Write current ID and Timestamp to Performance Monitor Socket
    writeToPerfMon("ID:"+QString::number(lastOrderID)+"-TM:"+QDateTime::currentDateTime().toString("hh:mm:ss.zzz"));

    for( i = orderbook.begin(); i != orderbook.end() && order.getQuantity() > 0; ++i)
    {
        //Are the orders of the same product type?
        if(order.getProductSymbol() == (*i).getProductSymbol())
        {
            //'BUY' orders have to match with 'SELL' orders
            //Orderbook orders must be the same value or lower(new 'BUY' order) / higher(new 'SELL' order)
            if((order.getOrdertype() == "BUY" && (*i).getOrdertype() == "SELL" && order.getValue() >= (*i).getValue() )|| (order.getOrdertype() == "SELL" && (*i).getOrdertype() == "BUY" && order.getValue() <= (*i).getValue()))
            {
                match = true;
                //The order quantity is less or the same as the current one from the orderbook
                if((*i).getQuantity() >= order.getQuantity())
                {
                    int newQuantity = (*i).getQuantity() - order.getQuantity();
                    if (newQuantity == 0)
                    {
                        ordersToDelete.append(orderbook.indexOf((*i)));
                        matchedOrders.append(MEX_Order((*i)));
                        matchedOrders.append(MEX_Order(order));
                        order.setQuantity(0);
                    }
                    else
                    {
                        matchedOrders.append(MEX_Order(order));
                        MEX_Order setOrder = MEX_Order((*i));
                        setOrder.setQuantity(order.getQuantity());
                        matchedOrders.append(setOrder);
                        order.setQuantity(0);
                        (*i).setQuantity(newQuantity);
                        //int 3 tells to highlight quantity because of the table column nr.
                        (*i).setUpdated(3);
                    }
                }
                else if ( order.getQuantity() > (*i).getQuantity())
                {
                    matchedOrders.append(MEX_Order((*i)));
                    MEX_Order setOrder = MEX_Order(order);
                    setOrder.setQuantity((*i).getQuantity());
                    matchedOrders.append(setOrder);
                    int newQuantity = order.getQuantity() - (*i).getQuantity();
                    ordersToDelete.append(orderbook.indexOf((*i)));
                    order.setQuantity(newQuantity);
                    //int 7 tells to highlight all columns
                    order.setUpdated(7);
                    //Go on searching for other orders in orderbook
                }
            }
        }
    }
    if (match == true && ordersToDelete.length() > 0)
    {
        //Delete orders from list
        for(int i = ordersToDelete.length()-1; i >= 0; i--)
        {
            orderbook.removeAt(ordersToDelete.value(i));
        }
        ordersToDelete.clear();
    }
    //If parts of the original order are left,  addto the order book
    if (match == true && order.getQuantity() > 0)
    {
        orderbook.append(order);
    }
    return match;
}

bool MEX_Server::writeToPerfMon(QString dataString)
{
    QByteArray data = dataString.toUtf8();
    //Write current ID and Timestamp to Performance Monitor Socket
    if(perfMonSocket->state() == QAbstractSocket::ConnectedState)
    {
        perfMonSocket->write(IntToArray(data.size())); //write size of data
        perfMonSocket->write(data); //write the data itself
        return perfMonSocket->waitForBytesWritten();
    }
    else
        return false;
}

QByteArray IntToArray(qint32 source) //Use qint32 to ensure that the number have 4 bytes
{
    //Avoid use of cast, this is the Qt way to serialize objects
    QByteArray temp;
    QDataStream data(&temp, QIODevice::ReadWrite);
    data << source;
    return temp;
}
