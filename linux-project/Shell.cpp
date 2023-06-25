#include "Shell.h"

// Default constructor for the Shell class
Shell::Shell()
{}

// Main function for the Shell class that starts the shell
void Shell::run(){
    /**
     * Runs the shell program, continuously prompts for user commands, and executes them.
     * It handles the execution of built-in commands (cd and myjobs) and external commands.
     */

    char** argumentArray = NULL;
    std::string commandLine;

    std::cout << "Enter a command: ";
    std::getline(std::cin, commandLine);

    while (true) {

        try {

            cleanUpFinishedProcesses();
            pid_t pid;
            std::istringstream iss(commandLine);
            auto argumentList = getArgumentListFromUser(iss);

            if (argumentList[0] == "exit")
                break;

            bool runInBackground = checkIfRunInBackground(argumentList);

            argumentArray = createArrayFromVector(argumentList);

            if (!isBuiltInProgram(argumentList, argumentArray)){
                pid = fork();

                if (pid == 0) {
                    runChildProcess(argumentArray);
                }

                else if (pid > 0) {
                    runParentProcess(argumentArray, pid, runInBackground, commandLine);
                }

                else {
                    throw std::runtime_error("Fork failed.");
                }
            
            }
        }
        catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }

        std::cout << "Enter a command: ";
        std::getline(std::cin, commandLine);

    }
    if (argumentArray)
        delete[] argumentArray;
        
}

std::vector<std::string> Shell::getArgumentListFromUser(std::istringstream& iss) {
    /**
     * Parses the user input command line and returns a vector of individual arguments.
     * The arguments are separated by whitespace.
     *
     * @param iss The input string stream containing the user command line.
     * @return A vector of individual arguments from the user command.
     */

    std::vector<std::string> argumentList;
    std::string argument;
    while (iss >> argument) {
        argumentList.push_back(argument);
    }

    if (argumentList.empty()) {
        throw std::runtime_error("Error: No command entered.");
    }

    return argumentList;
}

bool Shell::checkIfRunInBackground(std::vector<std::string>& argumentList) {
    /**
     * Checks if the given argument list specifies running the command in the background.
     * It determines if the command should execute and return immediately (run in the background).
     * As in the linux terminal, a process can be run in background if '&' is been used as last char
     * If is a new word xor if is the last char of the last command word (not both of them)
     *
     * @param argumentList The list of arguments.
     * @return True if the command should run in the background, false otherwise.
     */

    bool runInBackground = false;
    if (argumentList.back() == "&") {
        runInBackground = true;
        argumentList.pop_back();
    }

    else if (!argumentList.back().empty() && argumentList.back().back() == '&') {
        runInBackground = true;
        argumentList.back().pop_back();
    }

    return runInBackground;
}

char** Shell::createArrayFromVector(const std::vector<std::string>& argumentList) {
    /**
     * Converts a vector of strings into a dynamically allocated array of C-style strings (char**).
     *
     * @param argumentList The vector of strings to be converted.
     * @return A dynamically allocated array of C-style strings terminated with a NULL pointer.
     */
    
    char** argumentArray = new char* [argumentList.size() + 1];

    for (size_t i = 0; i < argumentList.size(); ++i) {
        argumentArray[i] = const_cast<char*>(argumentList[i].c_str());
    }

    argumentArray[argumentList.size()] = nullptr;

    return argumentArray;
}

bool Shell::isBuiltInProgram(const std::vector<std::string>& argumentList, char* const* argumentArray) {
    /**
     * Checks if the given command is a built-in program (cd or myjobs).
     * If the command is built-in, it performs the corresponding action (change directory or show background processes).
     *
     * @param argumentList The list of arguments.
     * @param argumentArray The array of C-style strings representing the command and its arguments.
     * @return True if the command is a built-in program, false otherwise.
     */

    char isBuiltInProgram = false;
    if (argumentList[0] == "cd") {

        if (argumentList.size() < 2) {
            throw std::runtime_error("Error: No directory specified for cd command.");
        }

        else {
            if (chdir(argumentArray[1]) != 0) {
                throw std::runtime_error("Error: Failed to change directory.");
            }

        }

        isBuiltInProgram = true;
    }

    else if (argumentList[0] == "myjobs") {
        if (argumentList.size() > 1) {
            throw std::runtime_error("Error: expected 1 argument. More than 1 was given.");
        }

        showBackgroundProcesses();
        isBuiltInProgram = true;
    }

    return isBuiltInProgram;
}

void Shell::runChildProcess(char* const* argumentArray) {
    /**
     * Executes the child process for an external command using execvp.
     * If the command execution fails, it displays an error message and terminates the child process.
     *
     * @param argumentArray The array of C-style strings representing the command and its arguments.
     */

    int statusCode = execvp(argumentArray[0], argumentArray);
    if (statusCode == -1) {
        std::cerr << argumentArray[0] << ": command not found.\n";
        exit(1);
    }
}

void Shell::runParentProcess(char* const* argumentArray, const pid_t& pid, const bool& runInBackground, const std::string& commandLine) {
    /**
     * Executes the parent process for an external command.
     * It waits for the child process to finish if not running in the background.
     * If running in the background, it adds the background process to the backgroundProcesses_ map.
     *
     * @param argumentArray The array of C-style strings representing the command and its arguments.
     * @param pid The process ID of the child process.
     * @param runInBackground Flag indicating if the command should run in the background.
     * @param commandLine The user command line.
     */

    int returnStatus;

    if (!runInBackground) {
        if (waitpid(pid, &returnStatus, 0) == -1) {
            throw std::runtime_error("Wait error.");
        }

    }
    else {
        m_backgroundProcesses[pid] = { commandLine, -1, std::time(0) };
        std::cout << "Process running in the background with PID: " << pid << std::endl;
    }

}

void Shell::showBackgroundProcesses() {
    /**
     * Displays the information about the background processes currently running.
     * It prints the process ID, command, status, and additional details (e.g., time running).
     */

    if (m_backgroundProcesses.size() == 0)
        std::cout << "No background processes running\n";

    else {
        for (const auto& [pid, jobInfo] : m_backgroundProcesses) {
            std::cout << "PID: " << pid 
                << " | Command: " << jobInfo.command 
                << " | Status: " << jobInfo.status 
                << " | time running: " << difftime(std::time(0), jobInfo.startTime) << " seconds\n";
        
        }
    }
}

void Shell::cleanUpFinishedProcesses() {
    /**
     * Cleans up the finished background processes by checking their status using waitpid.
     * It removes the finished processes from the backgroundProcesses_ map and prints relevant information.
     */

    for (auto it = m_backgroundProcesses.begin(); it != m_backgroundProcesses.end();) {
        int status;
        pid_t result = waitpid(it->first, &status, WNOHANG);
        
        if (result == it->first) {
            std::cout << "process with pid: " << it->first 
                << " runned by command: " << it->second.command
                << " has finished with status " << WEXITSTATUS(status)
                <<". Execution time : " << difftime(std::time(0), it->second.startTime) << " seconds\n";
            m_backgroundProcesses.erase(it++);
        }

        else {
            ++it;
        }

    }
}