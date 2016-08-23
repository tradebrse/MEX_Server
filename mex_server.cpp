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

    //Set SQL DB path
    QString dbPath = QCoreApplication::applicationDirPath() + "/persistence.sqlite";
    db = QSqlDatabase::addDatabase("QSQLITE", "persistence_connection");
    db.setDatabaseName(dbPath);

    //Set tradingstatus to closed
    open = false;
    //Set server date
    serverDate = QDate::currentDate();
}


MEX_Server::~MEX_Server()
{
    closeDB();
    delete perfMonSocket;
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


    ///Nicht sicher ob hier
    loadPersistentOrders();
}

void MEX_Server::loadPersistentOrders()
{
    bool ok = false;
    QString sqlCommand = "SELECT * FROM orders";
    QSqlQuery query = executeQuery(sqlCommand, ok);
    if (!ok)
    {
        //Error while executing SQL-Statement
        cout << "Error - Could not load persistent orders." << endl;
    }
    else
    {
        ////WEITERARBEITen &in mex_order tradable und pers hinzufügen!
        while(query.next())
        {
            MEX_Order newPersOrder;
            newPersOrder.setOrderID(query.value(0).toInt());
            newPersOrder.setProductSymbol(query.value(2).toString());
            newPersOrder.setQuantity(query.value(3).toInt());
            newPersOrder.setValue(query.value(4).toInt());
            newPersOrder.setOrdertype(query.value(5).toString());
            newPersOrder.setTime(QDateTime::fromString(query.value(6).toString(), "hh:mm:ss.zzz"));
            newPersOrder.setGTD(query.value(7).toString());
            newPersOrder.setComment(query.value(8).toString());
            newPersOrder.setTraderID(query.value(9).toString());
            newPersOrder.setPersistent(true);
            newPersOrder.setUpdated(0);
            // Persistent orders debug:
            /*qDebug()<<"persOrders: ";
            for(int i= 0; i < 10; i++)
            {
                qDebug() << i << " : " << query.value(i).toString();
            }
            */
            orderbook.append(newPersOrder);
        }
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
    else if(!open)
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

    //Set thread exchange status to the same as server
    thread->setExchangeStatus(open);

    //Start the thread
    thread->start();
}

void MEX_Server::getOrder(MEX_Order newOrder)
{
    //Nanosec timer start
    timer.start();

    newOrder.setOrderID(++lastOrderID);

    //GetOrder//

    //Write current ID and Timestamp to Performance Monitor Sockets
    writeToPerfMon("ID:"+QString::number(lastOrderID)+"-TG:"+QDateTime::currentDateTime().toString("hh:mm:ss.zzz")+"-NS:"+QString::number(timer.nsecsElapsed()));

    //Timer GetOrder restart
    timer.restart();

    //Check if order GTD date fits server date / Else no match
    if (QDate::fromString(newOrder.getGTD(),Qt::ISODate) >= serverDate || newOrder.getGTD() == "")
    {
        addOrder(newOrder);
    }
    else
    {
        //Write skipped timestamps
        //for sortOB
        writeToPerfMon("ID:"+QString::number(lastOrderID)+"-TS:"+QDateTime::currentDateTime().toString("hh:mm:ss.zzz")+"-NS:"+QString::number(timer.nsecsElapsed()));
        timer.restart();
        //for checkForMatch
        writeToPerfMon("ID:"+QString::number(lastOrderID)+"-TM:"+QDateTime::currentDateTime().toString("hh:mm:ss.zzz")+"-NS:"+QString::number(timer.nsecsElapsed()));
        timer.restart();
        //for addOrder
        writeToPerfMon("ID:"+QString::number(lastOrderID)+"-TA:"+QDateTime::currentDateTime().toString("hh:mm:ss.zzz")+"-NS:"+QString::number(timer.nsecsElapsed()));
        timer.restart();
    }
    /// Else return Invalid Date... ?

    //Broadcast//

    //Write current ID and Timestamp to Performance Monitor Socket
    writeToPerfMon("ID:"+QString::number(lastOrderID)+"-TB:"+QDateTime::currentDateTime().toString("hh:mm:ss.zzz")+"-NS:"+QString::number(timer.nsecsElapsed()));

    //Timer Broadcast restart
    timer.restart();

    emit broadcastData(orderbook, matchedOrders);

    //EndOfProcess//

    //Write current ID and Timestamp to Performance Monitor Socket
    writeToPerfMon("ID:"+QString::number(lastOrderID)+"-TE:"+QDateTime::currentDateTime().toString("hh:mm:ss.zzz")+"-NS:"+QString::number(timer.nsecsElapsed()));
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
    else
    {
        //Write skipped timestamps
        //for sortOB
        writeToPerfMon("ID:"+QString::number(lastOrderID)+"-TS:"+QDateTime::currentDateTime().toString("hh:mm:ss.zzz")+"-NS:"+QString::number(timer.nsecsElapsed()));
        timer.restart();
        //for checkForMatch
        writeToPerfMon("ID:"+QString::number(lastOrderID)+"-TM:"+QDateTime::currentDateTime().toString("hh:mm:ss.zzz")+"-NS:"+QString::number(timer.nsecsElapsed()));
        timer.restart();
    }

    //Add Order//

    //Write current ID and Timestamp to Performance Monitor Socket
    writeToPerfMon("ID:"+QString::number(lastOrderID)+"-TA:"+QDateTime::currentDateTime().toString("hh:mm:ss.zzz")+"-NS:"+QString::number(timer.nsecsElapsed()));

    //If there is no match for the order, then add it to the orderbook
    if(match == false)
    {
        //Set update highlight for complete new order book order
        order.setUpdated(7);
        orderbook.append(order);
        if(order.isPersistent())
        {
            addPersistentOrder(order);
        }
    }
}

void MEX_Server::addPersistentOrder(MEX_Order newOrder)
{
//Get Persistency
///only if no match...
if(newOrder.isPersistent())
{
    //Add persistent order to database...
    cout << "Persistent" << endl;

    bool ok = false;

    QString sqlCommand = "INSERT INTO orders (orderID, traderID, indexName, symbol, quantity, value, buysell, time, gtd, comment) SELECT '" + QString::number(newOrder.getOrderID()) + "', '" + newOrder.getTraderID() + "', indexName, '" + newOrder.getProductSymbol() + "', '" + QString::number(newOrder.getQuantity()) + "', '" + QString::number(newOrder.getValue()) + "', '" + newOrder.getOrdertype() + "', '" + newOrder.getTime().toString("hh:mm:ss.zzz") + "', '" + newOrder.getGTD() + "', '" + newOrder.getComment() + "' FROM productList WHERE symbol = '" + newOrder.getProductSymbol() + "' ";
    executeQuery(sqlCommand, ok);
    if (!ok)
    {
        //Error while executing SQL-Statement
        cout << "Error - Could not execute query." <<  endl;
    }
}
}
void MEX_Server::removePersistentOrder(int id)
{
    bool ok = false;
    QString sqlCommand = "DELETE FROM orders WHERE orderID='"+QString::number(id)+"'";
    QSqlQuery query = executeQuery(sqlCommand, ok);
    if (!ok)
    {
        //Error while executing SQL-Statement
        cout << "Error - Could not remove persistent order." << endl;
    }
    else
    {
        ///irgendwas? oder nichts?
    }
}

void MEX_Server::updatePersistentOrder(int id, int newQuantity)
{
    bool ok = false;
    QString sqlCommand = "UPDATE orders SET quantity='"+QString::number(newQuantity)+"' WHERE orderID='"+QString::number(id)+"'";
    QSqlQuery query = executeQuery(sqlCommand, ok);
    if (!ok)
    {
        //Error while executing SQL-Statement
        cout << "Error - Could not remove persistent order." << endl;
    }
    else
    {
        ///irgendwas? oder nichts?
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
            removePersistentOrder(id.toInt());
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
    writeToPerfMon("ID:"+QString::number(lastOrderID)+"-TS:"+QDateTime::currentDateTime().toString("hh:mm:ss.zzz")+"-NS:"+QString::number(timer.nsecsElapsed()));

    //Timer SortOB restart
    timer.restart();

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
    writeToPerfMon("ID:"+QString::number(lastOrderID)+"-TM:"+QDateTime::currentDateTime().toString("hh:mm:ss.zzz")+"-NS:"+QString::number(timer.nsecsElapsed()));

    //Timer Match restart
    timer.restart();

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
                        //delete pers orders from db
                        if((*i).isPersistent())
                        {
                            removePersistentOrder((*i).getOrderID());
                        }
                        ordersToDelete.append(orderbook.indexOf((*i)));
                        matchedOrders.append(MEX_Order((*i)));
                        matchedOrders.append(MEX_Order(order));
                        order.setQuantity(0);
                    }
                    else
                    {
                        if((*i).isPersistent())
                        {
                            updatePersistentOrder((*i).getOrderID(), newQuantity);
                        }
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
                    //delete pers orders from db
                    if((*i).isPersistent())
                    {
                        removePersistentOrder((*i).getOrderID());
                    }
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
        if(order.isPersistent())
        {
            addPersistentOrder(order);
        }
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


//SQL DATABASE / CONNECTION
QSqlQuery MEX_Server::executeQuery (QString sqlCommand, bool &ok)
{
    if (!db.open())
    {
        cout << "Error - No database connection." << endl;
        QSqlQuery emptyQuery;
        return emptyQuery;
    } else
    {
        QSqlQuery query(db);
        ok = query.exec(sqlCommand);
        return query;
    }
}

void MEX_Server::closeDB()
{
    //Get connection name
    QString connection;
    connection = db.connectionName();
    //Close connection
    db.close();
    db = QSqlDatabase();
    //remove old connection
    db.removeDatabase(connection);
}
