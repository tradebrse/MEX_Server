#ifndef MEX_XMLPROCESSOR_H
#define MEX_XMLPROCESSOR_H

#include <QObject>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <mex_order.h>

class MEX_XMLProcessor : public QObject
{
    Q_OBJECT
public:
    explicit MEX_XMLProcessor(QObject *parent = 0);
     ~MEX_XMLProcessor();
signals:
    void receivedOrder(MEX_Order newOrder);
    void updateRequest();
public slots:
    void processRead(QByteArray data);
    QByteArray processWrite(QByteArray *data, QList<MEX_Order> orderbook, QList<MEX_Order> matchedOrders,QString traderID);
private slots:
    void readOrderElements();
private:
    QXmlStreamReader *xmlReader;
    QXmlStreamWriter *xmlWriter;
};

#endif // MEX_XMLPROCESSOR_H
