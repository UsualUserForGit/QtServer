#include "server.h"

Server::Server(QObject *parent) : QObject(parent)
{

}

void Server::startServer()
{
    // store all the connected clients
    allClients = new QVector<QTcpSocket*>;
    // created a QTcpServer object called chatServer
    chatServer = new QTcpServer();
    // limit the maximum pending connections to 10 clients.
    chatServer->setMaxPendingConnections(10);
    // The chatServer will trigger the newConnection() signal whenever a client has connected to the server.
    connect(chatServer, SIGNAL(newConnection()), this, SLOT(newClientConnection()));
    // made it constantly listen to port 8001.
    if (chatServer->listen(QHostAddress::Any, 8001))
    {
        qDebug() << "Server has started. Listening to port 8001.";
    }
    else
    {
        qDebug() << "Server failed to start. Error: " + chatServer->errorString();
    }

}

void Server::sendMessageToClients(QString message)
{
    if (allClients->size() > 0)
    {
        // we simply loop through the allClients array and pass the message data to all the connected clients.
        for (int i = 0; i < allClients->size(); i++)
        {
            if (allClients->at(i)->isOpen() && allClients->at(i)->isWritable())
            {
                allClients->at(i)->write(message.toUtf8());
            }
        }
    }

}

void Server::newClientConnection()
{
    // Every new client connected to the server is a QTcpSocket object,
    // which can be obtained from the QTcpServer object by calling nextPendingConnection().
    QTcpSocket* client = chatServer->nextPendingConnection();
    // You can obtain information about the client
    // such as its IP address and port number by calling peerAddress() and peerPort(), respectively.
    QString ipAddress = client->peerAddress().toString();
    int port = client->peerPort();
    // connect the client's disconnected(),readyRead() and stateChanged() signals to its respective slot function.
    // 1、When a client is disconnected from the server, the disconnected() signal will be triggered
    connect(client, &QTcpSocket::disconnected, this, &Server::socketDisconnected);
    // 2、whenever a client is sending in a message to the server, the readyRead() signal will be triggered.
    connect(client, &QTcpSocket::readyRead, this, &Server::socketReadyRead);
    // 3、 connected another signal called stateChanged() to the socketStateChanged() slot function.
    connect(client, &QTcpSocket::stateChanged, this, &Server::socketStateChanged);
    // store each new client into the allClients array for future use.
    allClients->push_back(client);
    qDebug() << "Socket connected from " + ipAddress + ":" + QString::number(port);
}

// When a client is disconnected from the server, the disconnected() signal will be triggered
void Server::socketDisconnected()
{
    // displaying the message on the server console whenever it happens, and nothing more.
    QTcpSocket* client = qobject_cast<QTcpSocket*>(QObject::sender());
    QString socketIpAddress = client->peerAddress().toString();
    int port = client->peerPort();
    qDebug() << "Socket disconnected from " + socketIpAddress + ":" + QString::number(port);
}

// whenever a client is sending in a message to the server, the readyRead() signal will be triggered.
void Server::socketReadyRead()
{
    // use QObject::sender() to get the pointer of the object that emitted the readyRead signal
    // and convert it to the QTcpSocket class so that we can access its readAll() function.
    QTcpSocket* client = qobject_cast<QTcpSocket*>(QObject::sender());
    QString socketIpAddress = client->peerAddress().toString();
    int port = client->peerPort();
    QString data = QString(client->readAll());
    qDebug() << "Message: " + data + " (" + socketIpAddress + ":" + QString::number(port) + ")";
    // redirect the message, just passing the message to all connected clients.
    sendMessageToClients(data);
}

// This function gets triggered whenever a client's network state has changed,
// such as connected, disconnected, listening, and so on.
void Server::socketStateChanged(QAbstractSocket::SocketState state)
{
    QTcpSocket* client = qobject_cast<QTcpSocket*>(QObject::sender());
    QString socketIpAddress = client->peerAddress().toString();
    int port = client->peerPort();
    QString desc;
    // simply print out a relevant message according to its new state
    if (state == QAbstractSocket::UnconnectedState)
        desc = "The socket is not connected.";
    else if (state == QAbstractSocket::HostLookupState)
        desc = "The socket is performing a host name lookup.";
    else if (state == QAbstractSocket::ConnectingState)
        desc = "The socket has started establishing a connection.";
    else if (state == QAbstractSocket::ConnectedState)
        desc = "A connection is established.";
    else if (state == QAbstractSocket::BoundState)
        desc = "The socket is bound to an address and port.";
    else if (state == QAbstractSocket::ClosingState)
        desc = "The socket is about to close (data may still be waiting to be written).";
    else if (state == QAbstractSocket::ListeningState)
        desc = "For internal use only.";
    qDebug() << "Socket state changed (" + socketIpAddress + ":" + QString::number(port) + "): " + desc;
}
