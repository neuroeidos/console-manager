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

// ����� ��� ���������� ������� �����
class Logger {
private:
    std::vector<std::string> logs;
    size_t maxLines;

public:
    Logger(size_t maxLines = 1000) : maxLines(maxLines) {}
    std::string statusMessage;
    std::chrono::steady_clock::time_point statusTime;

    // �������� ��������� � ���
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

    // �������� ��� ����
    const std::vector<std::string>& getLogs() const {
        return logs;
    }

    // �������� ����
    void clearLogs() {
        logs.clear();
    }

private:
    // �������� ������� ����� ��� ����
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

// ����� ��� �������� ���������� �������
class CommandArgs {
private:
    std::vector<std::string> args;

public:
    CommandArgs(const std::vector<std::string>& args) : args(args) {}

    // �������� �������� �� �������
    std::string getArg(size_t index) const {
        if (index < args.size()) {
            return args[index];
        }
        return "";
    }

    // �������� ��� ���������
    const std::vector<std::string>& getArgs() const {
        return args;
    }

    // �������� ���������� ����������
    size_t count() const {
        return args.size();
    }
};

// ����� ��� ��������� ������
class CommandProcessor {
private:
    using CommandFunction = std::function<void(const CommandArgs&)>;
    std::map<std::string, CommandFunction> commands;
    std::map<std::string, std::string> commandDescriptions;
    std::shared_ptr<Logger> logger;

public:
    CommandProcessor(std::shared_ptr<Logger> logger) : logger(logger) {}

    // ����������� ����� �������
    void registerCommand(const std::string& name, CommandFunction callback, const std::string& description = "") {
        commands[name] = callback;
        commandDescriptions[name] = description;
    }

    // ���������� �������
    bool executeCommand(const std::string& commandLine) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(commandLine);

        // ��������� ��������� ������ �� ������
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

        // �������� ������������� �������
        auto it = commands.find(commandName);
        if (it != commands.end()) {
            logger->setStatusMessage(u8"���������� �������: " + commandName);

            it->second(args);
            return true;
        }
        else {
            logger->log(u8"������: ������� '" + commandName + u8"' �� �������");
            return false;
        }
    }

    // �������� ������ ��������� ������ � ����������
    const std::map<std::string, std::string>& getCommandHelp() const {
        return commandDescriptions;
    }
};

// ����� ����������������� ���������� � ImGui
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

    // ������������� ImGui � GLFW
    bool initialize() {
        // ������������� GLFW
        if (!glfwInit())
            return false;

        // ������� ���� GLFW
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(900, 600, u8"Console App Manager", NULL, NULL);
        if (window == NULL)
            return false;

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // �������� V-Sync

        gladLoadGL();

        // ��������� ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Bahnschrift.ttf", 16, NULL, io.Fonts->GetGlyphRangesCyrillic());
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // �������� ��������� � ����������
        // ��������� �����
        ImGui::StyleColorsDark();
        // ������������� ��������
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");

        logger->log(u8"���������� ��������. ������� 'help' ��� ��������� ������ ������.");

        return true;
    }

    // ��������� ������� ������
    void setupCommands() {
        // ������� ������
        processor->registerCommand("help", [this](const CommandArgs& args) {
            //this->showCommandsList = true;
            const auto& commands = processor->getCommandHelp();
            for (const auto& cmd : commands) {
                logger->log(u8""+ cmd.first + u8" -> " + cmd.second);
                
            }

            logger->setStatusMessage(u8"��������� ������ ��������� ������");
            }, u8"�������� ������ ��������� ������");

            
        // ������� ������
        processor->registerCommand("exit", [this](const CommandArgs& args) {
            this->running = false;
            logger->log(u8"���������� ������ ���������");
            }, u8"����� �� ���������");

        // ������� ������� �����
        processor->registerCommand("clear", [this](const CommandArgs& args) {
            logger->clearLogs();
            logger->log(u8"���� �������");
            }, u8"�������� ������ �����");

        // ������ ������� � �����������
        processor->registerCommand("echo", [this](const CommandArgs& args) {
            std::string message;
            for (size_t i = 0; i < args.count(); ++i) {
                message += args.getArg(i) + " ";
            }
            logger->log(u8"ECHO: " + message);
            logger->setStatusMessage(u8"ECHO: " + message);
            }, u8"������� ���������� �����");

        // ����������
        processor->registerCommand("add", [this](const CommandArgs& args) {
            if (args.count() < 2) {
                logger->log(u8"������: ������� 'add' ������� ���� �������� ����������");
                logger->setStatusMessage(u8"������: ������������ ����������");
                return;
            }

            try {
                int a = std::stoi(args.getArg(0));
                int b = std::stoi(args.getArg(1));
                int result = a + b;

                std::string message = u8"��������� �������� " + std::to_string(a) +
                    u8" � " + std::to_string(b) + " = " + std::to_string(result);
                logger->log(u8""+ message);
                logger->setStatusMessage(u8"" + message);
            }
            catch (std::exception& e) {
                std::string errorMsg = u8"������ ��� ���������� ������� 'add': " + std::string(e.what());
                logger->log(errorMsg);
                logger->setStatusMessage(errorMsg);
            }
            }, u8"������� ��� �����: add <�����1> <�����2>");
    }

    // ��������� ���������� ���������
    //void setStatusMessage(const std::string& message) {
    //    statusMessage = u8"" + message;
    //    statusTime = std::chrono::steady_clock::now();
    //}

    // ��������� ����������
    void render() {
        // ������� ������
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // ������ ������ ������ ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ������� ������� ����
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y));
        ImGui::Begin("##MainWindow", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse);

        // ������� ��� ����������� ����� (������� ������� �����)
        float commandHeight = 80.0f;
        float statusHeight = 28.0f;
        ImVec2 logSize = ImVec2(ImGui::GetWindowContentRegionWidth(),
            ImGui::GetWindowHeight() - commandHeight - statusHeight - 25);

        ImGui::BeginChild("LogArea", logSize, true, ImGuiWindowFlags_HorizontalScrollbar);


        // ��������� ���� �����
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // ��������� ���������� ����� ��������
        for (const auto& log : logger->getLogs()) {
            ImGui::TextUnformatted(log.c_str());
        }
        ImGui::PopStyleVar();
        
        // ����-���������
        if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndChild();

        // ��������� ������
        ImGui::BeginChild("StatusArea", ImVec2(ImGui::GetWindowContentRegionWidth(), statusHeight), true);

        // �������� ��������� ������� ���������� ��������� (5 ������)
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - logger->statusTime).count();

        if (!logger->statusMessage.empty() && elapsed < 5) {
            ImGui::TextUnformatted(logger->statusMessage.c_str());
        }

        ImGui::EndChild();

        // ������� ����� ������� (�����)
        ImGui::BeginChild("CommandArea", ImVec2(ImGui::GetWindowContentRegionWidth(), commandHeight), true);
        ImGui::Text(u8"������� �������:");

        // ����� �� ���� �����
        ImGui::SetKeyboardFocusHere();

        // ���� ����� ������� � ���������� ������� Enter
        if (ImGui::InputText("##CommandInput", commandBuffer, IM_ARRAYSIZE(commandBuffer),
            ImGuiInputTextFlags_EnterReturnsTrue |
            ImGuiInputTextFlags_CallbackHistory,
            [](ImGuiInputTextCallbackData* data) -> int {
                ImGuiUI* ui = static_cast<ImGuiUI*>(data->UserData);
                return ui->inputTextCallback(data);
            }, this)) {

            std::string command = commandBuffer;
            if (!command.empty()) {
                // ��������� ������� � �������
                commandHistory.push_back(command);
                if (commandHistory.size() > 50) {  // ������������ ������ �������
                    commandHistory.erase(commandHistory.begin());
                }
                historyPos = -1;  // ���������� ������� � �������

                // ��������� �������
                processor->executeCommand(command);
                commandBuffer[0] = '\0';  // ������� ���� �����
            }
        }

        // ������� ��� ����-���������
        ImGui::SameLine(ImGui::GetWindowWidth() - 120);
        ImGui::Checkbox(u8"����-������", &autoScroll);

        ImGui::EndChild();

        // ���� �� ������� ������ (���� �������)
        /*if (showCommandsList) {
            
            ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
            if (ImGui::Begin(u8"������ ��������� ������", &showCommandsList)) {
                const auto& commands = processor->getCommandHelp();
                if (ImGui::BeginTable(u8"CommandsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                    ImGui::TableSetupColumn(u8"�������", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                    ImGui::TableSetupColumn(u8"��������", ImGuiTableColumnFlags_WidthStretch);
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

        ImGui::End(); // ����� �������� ����

        // ��������� ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    // ������� ��� ��������� ������� �����
    int inputTextCallback(ImGuiInputTextCallbackData* data) {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
            // ��������� ������� �����/���� ��� ��������� �� �������
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

            // ��������� ������ �� �������
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

    // ������ ����������
    void run() {
        if (!initialize()) {
            std::cerr << "������ ������������� ImGui/GLFW!" << std::endl;
            return;
        }

        running = true;

        while (running && !glfwWindowShouldClose(window)) {
            glfwPollEvents();
            render();
            glfwSwapBuffers(window);
        }
    }

    // ������������ ��������
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

    // �������� ��������� �� ���������� ������
    std::shared_ptr<CommandProcessor> getCommandProcessor() const {
        return processor;
    }

    // �������� ��������� �� ������
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