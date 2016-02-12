#include <QCoreApplication>
#include <mex_server.h>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    //Create a server and start it
    MEX_Server server;
    server.startServer();

    return a.exec();
}
