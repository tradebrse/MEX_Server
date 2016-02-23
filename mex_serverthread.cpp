#include "mex_serverthread.h"

using namespace std;

MEX_ServerThread::MEX_ServerThread(qintptr ID, QByteArray currentOrderbookData, QObject *parent) :
    QThread(parent)
{
    abort = false;
    this->socketDescriptor = ID;
    if(!currentOrderbookData.isNull())
    {
        this->currentOrderbookData = currentOrderbookData;
    }
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

        //Write current Orderbook Data to the client so that it is up to date
        socket->write(currentOrderbookData);

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

    MEX_XMLProcessor* xmlProcessor = new MEX_XMLProcessor();
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
    //Send Data to MyServer
    receivedOrder(newOrder);
}

void MEX_ServerThread::writeData(QByteArray currentOrderbook)
{
    socket->write(currentOrderbook);
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

