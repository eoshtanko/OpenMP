#include <iostream>
#include <thread>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <map>
#include <vector>
#include "/usr/local/opt/libomp/include/omp.h"

// Штанько Екатерина Олеговна БПИ193 Вариант 27

//27. Пляшущие человечки. На тайном собрании глав преступного мира города Лондона
//председатель собрания профессор Мориарти постановил: отныне вся переписка
//между преступниками должна вестись тайнописью. В качестве стандарта были
//выбраны «пляшущие человечки», шифр, в котором каждой букве латинского
//алфавита соответствует хитроумный значок. Реализовать многопоточное
//приложение, шифрующее исходный текст (в качестве ключа используется кодовая
//таблица, устанавливающая однозначное соответствие между каждой буквой и каким-
//нибудь числом). Каждый поток шифрует свои кусочки текста. При решении
//использовать парадигму портфеля задач.

// Аргументы для запуска приложения из командной строки:
// 1. argv[1] - путь до файла для чтения входных данных. Если файла по данному пути не обнаружено -
// сообщение об ошибке.
// 2. argv[2] - путь до файла для записи выходных данных. Если файла по данному пути не обнаружено -
// создание нового файла.
// 3. argv[3] - число потоков. Требование: значение > 0  и < 101!

/// Словарь, хранящий установленное соответствие между буквами латинского алфавита и числами
static std::map<char, int> letterSymbol;

/// Полный путь до файла для чтения входных данных(получаем из аргументов командной стр. argv[1])
static std::string inputPath;

/// Полный путь до файла для записи выходных данных(получаем из аргументов командной стр. argv[2])
static std::string outputPath;

/// Колличество потоков(получаем из аргументов командной стр. argv[3])
static int numberOfThreads;

/// Входные данные - строка для кодирования
static std::string inputString;

/// Выходные данные - закодированная строка
static std::string outputString;

/// Устанавливает соответствие между буквами латинского алфавита и числами.
/// Установленное соответствие хранится в словаре letterSymbol.
static void letterSymbolFull() {
    srand(0);
    int num = 0;
    std::vector<int> numbers;
    bool exist;
    for (int i = 'a'; i < 'z' + 1; ++i) {
        do {
            num = rand() % 38 + 129;
            exist = std::find(numbers.begin(), numbers.end(), num) != numbers.end();
        } while (num == (int) ' ' || exist || num == 152 || num == 150);
        numbers.push_back(num);
        letterSymbol[(char) i] = num;
    }
}

/// Заполняет выходную строку по следующему принципу:
/// Если введена буква латинского алфавита - устанавливает пробел
/// Иначе - оставляет тот символ, что находился по этому индексу во входной строке
static void fullOutputString() {
    for (int i = 0; i < inputString.size(); ++i) {
        // Если элемент поступивший строки латинский символ - устанавливаем пробел
        // На этот пробел далее будет помещен закодированный символ
        if (inputString[i] >= 'a' && inputString[i] <= 'z') {
            outputString += " ";
        }
            // Иначе оставляем тот символ, что был в поступившей строке
            // Это позволит главам преступного мира как минимум пользоваться числами и
            // переходом на новую строку :)
        else {
            outputString += inputString[i];
        }
    }
}

/// Считывает строки из файла.
static void readFromFile() {
    std::ifstream fileInput(inputPath);
    std::string line;
    if (fileInput.is_open()) {
        int numberOfInputString = 0;
        while (getline(fileInput, line)) {
            if (numberOfInputString > 0) {
                inputString += "\n" + line;
            } else {
                inputString += line;
            }
            numberOfInputString++;
        }
    } else {
        throw std::invalid_argument("Файл с входными данными найден не был!");
    }
    fileInput.close();
}

/// Переводит заглавные буквы в строчные
static void toLowerCase() {
    for (int i = 0; i < inputString.size(); ++i) {
        if (inputString[i] >= 'A' && inputString[i] <= 'Z') {
            inputString[i] = tolower(inputString[i]);
        }
    }
}

/// Вывод выходных данных в файл.
/// Если по указанному пути файл найден не был - происходит создание
/// нового файла и запись в него.
static void outputToFile(const std::string &output) {
    std::ofstream fileOutput;
    fileOutput.open(outputPath);
    if (fileOutput.is_open()) {
        fileOutput << output;
    }
    fileOutput.close();
}

/// Кодирует латинские символы в переданной строке и записывает результат в строку для вывода.
static void encoding(std::string str, int begin) {
    for (int i = 0; i < str.size(); ++i) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            outputString[begin + i] = (char) letterSymbol[str[i]];
        } else {
            outputString[begin + i] = str[i];
        }
    }
}

/// Портфель задач
static int begin = 0;

void *func() {
    // Под "словом" я подразумеваю последовательность с которой работает один поток

    // Индекс начала одного "слова" в считанной последовательности
    int beginTask = 0;
    // Вычисляем длину "слова" для одного потока
    int oneTaskStrLength = (int) ceil((double) (inputString.size()) / numberOfThreads);
    // Вычисляем кол-во "слов" в считанной последовательности
    int numberOfTasksInStr = (int) ceil((double) (inputString.size()) / oneTaskStrLength);

// Область паралельного исполнения.
// Кол-во потоков задано через num_threads(numberOfThreads - 1).
// Я создаю на один поток меньше, чем было указано пользователем в командной строке.
// Делая так, я учитываю наличие основного потока.
// Условие private указывает на то, что каждый поток должен иметь
// свою копию переменной(beginTask - его задача) на всем протяжении своего исполнения.
// Условие shared указывает на то, что переменная(begin - портфель)
// будут разделяться между потоками. Все потоки будут доступаться к одной и той же области памяти.
#pragma omp parallel num_threads(numberOfThreads - 1) private(beginTask) shared(begin)
    {
// Директива #pragma omp parallel for указывает на то, что данный цикл следует разделить по итерациям между потоками
#pragma omp parallel for
        for (int i = 0; i < numberOfTasksInStr; ++i) {

// Критическая секция, в которой происходит выделение задачи из портфеля задач
#pragma omp critical
            {
                beginTask = begin; // (Выделение задачи)
                // Перемещаем индекс начала на следующее слово
                begin += oneTaskStrLength;
            }

            if (beginTask < inputString.size()) {
                // Выбираем из введенной последовательности "слово" для одного потока и кодируем его
                encoding(inputString.substr(beginTask, oneTaskStrLength), beginTask);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    letterSymbolFull();
    try {
        if (argc != 4) {
            throw std::invalid_argument("Ошибка в данных переданных через командную строку!");
        }
        inputPath = argv[1];
        outputPath = argv[2];
        numberOfThreads = atoi(argv[3]);
        if (numberOfThreads < 1) {
            throw std::invalid_argument("Число потоков не может быть меньше 1!");
        }
        if (numberOfThreads > 100) {
            throw std::invalid_argument("Число потоков следует установить меньше <= 100!");
        }
        readFromFile();
        toLowerCase();
        fullOutputString();
        func();
        outputToFile(outputString);
    }
    catch (const std::exception &e) {
        std::cout << e.what();
    }
    return 0;
}