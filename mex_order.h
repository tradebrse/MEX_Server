#ifndef MEX_ORDER_H
#define MEX_ORDER_H
#include <QDateTime>
#include <iostream>


class MEX_Order
{
public:
    MEX_Order();
    MEX_Order(QString traderID, int value, int quantity, QString comment, QString productSymbol, QString ordertype);
    MEX_Order(const MEX_Order &other);
    ~MEX_Order();


    //Initializer for XML Reader
    void initialize(QString traderID, int value, int quantity, QString comment, QString productSymbol, QString ordertype);
    // Getter methods
    QString getTraderID() const;
    QString getOrdertype() const;
    int getOrderID() const;
    int getValue() const;
    int getQuantity() const;
    QDateTime getTime() const;
    QString getComment() const;
    QString getProductSymbol() const;
    //Setter methods
    void setTraderID(QString);
    void setOrderID(int);
    void setValue(int);
    void setQuantity(int);
    void setComment(QString);
    void setProductSymbol(QString);

    bool operator==(const MEX_Order &order) const;
private:

    QString traderID;
    int orderID;
    int value;
    int quantity;
    QString comment;
    QDateTime time;
    QString ordertype;
    QString productSymbol;

};

#endif //MEX_ORDER_H
