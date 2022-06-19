/* Создает отдельный поток который слушает поток стандартного ввода
 * Когда приходит команда генерирует событие GetCommand в качестве
 * параметра передает текст комадны без знака перевода строки
*/
#ifndef TCONSOLE_H
#define TCONSOLE_H

#include <QObject>
#include <QThread>
#include <QTextStream>

namespace LevelGauge {

class TConsole final: public QThread
{
    Q_OBJECT

public:
    static TConsole* Console(QObject* parent = nullptr)
    {
        static TConsole* _console = nullptr;

        Q_ASSERT((_console != nullptr) && (parent != nullptr));

        if (_console == nullptr){
            _console = new TConsole(parent);
        }

        return _console;
    };

private:
    explicit TConsole(QObject* parent = nullptr);

public:
    ~TConsole();
    void stop();

signals:
     void getCommand(const QString& cmd); //генерируется когда приходит новая команда

private:
    void run() override; //основной цикл ожидания команды

private:
     QTextStream* _cmdStream = nullptr;  //поток чтения команд
     bool _isStoped = false;
};

} //namespace LevelGaugeStatus

#endif // TCONSOLE_H
