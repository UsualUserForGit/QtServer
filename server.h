#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QDebug>
#include <QVector>
#include <QHash>
#include <QFile>

#include <QTcpServer>
#include <QTcpSocket>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>


class Server : public QObject
{
    Q_OBJECT

    struct Message
    {
        int id;
        QString sender_login;
        QString created_at;
        QString message_text;
        QString chatline_name;

        bool isMessageEmpty()
        {
            return (id < 0 &&
                    sender_login.isEmpty() &&
                    created_at.isNull() &&
                    message_text.isEmpty() &&
                    chatline_name.isEmpty());
        }
    };

    QTcpServer*             chatServer;
    QVector<QTcpSocket*>*   allClients;
    QSqlDatabase chatsDB;
    QSqlDatabase usersDB;
    QSqlQuery sqlQuery;
    Message messageObject;

    void proceedQuery(QString query = "", bool clearAfter = true);

    enum class JsonFileType {SignInData, RegisterUser, SignInResults, RegisterUserResults, Message};

    QJsonObject createJsonObject(JsonFileType jsonFile, bool operationReslt = false);
    void sendJsonToClient(const QJsonObject& jsonObject, QTcpSocket *clientSocket);
    void processServerResponse(QJsonObject jsonObject, QTcpSocket *client);


public:
    explicit Server(QObject *parent = nullptr);

    void startServer();
    void sendMessageToClients(QJsonObject jsonObject);

signals:

public slots:
    void newClientConnection();
    void socketDisconnected();
    void socketReadyRead();
    void socketStateChanged(QAbstractSocket::SocketState state);

};

#endif // SERVER_H
