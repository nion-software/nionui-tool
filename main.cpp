/*
 Copyright (c) 2012-2015 Nion Company.
*/

#include "Application.h"

#if USE_THRIFT
#include "GUI_server.cpp"
#endif // USE_THRIFT

int main(int argv, char **args)
{
    Application app(argv, args);

#if USE_THRIFT
    int port = 9090;
    boost::shared_ptr<QTcpServer> tcp_server(new QTcpServer());
    if (!tcp_server->listen(QHostAddress::Any, port))
    {
        // throw exception
        return 0;
    }
    shared_ptr<GUIAsyncHandler> handler(new GUIAsyncHandler());
    shared_ptr<TAsyncProcessor> processor(new GUIAsyncProcessor(handler));
    shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

    boost::shared_ptr<TQTcpServer> server(new TQTcpServer(tcp_server, processor, protocolFactory));
#endif // USE_THRIFT

    if (app.initialize())
        return app.exec();

    return 0;
}
