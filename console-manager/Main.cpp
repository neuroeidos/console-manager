#include"imgui.h"
#include"imgui_impl_glfw.h"
#include"imgui_impl_opengl3.h"

#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>

#include <string>
#include <map>
#include <functional>
#include <vector>
#include <sstream>
#include <memory>
#include <chrono>

// Класс для управления выводом логов
class Logger {
private:
    std::vector<std::string> logs;
    size_t maxLines;

public:
    Logger(size_t maxLines = 1000) : maxLines(maxLines) {}
    std::string statusMessage;
    std::chrono::steady_clock::time_point statusTime;

    // Добавить сообщение в лог
    void log(const std::string& message) {
        std::string timestampedMessage = getTimestamp() + u8" " + message;
        logs.push_back(timestampedMessage);
        if (logs.size() > maxLines) {
            logs.erase(logs.begin());
        }
    }

    void setStatusMessage(const std::string& message) {

        statusMessage =  message;
        statusTime = std::chrono::steady_clock::now();
    }

    // Получить все логи
    const std::vector<std::string>& getLogs() const {
        return logs;
    }

    // Очистить логи
    void clearLogs() {
        logs.clear();
    }

private:
    // Получить текущее время для лога
    std::string getTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf;
#ifdef _WIN32
        localtime_s(&tm_buf, &time);
#else
        localtime_r(&time, &tm_buf);
#endif
        char buffer[80];
        strftime(buffer, sizeof(buffer), "[%H:%M:%S]", &tm_buf);
        return std::string(buffer);
    }
};

// Класс для хранения аргументов команды
class CommandArgs {
private:
    std::vector<std::string> args;

public:
    CommandArgs(const std::vector<std::string>& args) : args(args) {}

    // Получить аргумент по индексу
    std::string getArg(size_t index) const {
        if (index < args.size()) {
            return args[index];
        }
        return "";
    }

    // Получить все аргументы
    const std::vector<std::string>& getArgs() const {
        return args;
    }

    // Получить количество аргументов
    size_t count() const {
        return args.size();
    }
};

// Класс для обработки команд
class CommandProcessor {
private:
    using CommandFunction = std::function<void(const CommandArgs&)>;
    std::map<std::string, CommandFunction> commands;
    std::map<std::string, std::string> commandDescriptions;
    std::shared_ptr<Logger> logger;

public:
    CommandProcessor(std::shared_ptr<Logger> logger) : logger(logger) {}

    // Регистрация новой команды
    void registerCommand(const std::string& name, CommandFunction callback, const std::string& description = "") {
        commands[name] = callback;
        commandDescriptions[name] = description;
    }

    // Выполнение команды
    bool executeCommand(const std::string& commandLine) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(commandLine);

        // Разделяем командную строку на токены
        while (std::getline(tokenStream, token, ' ')) {
            if (!token.empty()) {
                tokens.push_back(token);
            }
        }

        if (tokens.empty()) {
            return false;
        }

        std::string commandName = tokens[0];
        tokens.erase(tokens.begin());
        CommandArgs args(tokens);

        // Проверка существования команды
        auto it = commands.find(commandName);
        if (it != commands.end()) {
            logger->setStatusMessage(u8"Выполнение команды: " + commandName);

            it->second(args);
            return true;
        }
        else {
            logger->log(u8"Ошибка: Команда '" + commandName + u8"' не найдена");
            return false;
        }
    }

    // Получить список доступных команд с описаниями
    const std::map<std::string, std::string>& getCommandHelp() const {
        return commandDescriptions;
    }
};

// Класс пользовательского интерфейса с ImGui
class ImGuiUI {
private:
    std::shared_ptr<Logger> logger;
    std::shared_ptr<CommandProcessor> processor;
    bool running;

    GLFWwindow* window;
    char commandBuffer[256];
    bool autoScroll;
    //bool showCommandsList;
    //std::string statusMessage;
    //std::chrono::steady_clock::time_point statusTime;
    int historyPos;
    std::vector<std::string> commandHistory;

public:
    ImGuiUI() :
        running(false),
        window(nullptr),
        autoScroll(true),
        //showCommandsList(false),
        historyPos(-1) {

        logger = std::make_shared<Logger>();
        processor = std::make_shared<CommandProcessor>(logger);
        commandBuffer[0] = '\0';
        setupCommands();
    }

    ~ImGuiUI() {
        shutdown();
    }

    // Инициализация ImGui и GLFW
    bool initialize() {
        // Инициализация GLFW
        if (!glfwInit())
            return false;

        // Создаем окно GLFW
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(900, 600, u8"Console App Manager", NULL, NULL);
        if (window == NULL)
            return false;

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Включаем V-Sync

        gladLoadGL();

        // Настройка ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Bahnschrift.ttf", 16, NULL, io.Fonts->GetGlyphRangesCyrillic());
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Включаем навигацию с клавиатуры
        // Настройка стиля
        ImGui::StyleColorsDark();
        // Инициализация бэкендов
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");

        logger->log(u8"Приложение запущено. Введите 'help' для получения списка команд.");

        return true;
    }

    // Настройка базовых команд
    void setupCommands() {
        // Команда помощи
        processor->registerCommand("help", [this](const CommandArgs& args) {
            //this->showCommandsList = true;
            const auto& commands = processor->getCommandHelp();
            for (const auto& cmd : commands) {
                logger->log(u8""+ cmd.first + u8" -> " + cmd.second);
                
            }

            logger->setStatusMessage(u8"Отображен список доступных команд");
            }, u8"Показать список доступных команд");

            
        // Команда выхода
        processor->registerCommand("exit", [this](const CommandArgs& args) {
            this->running = false;
            logger->log(u8"Завершение работы программы");
            }, u8"Выйти из программы");

        // Команда очистки логов
        processor->registerCommand("clear", [this](const CommandArgs& args) {
            logger->clearLogs();
            logger->log(u8"Логи очищены");
            }, u8"Очистить журнал логов");

        // Пример команды с аргументами
        processor->registerCommand("echo", [this](const CommandArgs& args) {
            std::string message;
            for (size_t i = 0; i < args.count(); ++i) {
                message += args.getArg(i) + " ";
            }
            logger->log(u8"ECHO: " + message);
            logger->setStatusMessage(u8"ECHO: " + message);
            }, u8"Вывести переданный текст");

        // Добавление
        processor->registerCommand("add", [this](const CommandArgs& args) {
            if (args.count() < 2) {
                logger->log(u8"Ошибка: Команда 'add' требует двух числовых аргументов");
                logger->setStatusMessage(u8"Ошибка: Недостаточно аргументов");
                return;
            }

            try {
                int a = std::stoi(args.getArg(0));
                int b = std::stoi(args.getArg(1));
                int result = a + b;

                std::string message = u8"Результат сложения " + std::to_string(a) +
                    u8" и " + std::to_string(b) + " = " + std::to_string(result);
                logger->log(u8""+ message);
                logger->setStatusMessage(u8"" + message);
            }
            catch (std::exception& e) {
                std::string errorMsg = u8"Ошибка при выполнении команды 'add': " + std::string(e.what());
                logger->log(errorMsg);
                logger->setStatusMessage(errorMsg);
            }
            }, u8"Сложить два числа: add <число1> <число2>");
    }

    // Установка статусного сообщения
    //void setStatusMessage(const std::string& message) {
    //    statusMessage = u8"" + message;
    //    statusTime = std::chrono::steady_clock::now();
    //}

    // Отрисовка интерфейса
    void render() {
        // Очистка экрана
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Начало нового фрейма ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Создаем главное окно
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y));
        ImGui::Begin("##MainWindow", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse);

        // Область для отображения логов (большая верхняя часть)
        float commandHeight = 80.0f;
        float statusHeight = 28.0f;
        ImVec2 logSize = ImVec2(ImGui::GetWindowContentRegionWidth(),
            ImGui::GetWindowHeight() - commandHeight - statusHeight - 25);

        ImGui::BeginChild("LogArea", logSize, true, ImGuiWindowFlags_HorizontalScrollbar);


        // Отрисовка всех логов
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Уменьшаем расстояние между строками
        for (const auto& log : logger->getLogs()) {
            ImGui::TextUnformatted(log.c_str());
        }
        ImGui::PopStyleVar();
        
        // Авто-прокрутка
        if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndChild();

        // Статусная строка
        ImGui::BeginChild("StatusArea", ImVec2(ImGui::GetWindowContentRegionWidth(), statusHeight), true);

        // Проверка истечения времени статусного сообщения (5 секунд)
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - logger->statusTime).count();

        if (!logger->statusMessage.empty() && elapsed < 5) {
            ImGui::TextUnformatted(logger->statusMessage.c_str());
        }

        ImGui::EndChild();

        // Область ввода команды (внизу)
        ImGui::BeginChild("CommandArea", ImVec2(ImGui::GetWindowContentRegionWidth(), commandHeight), true);
        ImGui::Text(u8"Введите команду:");

        // Фокус на поле ввода
        ImGui::SetKeyboardFocusHere();

        // Поле ввода команды с обработкой нажатия Enter
        if (ImGui::InputText("##CommandInput", commandBuffer, IM_ARRAYSIZE(commandBuffer),
            ImGuiInputTextFlags_EnterReturnsTrue |
            ImGuiInputTextFlags_CallbackHistory,
            [](ImGuiInputTextCallbackData* data) -> int {
                ImGuiUI* ui = static_cast<ImGuiUI*>(data->UserData);
                return ui->inputTextCallback(data);
            }, this)) {

            std::string command = commandBuffer;
            if (!command.empty()) {
                // Сохраняем команду в истории
                commandHistory.push_back(command);
                if (commandHistory.size() > 50) {  // Ограничиваем размер истории
                    commandHistory.erase(commandHistory.begin());
                }
                historyPos = -1;  // Сбрасываем позицию в истории

                // Выполняем команду
                processor->executeCommand(command);
                commandBuffer[0] = '\0';  // Очищаем поле ввода
            }
        }

        // Чекбокс для авто-прокрутки
        ImGui::SameLine(ImGui::GetWindowWidth() - 120);
        ImGui::Checkbox(u8"Авто-скролл", &autoScroll);

        ImGui::EndChild();

        // Окно со списком команд (если открыто)
        /*if (showCommandsList) {
            
            ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
            if (ImGui::Begin(u8"Список доступных команд", &showCommandsList)) {
                const auto& commands = processor->getCommandHelp();
                if (ImGui::BeginTable(u8"CommandsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                    ImGui::TableSetupColumn(u8"Команда", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                    ImGui::TableSetupColumn(u8"Описание", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableHeadersRow();

                    for (const auto& cmd : commands) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(cmd.first.c_str());
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(cmd.second.c_str());
                    }


                    ImGui::EndTable();
                }
            }
            ImGui::End();
        }*/

        ImGui::End(); // Конец главного окна

        // Рендеринг ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    // Коллбэк для обработки истории ввода
    int inputTextCallback(ImGuiInputTextCallbackData* data) {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
            // Обработка стрелок вверх/вниз для навигации по истории
            if (data->EventKey == ImGuiKey_UpArrow) {
                if (historyPos == -1) {
                    historyPos = commandHistory.size() - 1;
                }
                else if (historyPos > 0) {
                    historyPos--;
                }
            }
            else if (data->EventKey == ImGuiKey_DownArrow) {
                if (historyPos != -1) {
                    if (historyPos < commandHistory.size() - 1) {
                        historyPos++;
                    }
                    else {
                        historyPos = -1;
                    }
                }
            }

            // Установка текста из истории
            if (historyPos != -1) {
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, commandHistory[historyPos].c_str());
            }
            else if (data->EventKey == ImGuiKey_DownArrow) {
                data->DeleteChars(0, data->BufTextLen);
            }
        }
        return 0;
    }

    // Запуск приложения
    void run() {
        if (!initialize()) {
            std::cerr << "Ошибка инициализации ImGui/GLFW!" << std::endl;
            return;
        }

        running = true;

        while (running && !glfwWindowShouldClose(window)) {
            glfwPollEvents();
            render();
            glfwSwapBuffers(window);
        }
    }

    // Освобождение ресурсов
    void shutdown() {
        if (window) {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();

            glfwDestroyWindow(window);
            glfwTerminate();
            window = nullptr;
        }
    }

    // Получить указатель на обработчик команд
    std::shared_ptr<CommandProcessor> getCommandProcessor() const {
        return processor;
    }

    // Получить указатель на логгер
    std::shared_ptr<Logger> getLogger() const {
        return logger;
    }
};

int main()
{
    ImGuiUI ui;
    ui.run();
    return 0;
}