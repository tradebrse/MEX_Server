#include "mex_server.h"

using namespace std;

MEX_Server::MEX_Server(QObject *parent) :
    QTcpServer(parent)
{
    //Register MEX_Order so that we can use it with connect()
    qRegisterMetaType<MEX_Order>("MEX_Order");
    qRegisterMetaType< QList<MEX_Order> >("QList<MEX_Order>");
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
    MEX_ServerThread *thread = new MEX_ServerThread(socketDescriptor);
    //Seperate the thread from the current(myserver) one
    thread->moveToThread(thread);
    //Connect incoming Data from thread to other threads
    connect(thread,SIGNAL(receivedOrder(MEX_Order)),this,SLOT(getOrder(MEX_Order)));
    connect(this, SIGNAL(broadcastData(QList<MEX_Order>, QList<MEX_Order>)),thread,SLOT(writeData(QList<MEX_Order>, QList<MEX_Order>)));
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

    emit broadcastData(orderbook, matchedOrders);
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
    //If there is no match for the order, then add it to the orderbook
    if(match == false)
    {
        //Set update highlight for complete new order book order
        order.setUpdated(7);
        orderbook.append(order);
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
    for( i = orderbook.begin(); i != orderbook.end() && order.getQuantity() > 0; ++i)
    {
        //Are the orders of the same product type?
        if(order.getProductSymbol() ==  (*i).getProductSymbol())
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
                    //int 3 tells to highlight quantity because of the table column nr.
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

