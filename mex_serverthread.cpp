#include "mex_serverthread.h"

using namespace std;

MEX_ServerThread::MEX_ServerThread(qintptr socketDescriptor, QObject *parent) :
    QThread(parent)
{
    abort = false;
    this->socketDescriptor = socketDescriptor;
}

void MEX_ServerThread::setCurrentOrderbookData(QList<MEX_Order> orderbook, QList<MEX_Order> matchedOrders)
{
    //Reset the old order book bytearray
    currentOrderbookData.clear();
    MEX_XMLProcessor *xmlProcessor = new MEX_XMLProcessor;
    //Set pointer to order book bytearray
    QByteArray *pointerOrderbookData = &currentOrderbookData;
    //Write order book nad matched orders to bytearray as XML
    currentOrderbookData = xmlProcessor->processWrite(pointerOrderbookData, orderbook, matchedOrders, socket->objectName());
}

void MEX_ServerThread::run()
{
    //Thread starts here

    //Abort is true when the client disconnected
    while(!abort)
    {
        //Create a new socket for the client
        socket = new QTcpSocket(this);

        //Set the ID
        if(!socket->setSocketDescriptor(this->socketDescriptor))
        {
            //Something's wrong, we just emit a signal
            emit error(socket->error());
            cout << "Some error occurred" << endl;
            return;
        }

        //Connect socket and signal
        //Note - QT::DirectConnection is used because it's multithreaded
        //This makes the slot to be invoked immediately, when the signal is emitted
        connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()),Qt::DirectConnection);
        connect(socket, SIGNAL(disconnected()), this, SLOT(disconnected()));


        //Wel'll have multiple clients, we want to know which is which
        cout << socketDescriptor << " Client connected" << endl;

        //Make this thread a loop
        //Thread will stay alive so thjat signal/slot to function properly
        //Not dropped out in the middle when a thread dies
        exec();
    }
}

void MEX_ServerThread::readyRead()
{
    //read Data from socket
    newData = socket->readAll();

    MEX_XMLProcessor *xmlProcessor = new MEX_XMLProcessor();
    connect(xmlProcessor,SIGNAL(receivedOrder(MEX_Order)),this,SLOT(receiveOrder(MEX_Order)));
    connect(xmlProcessor,SIGNAL(updateRequest()),this,SLOT(requestUpdate()));

    xmlProcessor->processRead(newData);
}

void MEX_ServerThread::requestUpdate()
{
    //Send orderbook update request to server class
    emit updateRequest();
}

void MEX_ServerThread::receiveOrder(MEX_Order newOrder)
{
    if(newOrder.getQuantity() == 0)
    {
        //Detected initial order for trader ID -> set Trader ID
        this->socket->setObjectName(newOrder.getTraderID());
        requestUpdate();
    }
    else
    {
        //Send Data to MyServer
        receivedOrder(newOrder);
    }
}

void MEX_ServerThread::writeData(QList<MEX_Order> orderbook, QList<MEX_Order> matchedOrders)
{
    //Set current values for order book and matched orders
    setCurrentOrderbookData(orderbook, matchedOrders);

    //If there is data, send it to the client
    if(!currentOrderbookData.isNull())
    {
        socket->write(currentOrderbookData);
        socket->waitForBytesWritten(-1);
    }
}

void MEX_ServerThread::disconnected()
{
    cout << socketDescriptor << " Disconnected" << endl;
    socket->deleteLater();
    this->abort = true; //Tell the thread to abort
    sleep(5000); //wait until thread is aborted
    if(this->isRunning()){
        cout << "Thread deadlock detected, forcing termination" << endl;
        terminate(); //Thread didn't exit in time, probably deadlocked, terminate it!
        wait(5000); //Note: We have to wait again here!
    }
    exit(0);
}

