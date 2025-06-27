#ifndef MYTCPSERVER_H
#define MYTCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtNetwork>
#include <QByteArray>
#include <QDebug>
#include <QList>
#include <QTimer>
#include <QRandomGenerator>

class MyTcpServer : public QObject
{
    Q_OBJECT

public:
    explicit MyTcpServer(QObject *parent = nullptr);
    ~MyTcpServer();

public slots:
    void slotNewConnection();
    void slotServerRead();
    void slotClientDisconnected();

private:
    QTcpServer* mTcpServer;
    QList<QTcpSocket*> mClients;     // Список подключенных клиентов

    // Игровые параметры
    static const int MAX_CLIENTS = 5;
    int secretNumber;                 // Загаданное число (1-100)
    int currentPlayerIndex;           // Индекс текущего игрока
    bool gameStarted;                // Флаг начала игры
    bool gameEnded;                  // Флаг окончания игры
    int attempts;                    // Счетчик попыток

    // Служебные методы
    void initializeGame();
    void startGame();
    void processGuess(QTcpSocket* player, int guess);
    void sendToAllClients(const QString& message);
    void sendGameStatus();
    void endGame(QTcpSocket* winner = nullptr);
    void moveToNextPlayer();
    QString getPlayerName(QTcpSocket* socket);
    int getPlayerIndex(QTcpSocket* socket);
};

#endif // MYTCPSERVER_H
