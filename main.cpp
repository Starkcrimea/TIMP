#include <QCoreApplication>
#include "mytcpserver.h"
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qDebug() << "===========================================";
    qDebug() << "    СЕРВЕР ИГРЫ 'УГАДАЙ ЧИСЛО' v1.0";
    qDebug() << "===========================================";
    qDebug() << "Особенности:";
    qDebug() << "Поддержка до 5 одновременных игроков";
    qDebug() << "Автоматическое отклонение лишних подключений";
    qDebug() << "Пошаговая игра с загаданным числом 1-100";
    qDebug() << "Автоматический перезапуск после завершения";
    qDebug() << "===========================================";

    MyTcpServer server;

    qDebug() << "\nДля остановки сервера нажмите Ctrl+C";
    qDebug() << "Клиенты могут подключаться через telnet:";
    qDebug() << "telnet localhost 9000";
    qDebug() << "\n===========================================\n";

    return app.exec();
}
