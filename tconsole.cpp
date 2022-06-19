#include <iostream>
#include <QTextStream>
#include "tconsole.h"

using namespace LevelGauge;

TConsole::TConsole(QObject *parent /* = nullptr*/)
    : QThread(parent)
{
    _cmdStream = new QTextStream(stdin);  //связываем текстовый поток со стандартным потоком ввода
}

TConsole::~TConsole()
{
    Q_ASSERT(_cmdStream);

    delete _cmdStream;
}

void TConsole::stop()
{
    _isStoped = true;
    //_cmdStream->"1"    //std::cin << " ";
}

void TConsole::run()
{
    QString buf;
    while (!_isStoped) {
        //ждем ввода команды
        QString tmp;
        tmp = _cmdStream->read(1);
        buf += tmp;
        if (tmp == "\n") {
            emit getCommand(buf);
            buf.clear();
        }
    }
}



