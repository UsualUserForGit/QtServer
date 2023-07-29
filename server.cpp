#include "server.h"


Server::Server(QObject *parent) : QObject(parent)
{
    chatsDB = QSqlDatabase::addDatabase("QMYSQL");
    chatsDB.setUserName("Sheeesh");
    chatsDB.setPassword("IAMAFAILURETOMYFAMILY./1311");
    chatsDB.setPort(3306);
    if (chatsDB.open())
    {
        qDebug() << "successfully opened chats!";
    }
    else
    {
        qDebug() << "ERROR! cannot opened chats " << chatsDB.lastError().text();
    }

    sqlQuery = QSqlQuery(chatsDB);

    proceedQuery("CREATE DATABASE IF NOT EXISTS whispertalk_messaging_system_db;");
    proceedQuery("use whispertalk_messaging_system_db;");

    proceedQuery("CREATE TABLE IF NOT EXISTS user_accounts"
                "(id INT UNSIGNED NOT NULL AUTO_INCREMENT,"
                "login VARCHAR(50) NOT NULL,"
                "password varchar(100) NOT NULL,"
                "salt varchar(50) NOT NULL,"
                "PRIMARY KEY (id))"
                "ENGINE = InnoDB;");

    proceedQuery("CREATE TABLE IF NOT EXISTS user_chat_lines"
                "(id INT UNSIGNED NOT NULL AUTO_INCREMENT,"
                "chatline_name VARCHAR(20) NOT NULL,"
                "updated_at TIMESTAMP NOT NULL,"
                "last_message VARCHAR(50),"
                "user_id INT UNSIGNED NOT NULL,"
                "PRIMARY KEY (id),"
                "CONSTRAINT null_user FOREIGN KEY (user_id) REFERENCES user_accounts(id)"
                "ON DELETE CASCADE ON UPDATE CASCADE)"
                "ENGINE=InnoDB;");

    proceedQuery("CREATE TABLE IF NOT EXISTS chat_line_messages"
                "(id INT UNSIGNED NOT NULL AUTO_INCREMENT,"
                "client_id INT UNSIGNED NOT NULL,"
                "created_at TIMESTAMP NOT NULL,"
                "message TEXT NOT NULL,"
                "chatline_id INT UNSIGNED NOT NULL,"
                "contact_id INT UNSIGNED NOT NULL,"
                "FOREIGN KEY (client_id) REFERENCES user_accounts(id) "
                "ON DELETE CASCADE ON UPDATE CASCADE,"
                "FOREIGN KEY (chatline_id) REFERENCES user_accounts(id) "
                "ON DELETE CASCADE ON UPDATE CASCADE,"
                "FOREIGN KEY (contact_id) REFERENCES user_chat_lines(user_id) "
                "ON DELETE CASCADE ON UPDATE CASCADE)"
                "ENGINE InnoDB;");

    sqlQuery.clear();
}

// Function to execute queries to MYSQL database
void Server::proceedQuery(QString query, bool clearAfter)
{
    if(sqlQuery.exec(query)){
        qDebug() << "query " << query << " - successfully processed\n";
    }
    else{
        qDebug() << "query " << query << " - hasn't processed " << sqlQuery.lastError().text() << "\n";
    }


    if (clearAfter) sqlQuery.clear();
}

// This function creates and return QJsonObject with the data depending on the JsonFileType enum
QJsonObject Server::createJsonObject(JsonFileType jsonFile, bool operationReslt)
{
    QJsonObject mainJsonObject;

    switch (jsonFile) {
    case JsonFileType::SignInResults:

        mainJsonObject["login_reslt"] = operationReslt;
        break;
    case JsonFileType::RegisterUserResults:

        mainJsonObject["register_result"] = operationReslt;
        break;
    default:
        break;
    }

    // Convert the JSON object to a JSON document
    QJsonDocument jsonDocument(mainJsonObject);

    // Convert the JSON document to a QByteArray
    QByteArray jsonData = jsonDocument.toJson();

    // Create and open a file for writing
    QFile file("data.json");
    if (file.open(QIODevice::WriteOnly))
    {
        // Write the JSON data to the file
        file.write(jsonData);
        file.close();
        qDebug() << "JSON file created successfully.";
    } else
    {
        qDebug() << "Failed to create JSON file.";
    }

    return mainJsonObject;
}

// This function sends Json file to the certain client tcp socket
void Server::sendJsonToClient(const QJsonObject &jsonObject, QTcpSocket *clientSocket)
{
    QJsonDocument jsonDocument(jsonObject);
    QByteArray jsonData = jsonDocument.toJson();

    // Send the JSON data to the server
    clientSocket->write(jsonData);
}

// This function proceeds JSON data depending on the key of the file
void Server::processServerResponse(QJsonObject jsonObject, QTcpSocket *client)
{
    const QString key = jsonObject.begin().key();

    QJsonObject jsonDataObject = jsonObject.begin().value().toObject();
    // Display the key and values
    qDebug() << "Key:" << key;

    if (key == "message")
    {
        QString sender = jsonDataObject.value("sender").toString();
        QString message = jsonDataObject.value("message_text").toString();
        QString created_at = jsonDataObject.value("created_at").toString();
        QString chatline = jsonDataObject.value("chatline_name").toString();

        if (chatline == "General chat")
        {
            sendMessageToClients(jsonObject);
        }

        qDebug() << "sender_id:" << sender;
        qDebug() << "messsage:" << message;
        qDebug() << "created_at:" << created_at;
        qDebug() << "chatline_id:" << chatline;
        qDebug() << "---------------------";
    }
    else if (key == "login")
    {
        QString login = jsonDataObject.value("login").toString();
        QString password = jsonDataObject.value("password").toString();
        qDebug() << "login:" << login;
        qDebug() << "password:" << password;
        QString query = "select * from user_accounts where login = \'" + login + "\' and password = \'" + password + "\';";
        proceedQuery(query, false);
        bool login_check_success;
        if (sqlQuery.next())
        {
            qDebug() << "Successful authentication";
            login_check_success = true;
        }
        else
        {
            qDebug() << "Invalid login or password";
            login_check_success = false;
        }
        QJsonObject login_result = createJsonObject(JsonFileType::SignInResults, login_check_success);
        sendJsonToClient(login_result, client);
    }
    else if (key == "register")
    {
        //Implement this
        qDebug() << "unkown key - " << key;
    }
    else
    {

    }
}

// This slot starts the server
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

// This function sends JSON data to all of the clients that are connected to the server
void Server::sendMessageToClients(QJsonObject jsonObject)
{
    QJsonDocument jsonDocument(jsonObject);
    QByteArray jsonData = jsonDocument.toJson();
    if (allClients->size() > 0)
    {
        // we simply loop through the allClients array and pass the message data to all the connected clients.
        for (int i = 0; i < allClients->size(); i++)
        {
            if (allClients->at(i)->isOpen() && allClients->at(i)->isWritable())
            {
                allClients->at(i)->write(jsonData);
            }
        }
    }

}

// This slot creates new client connection if the server gets a new connection from a client
// further explanation you can see bellow
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
    // 2、whenever a client is sending in a message to the server, the readyRead() signal will be triggered.
    // 3、 connected another signal called stateChanged() to the socketStateChanged() slot function.
    // store each new client into the allClients array for future use.

    connect(client, &QTcpSocket::disconnected, this, &Server::socketDisconnected);
    connect(client, &QTcpSocket::readyRead, this, &Server::socketReadyRead);
    connect(client, &QTcpSocket::stateChanged, this, &Server::socketStateChanged);

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
    QByteArray jsonData = client->readAll();

    // Parse the JSON data
    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData);

    // Check if parsing was successful
    if (!jsonDocument.isNull() && jsonDocument.isObject())
    {
        // Process the JSON object here
        // Example: QString name = jsonObject["name"].toString();
        // Output it to console or do whatever you need
        processServerResponse(jsonDocument.object(), client);


    }
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
