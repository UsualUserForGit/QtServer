#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QDebug>
#include <QVector>
#include <QTcpServer>
#include <QTcpSocket>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QFile>


class Server : public QObject
{
    Q_OBJECT

    QTcpServer*             chatServer;
    QVector<QTcpSocket*>*   allClients;
    QSqlDatabase chatsDB;
    QSqlDatabase usersDB;
    QSqlQuery sqlQuery;

    void proceedQuery(QString query = "", bool clearAfter = true);

    enum class JsonFileType {SignInData, RegisterUser, SignInResults, RegisterUserResults};

    QJsonObject createJsonObject(JsonFileType jsonFile, bool operationReslt);
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
