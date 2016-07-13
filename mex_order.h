#ifndef MEX_ORDER_H
#define MEX_ORDER_H
#include <QDateTime>
#include <iostream>


class MEX_Order
{
public:
    MEX_Order();
    MEX_Order(QString traderID, double value, int quantity, QString comment, QString productSymbol, QString ordertype);
    MEX_Order(const MEX_Order &other);
    ~MEX_Order();


    //Initializer for XML Reader
    void initialize(QString traderID, double value, int quantity, QString comment, QString productSymbol, QString ordertype, QString gtd, bool persistent);
    // Getter methods
    QString getTraderID() const;
    QString getOrdertype() const;
    int getOrderID() const;
    double getValue() const;
    int getQuantity() const;
    QDateTime getTime() const;
    QString getComment() const;
    QString getProductSymbol() const;
    int getUpdated() const;
    bool isTradable() const;
    QString getGTD() const;
    bool isPersistent() const;

    //Setter methods
    void setTraderID(QString);
    void setOrderID(int);
    void setValue(double);
    void setQuantity(int);
    void setComment(QString);
    void setProductSymbol(QString);
    void setUpdated(int);
    void setTradable(bool);
    void setGTD(QString gtd);
    void setPersistent(bool persistent);

    bool operator==(const MEX_Order &order) const;
private:

    QString traderID;
    int orderID;
    double value;
    int quantity;
    QString comment;
    QDateTime time;
    QString ordertype;
    QString productSymbol;
    int updated;
    bool tradable;
    QString gtd;
    bool persistent;
};

#endif //MEX_ORDER_H
