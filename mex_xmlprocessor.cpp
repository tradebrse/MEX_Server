#include "mex_xmlprocessor.h"
#include <QDebug>

MEX_XMLProcessor::MEX_XMLProcessor(QObject *parent) :
    QObject(parent)
{
}

MEX_XMLProcessor::~MEX_XMLProcessor()
{
    delete xmlReader;
    delete xmlWriter;
}

void MEX_XMLProcessor::processRead(QByteArray data)
{
    xmlReader = new QXmlStreamReader(data);
    while(!xmlReader->atEnd())
    {
        //Read line
        xmlReader->readNext();
        if(xmlReader->isStartElement())
        {
            if(xmlReader->name() == "Order")
            {
                readOrderElements();
            }
        }
    }
}

QByteArray MEX_XMLProcessor::processWrite(QByteArray *data, QList<MEX_Order> orderbook, QList<MEX_Order> matchedOrders, QString traderID)
{
    xmlWriter = new QXmlStreamWriter(data);
    xmlWriter->setAutoFormatting(true);
    //Write start of orderbook document and set Order tag
    xmlWriter->writeStartDocument();
    //Start order lists xml
    xmlWriter->writeStartElement("Orderlists");
    xmlWriter->writeStartElement("Orderbook");
    foreach(MEX_Order order, orderbook)
    {
        xmlWriter->writeStartElement("Order");
        //Write trader ID
        xmlWriter->writeStartElement("Trader_ID");
        xmlWriter->writeCharacters(order.getTraderID());
        xmlWriter->writeEndElement();
        //Write order ID
        xmlWriter->writeStartElement("Order_ID");
        xmlWriter->writeCharacters(QString::number(order.getOrderID()));
        xmlWriter->writeEndElement();
        //Write value data
        xmlWriter->writeStartElement("Value");
        xmlWriter->writeCharacters(QString::number(order.getValue()));
        xmlWriter->writeEndElement();
        //Write quantity data
        xmlWriter->writeStartElement("Quantity");
        xmlWriter->writeCharacters(QString::number(order.getQuantity()));
        xmlWriter->writeEndElement();
        //Write comment data
        xmlWriter->writeStartElement("Comment");
        xmlWriter->writeCharacters(order.getComment());
        xmlWriter->writeEndElement();
        //Write product data
        xmlWriter->writeStartElement("Product");
        xmlWriter->writeCharacters(order.getProductSymbol());
        xmlWriter->writeEndElement();
        //Write ordertype data
        xmlWriter->writeStartElement("Order_Type");
        xmlWriter->writeCharacters(order.getOrdertype());
        xmlWriter->writeEndElement();
        //Write time data
        xmlWriter->writeStartElement("Time");
        xmlWriter->writeCharacters(order.getTime().toString("hh:mm:ss.zzz"));
        xmlWriter->writeEndElement();
        //Write updated data
        xmlWriter->writeStartElement("Update");
        xmlWriter->writeCharacters(QString::number(order.getUpdated()));
        xmlWriter->writeEndElement();
        //Write GTD data
        xmlWriter->writeStartElement("GTD");
        xmlWriter->writeCharacters(order.getGTD());
        xmlWriter->writeEndElement();
        //Close tag Order
        xmlWriter->writeEndElement();
    }
    //Close tag Orderbook
    xmlWriter->writeEndElement();

    //Seperate order boom and matched orders
    xmlWriter->writeStartElement("Separator");
    xmlWriter->writeEndElement();

    //Write start of matchedOrders document and set Order tag
    xmlWriter->writeStartElement("MatchedOrders");
    foreach(MEX_Order order, matchedOrders)
    {
        //Sort out trades of other users
        if(order.getTraderID() == traderID)
        {
            xmlWriter->writeStartElement("Order");
            //Write trader ID
            xmlWriter->writeStartElement("Trader_ID");
            xmlWriter->writeCharacters(order.getTraderID());
            xmlWriter->writeEndElement();
            //Write order ID
            xmlWriter->writeStartElement("Order_ID");
            xmlWriter->writeCharacters(QString::number(order.getOrderID()));
            xmlWriter->writeEndElement();
            //Write value data
            xmlWriter->writeStartElement("Value");
            xmlWriter->writeCharacters(QString::number(order.getValue()));
            xmlWriter->writeEndElement();
            //Write quantity data
            xmlWriter->writeStartElement("Quantity");
            xmlWriter->writeCharacters(QString::number(order.getQuantity()));
            xmlWriter->writeEndElement();
            //Write comment data
            xmlWriter->writeStartElement("Comment");
            xmlWriter->writeCharacters(order.getComment());
            xmlWriter->writeEndElement();
            //Write product data
            xmlWriter->writeStartElement("Product");
            xmlWriter->writeCharacters(order.getProductSymbol());
            xmlWriter->writeEndElement();
            //Write ordertype data
            xmlWriter->writeStartElement("Order_Type");
            xmlWriter->writeCharacters(order.getOrdertype());
            xmlWriter->writeEndElement();
            //Write time data
            xmlWriter->writeStartElement("Time");
            xmlWriter->writeCharacters(order.getTime().toString("hh:mm:ss.zzz"));
            xmlWriter->writeEndElement();
            //Write updated data
            xmlWriter->writeStartElement("Update");
            xmlWriter->writeCharacters(QString::number(order.getUpdated()));
            xmlWriter->writeEndElement();
            //Write GTD data
            xmlWriter->writeStartElement("GTD");
            xmlWriter->writeCharacters(order.getGTD());
            xmlWriter->writeEndElement();
            //Close tag Order
            xmlWriter->writeEndElement();
        }
    }
    //Close tag matched orders
    xmlWriter->writeEndElement();

    //Close tag order lists
    xmlWriter->writeEndElement();
    //Close document
    xmlWriter->writeEndDocument();

    QByteArray nonPointerData = *data;
    return  nonPointerData;
}


void MEX_XMLProcessor::readOrderElements()
{
    //Declare order variables
    QString traderID, comment, productSymbol, ordertype;
    double value = 0.0;
    int quantity = 0;
    QString gtd = "";
    bool persistent = false;

    //Initialise order variables
    while(!xmlReader->atEnd())
    {
        xmlReader->readNext();

        if (xmlReader->isStartElement())
        {
            if (xmlReader->name() == "Trader_ID")
            {
                traderID = xmlReader->readElementText();
            }
            else if (xmlReader->name() == "Value")
            {
                value = xmlReader->readElementText().toDouble();
            }
            else if (xmlReader->name() == "Quantity")
            {
                quantity = xmlReader->readElementText().toInt();
            }
            else if (xmlReader->name() == "Comment")
            {
                comment = xmlReader->readElementText();
            }
            else if (xmlReader->name() == "Product")
            {
                productSymbol = xmlReader->readElementText();
            }
            else if (xmlReader->name() == "Order_Type")
            {
                ordertype = xmlReader->readElementText();
            }
            else if (xmlReader->name() == "GTD")
            {
                gtd = xmlReader->readElementText();
            }
            else if (xmlReader->name() == "Persistent")
            {
                if(xmlReader->readElementText() == "1") persistent = true;
            }
        }
    }
    //Create order object
    MEX_Order incomingOrder;
    incomingOrder.initialize(traderID, value, quantity, comment, productSymbol, ordertype, gtd, persistent);
    //Send Data to Serverthread : receiveOrder
    emit receivedOrder(incomingOrder);
}
