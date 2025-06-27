#include "mytcpserver.h"
#include <QDateTime>

MyTcpServer::MyTcpServer(QObject *parent) : QObject(parent),
    currentPlayerIndex(0), gameStarted(false), gameEnded(false), attempts(0)
{
    mTcpServer = new QTcpServer(this);

    connect(mTcpServer, &QTcpServer::newConnection,
            this, &MyTcpServer::slotNewConnection);

    if(!mTcpServer->listen(QHostAddress::Any, 9000)){
        qDebug() << "Сервер НЕ запущен! Ошибка:" << mTcpServer->errorString();
    } else {
        qDebug() << "=== СЕРВЕР ИГРЫ 'УГАДАЙ ЧИСЛО' ЗАПУЩЕН ===";
        qDebug() << "Порт: ";
        qDebug() << "Максимум игроков:" << MAX_CLIENTS;
        qDebug() << "Ожидание подключений...";
        initializeGame();
    }
}

MyTcpServer::~MyTcpServer()
{
    // Отключаем всех клиентов
    for(auto client : mClients) {
        if(client->state() == QTcpSocket::ConnectedState) {
            client->write("Сервер завершает работу. До свидания!\r\n");
            client->flush();
            client->close();
        }
    }

    mTcpServer->close();
    qDebug() << "Сервер остановлен.";
}

void MyTcpServer::initializeGame()
{
    // Генерируем случайное число от 1 до 100
    secretNumber = QRandomGenerator::global()->bounded(1, 101);
    currentPlayerIndex = 0;
    gameStarted = false;
    gameEnded = false;
    attempts = 0;

    qDebug() << QString("Новая игра! Загадано число: %1").arg(secretNumber);
}

void MyTcpServer::slotNewConnection()
{
    QTcpSocket* newClient = mTcpServer->nextPendingConnection();

    qDebug() << QString("Попытка подключения от %1:%2")
                    .arg(newClient->peerAddress().toString())
                    .arg(newClient->peerPort());

    // Проверяем лимит подключений
    if(mClients.size() >= MAX_CLIENTS) {
        qDebug() << "Превышен лимит клиентов! Отклоняем подключение.";

        newClient->write("СЕРВЕР ЗАНЯТ!\r\n");
        newClient->write("Максимальное количество игроков (5) уже подключено.\r\n");
        newClient->write("Пожалуйста, попробуйте подключиться позже.\r\n");
        newClient->write("Соединение будет разорвано через 3 секунды...\r\n");
        newClient->flush();

        // Задержка перед разрывом соединения
        QTimer::singleShot(3000, [newClient]() {
            newClient->close();
            newClient->deleteLater();
        });

        return;
    }

    // Добавляем клиента в список
    mClients.append(newClient);
    int playerNumber = mClients.size();

    // Приветствие новому игроку
    QString welcomeMsg = QString("=== ДОБРО ПОЖАЛОВАТЬ В ИГРУ 'УГАДАЙ ЧИСЛО' ===\r\n");
    welcomeMsg += QString("Вы игрок #%1\r\n").arg(playerNumber);
    welcomeMsg += QString("Правила:\r\n");
    welcomeMsg += QString("- Я загадал число от 1 до 100\r\n");
    welcomeMsg += QString("- Игроки ходят по очереди\r\n");
    welcomeMsg += QString("- Отправьте число для попытки угадать\r\n");
    welcomeMsg += QString("- Я скажу 'больше' или 'меньше'\r\n");
    welcomeMsg += QString("- Кто угадает - тот победил!\r\n");
    welcomeMsg += QString("==========================================\r\n");

    newClient->write(welcomeMsg.toUtf8());

    // Подключаем сигналы
    connect(newClient, &QTcpSocket::readyRead, this, &MyTcpServer::slotServerRead);
    connect(newClient, &QTcpSocket::disconnected, this, &MyTcpServer::slotClientDisconnected);

    qDebug() << QString("Игрок #%1 подключился! Всего игроков: %2")
                    .arg(playerNumber).arg(mClients.size());

    // Уведомляем всех о новом игроке
    QString notification = QString("Игрок #%1 присоединился к игре! Всего игроков: %2/%3\r\n")
                               .arg(playerNumber).arg(mClients.size()).arg(MAX_CLIENTS);
    sendToAllClients(notification);

    // Если достаточно игроков и игра не началась - стартуем
    if(mClients.size() >= 2 && !gameStarted) {
        startGame();
    } else if(!gameStarted) {
        QString waitMsg = QString("Ожидаем еще игроков... (минимум 2, сейчас: %1)\r\n")
                              .arg(mClients.size());
        sendToAllClients(waitMsg);
    }
}

void MyTcpServer::startGame()
{
    if(gameStarted || mClients.isEmpty()) return;

    gameStarted = true;
    qDebug() << "=== ИГРА НАЧАЛАСЬ ===";

    QString startMsg = "\r\nИГРА НАЧАЛАСЬ!\r\n";
    startMsg += QString("Загадано число от 1 до 100\r\n");
    startMsg += QString("Всего игроков: %1\r\n").arg(mClients.size());
    startMsg += QString("Первый ход: Игрок #1\r\n");
    startMsg += QString("Отправьте ваше число!\r\n\r\n");

    sendToAllClients(startMsg);
    sendGameStatus();
}

void MyTcpServer::slotServerRead()
{
    QTcpSocket* sender = qobject_cast<QTcpSocket*>(QObject::sender());
    if(!sender || gameEnded) return;

    QByteArray data = sender->readAll();
    QString message = QString::fromUtf8(data).trimmed();

    if(message.isEmpty()) return;

    int playerIndex = getPlayerIndex(sender);
    qDebug() << QString("Игрок #%1 отправил: '%2'").arg(playerIndex + 1).arg(message);

    if(!gameStarted) {
        sender->write("Игра еще не началась! Ожидайте...\r\n");
        return;
    }

    // Проверяем, чей сейчас ход
    if(playerIndex != currentPlayerIndex) {
        QString turnMsg = QString("Не ваш ход! Сейчас ходит игрок #%1\r\n")
                              .arg(currentPlayerIndex + 1);
        sender->write(turnMsg.toUtf8());
        return;
    }

    // Пытаемся преобразовать в число
    bool ok;
    int guess = message.toInt(&ok);

    if(!ok || guess < 1 || guess > 100) {
        sender->write("Ошибка! Отправьте число от 1 до 100\r\n");
        return;
    }

    processGuess(sender, guess);
}

void MyTcpServer::processGuess(QTcpSocket* player, int guess)
{
    attempts++;
    int playerNum = getPlayerIndex(player) + 1;

    QString resultMsg = QString("Игрок #%1 предполагает: %2\r\n").arg(playerNum).arg(guess);

    qDebug() << QString("Попытка #%1: Игрок #%2 угадывает %3 (загадано: %4)")
                    .arg(attempts).arg(playerNum).arg(guess).arg(secretNumber);

    if(guess == secretNumber) {
        // ПОБЕДА!
        resultMsg += QString("ПРАВИЛЬНО! Игрок #%1 ПОБЕДИЛ! \r\n").arg(playerNum);
        resultMsg += QString("Загаданное число было: %1\r\n").arg(secretNumber);
        resultMsg += QString("Количество попыток: %1\r\n").arg(attempts);
        resultMsg += QString("Игра окончена!\r\n");

        sendToAllClients(resultMsg);
        endGame(player);

    } else if(guess < secretNumber) {
        resultMsg += QString("Загаданное число БОЛЬШЕ чем %1\r\n").arg(guess);
        sendToAllClients(resultMsg);
        moveToNextPlayer();

    } else {
        resultMsg += QString("Загаданное число МЕНЬШЕ чем %1\r\n").arg(guess);
        sendToAllClients(resultMsg);
        moveToNextPlayer();
    }
}

void MyTcpServer::moveToNextPlayer()
{
    if(mClients.isEmpty()) return;

    currentPlayerIndex = (currentPlayerIndex + 1) % mClients.size();
    sendGameStatus();
}

void MyTcpServer::sendGameStatus()
{
    if(gameEnded || mClients.isEmpty()) return;

    QString statusMsg = QString("\r\n--- ХОД ИГРОКА #%1 ---\r\n")
                            .arg(currentPlayerIndex + 1);
    statusMsg += QString("Попытка #%1\r\n").arg(attempts + 1);
    statusMsg += QString("Отправьте число от 1 до 100:\r\n");

    sendToAllClients(statusMsg);
}

void MyTcpServer::sendToAllClients(const QString& message)
{
    QByteArray data = message.toUtf8();

    for(auto it = mClients.begin(); it != mClients.end();) {
        QTcpSocket* client = *it;

        if(client->state() == QTcpSocket::ConnectedState) {
            client->write(data);
            client->flush();
            ++it;
        } else {
            // Удаляем отключенного клиента
            qDebug() << "Удаляем отключенного клиента";
            it = mClients.erase(it);
            client->deleteLater();
        }
    }
}

void MyTcpServer::endGame(QTcpSocket* winner)
{
    gameEnded = true;

    qDebug() << "=== ИГРА ЗАВЕРШЕНА ===";
    if(winner) {
        qDebug() << QString("Победитель: Игрок #%1").arg(getPlayerIndex(winner) + 1);
    }
    qDebug() << QString("Попыток сделано: %1").arg(attempts);

    // Задержка перед отключением
    QTimer::singleShot(5000, [this]() {
        QString finalMsg = "\r\nСервер завершает соединения через 3 секунды...\r\n";
        finalMsg += "Спасибо за игру! До новых встреч!\r\n";
        sendToAllClients(finalMsg);

        QTimer::singleShot(3000, [this]() {
            for(auto client : mClients) {
                if(client->state() == QTcpSocket::ConnectedState) {
                    client->close();
                }
            }

            // Перезапускаем игру для новых клиентов
            mClients.clear();
            initializeGame();
            qDebug() << "\nИгра перезапущена. Ожидание новых игроков...";
        });
    });
}

void MyTcpServer::slotClientDisconnected()
{
    QTcpSocket* client = qobject_cast<QTcpSocket*>(QObject::sender());
    if(!client) return;

    int playerIndex = getPlayerIndex(client);

    qDebug() << QString("Игрок #%1 отключился").arg(playerIndex + 1);

    // Удаляем из списка
    mClients.removeOne(client);
    client->deleteLater();

    // Если игрок отключился во время своего хода
    if(gameStarted && !gameEnded && playerIndex == currentPlayerIndex) {
        if(!mClients.isEmpty()) {
            currentPlayerIndex = currentPlayerIndex % mClients.size();
            sendGameStatus();
        }
    }

    // Уведомляем остальных
    if(!mClients.isEmpty() && !gameEnded) {
        QString disconnectMsg = QString("Игрок #%1 покинул игру. Осталось игроков: %2\r\n")
                                    .arg(playerIndex + 1).arg(mClients.size());
        sendToAllClients(disconnectMsg);

        // Если игроков стало слишком мало
        if(mClients.size() < 2 && gameStarted) {
            sendToAllClients("Недостаточно игроков для продолжения. Игра остановлена.\r\n");
            gameStarted = false;
            gameEnded = true;
        }
    }

    // Если все отключились - перезапускаем
    if(mClients.isEmpty()) {
        qDebug() << "Все игроки отключились. Перезапуск игры...";
        initializeGame();
    }
}

QString MyTcpServer::getPlayerName(QTcpSocket* socket)
{
    int index = getPlayerIndex(socket);
    return QString("Игрок #%1").arg(index + 1);
}

int MyTcpServer::getPlayerIndex(QTcpSocket* socket)
{
    return mClients.indexOf(socket);
}
