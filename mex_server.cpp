#include "mex_server.h"

using namespace std;

MEX_Server::MEX_Server(QObject *parent) :
    QTcpServer(parent)
{
    //Register MEX_Order so that we can use it with connect()
    qRegisterMetaType<MEX_Order>("MEX_Order");
    lastOrderID = 0;
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


//This function is called by QTcpServer when a new connection is aviable.

void MEX_Server::incomingConnection(qintptr socketDescriptor)
{
    //We have a new connection
    cout << socketDescriptor << " Connecting..." << endl;

    //Every new connection weill be run in a newly created thread
    MEX_ServerThread *thread = new MEX_ServerThread(socketDescriptor, currentOrderbookData);
    //Seperate the thread from the current(myserver) one
    thread->moveToThread(thread);

    //Connect incoming Data from thread to other threads
    connect(thread,SIGNAL(receivedOrder(MEX_Order)),this,SLOT(getOrder(MEX_Order)));
    connect(this, SIGNAL(broadcastData(QByteArray)),thread,SLOT(writeData(QByteArray)));
    connect(thread, SIGNAL(updateRequest()),this,SLOT(requestUpdate()));

    //Once a thread is not needed, it will be deleted later
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    //Start the thread
    thread->start();
}

void MEX_Server::getOrder(MEX_Order newOrder)
{
    newOrder.setOrderID(++lastOrderID);

    addOrder(newOrder);

    currentOrderbookData.clear();
    MEX_XMLProcessor *xmlProcessor = new MEX_XMLProcessor;
    QByteArray *pointerOrderbookData = &currentOrderbookData;
    currentOrderbookData = xmlProcessor->processWrite(pointerOrderbookData, orderbook);

    if(!currentOrderbookData.isNull())
    {
        emit broadcastData(currentOrderbookData);
    }
}

void MEX_Server::requestUpdate()
{
    if(!currentOrderbookData.isNull())
    {
        emit broadcastData(currentOrderbookData);
    }
}

//Try to match the order and update the orderbook
void MEX_Server::addOrder(MEX_Order order)
{
    bool match = false;
    //Check if orderbook is not empty and did not already (partly) match
    if (orderbook.size() > 0 && skipMatch == false)
    {
        match = checkForMatch(order);
    }
    //If there is no match for the order, then add it to the orderbook
    if(match == false)
    {
        orderbook.append(order);
    }
}

//Look for matching orders in orderbook
bool MEX_Server::checkForMatch(MEX_Order &order)
{
    bool match = false;
    QList<MEX_Order>::iterator i;

    for( i = orderbook.begin(); i != orderbook.end() && order.getQuantity() > 0; ++i)
    {
        //Are the orders of the same product?
        if(order.getProductSymbol() ==  (*i).getProductSymbol())
        {
            //'BID' orders have to match with 'ASK' orders
            //Orderbook orders must be the same value or lower(new 'BID' order) / higher(new 'ASK' order)
            if((order.getOrdertype() == "BID" && (*i).getOrdertype() == "ASK" && order.getValue() >= (*i).getValue() )|| (order.getOrdertype() == "ASK" && (*i).getOrdertype() == "BID" && order.getValue() <= (*i).getValue()))
            {
                match = true;
                //The order quantity is less or the same as the current one from the orderbook
                if((*i).getQuantity() >= order.getQuantity())
                {
                    int newQuantity = (*i).getQuantity() - order.getQuantity();
                    if (newQuantity == 0)
                    {
                        const MEX_Order indexOrder = (*i);
                        ordersToDelete.append(orderbook.indexOf(indexOrder));
                        order.setQuantity(0);
                        ///Matched orders irgendwo irgendwie speichern
                    }
                    else
                    {
                        order.setQuantity(0);
                        (*i).setQuantity(newQuantity);
                    }
                }
                else if ( order.getQuantity() > (*i).getQuantity())
                {
                    int newQuantity = order.getQuantity() - (*i).getQuantity();
                    ordersToDelete.append(orderbook.indexOf((*i)));
                    order.setQuantity(newQuantity);
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
    //If parts of the original order are left, set match to false so that it is added to the orderbook
    if (match == true && order.getQuantity() > 0)
    {
        match = false;
    }

    return match;
}

