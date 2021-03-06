#include "mex_order.h"
#include <QDebug>

MEX_Order::MEX_Order()
{
}

MEX_Order::MEX_Order(QString traderID, double value, int quantity, QString comment, QString productSymbol, QString ordertype)
{
    this->traderID = traderID;
    this->value = value;
    this->quantity = quantity;
    this->comment = comment;
    this->productSymbol = productSymbol;
    this->ordertype = ordertype;
    this->time = QDateTime::currentDateTime();
    this->updated = 0;
    this->gtd = "";
    this->persistent = false; /// Ändern zu konstrutktor wert!!
    ////tradable hinzufügen!!!
}

//Copy constructor
MEX_Order::MEX_Order(const MEX_Order &other)
{
    this->traderID = other.traderID;
    this->orderID = other.orderID;
    this->value = other.value;
    this->quantity = other.quantity;
    this->comment = other.comment;
    this->productSymbol = other.productSymbol;
    this->ordertype = other.ordertype;
    this->time = other.time;
    this->updated = other.updated;
    this->gtd = other.gtd;
    this->persistent = other.persistent;
}

MEX_Order::~MEX_Order()
{
}

//Initializer for XML Reader
void MEX_Order::initialize(QString traderID, double value, int quantity, QString comment, QString productSymbol, QString ordertype, QString gtd, bool persistent)
{
    this->traderID = traderID;
    this->value = value;
    this->quantity = quantity;
    this->comment = comment;
    this->productSymbol = productSymbol;
    this->ordertype = ordertype;
    this->time = QDateTime::currentDateTime();
    this->updated = 0;
    this->gtd = gtd;
    this->persistent = persistent;
}

//Getter methods
QString MEX_Order::getTraderID() const
{
    return this->traderID;
}
QString MEX_Order::getOrdertype() const
{
    return this->ordertype;
}
int MEX_Order::getOrderID() const
{
    return this->orderID;
}
double MEX_Order::getValue() const
{
    return this->value;
}
int MEX_Order::getQuantity() const
{
    return this->quantity;
}
QString MEX_Order::getComment() const
{
    return this->comment;
}
QDateTime MEX_Order::getTime() const
{
    return this->time;
}

QString MEX_Order::getProductSymbol() const
{
    return this->productSymbol;
}

int MEX_Order::getUpdated() const
{
    return this->updated;
}

bool MEX_Order::isTradable() const
{
    return this->tradable;
}

QString MEX_Order::getGTD() const
{
    return this->gtd;
}

bool MEX_Order::isPersistent() const
{
    return this->persistent;
}

//Setter methods
void MEX_Order::setTraderID(QString traderID)
{
    this->traderID = traderID;
}

void MEX_Order::setOrderID(int orderID)
{
    this->orderID = orderID;
}

void MEX_Order::setValue(double value)
{
    this->value = value;
}

void MEX_Order::setQuantity(int quantity)
{
    this->quantity = quantity;
}

void MEX_Order::setComment(QString comment)
{
    this->comment = comment;
}

void MEX_Order::setProductSymbol(QString productSymbol)
{
    this->productSymbol = productSymbol;
}

void MEX_Order::setOrdertype(QString ordertype)
{
    this->ordertype = ordertype;
}

void MEX_Order::setTime(QDateTime time)
{
    this->time = time;
}

void MEX_Order::setUpdated(int updated)
{
    this->updated = updated;
}

void MEX_Order::setTradable(bool tradable)
{
    this->tradable = tradable;
}

void MEX_Order::setGTD(QString gtd)
{
    this->gtd = gtd;
}

void MEX_Order::setPersistent(bool persistent)
{
    this->persistent = persistent;
}

bool MEX_Order::operator==(const MEX_Order &order) const {
    if(order.getOrderID() == this->getOrderID())
    {
        return true;
    }
    else
    {
        return false;
    }
}
