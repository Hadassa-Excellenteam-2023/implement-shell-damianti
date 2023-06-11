#include "Shell.h"

// Default constructor for the Shell class
Shell::Shell()
{}

//-----------------------------------------------------------------------------

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


            if (!isBuiltInProgram(argumentList)){
                pid = fork();

                if (pid == 0) {
                    runChildProcess(argumentList);
                }

                else if (pid > 0) {
                    runParentProcess(pid, runInBackground, commandLine);
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

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

bool Shell::isBuiltInProgram(const std::vector<std::string>& argumentList) {
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
            if (chdir(const_cast<char*>(argumentList[1].c_str())) != 0) {
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

//-----------------------------------------------------------------------------

void Shell::runChildProcess(std::vector<std::string>& argumentList) {
    /**
     * Executes the child process for an external command.
     *
     * The function determines if the command is pipelined, in which case it executes the piped commands.
     * If the command isn't pipelined, it checks for any input or output redirections and applies them.
     * After preparing the command, it is finally executed using execvp.
     *
     * In case of execution failure, an error message is displayed and the child process is terminated.
     *
     * @param argumentList The vector of strings representing the command and its arguments.
     */

    if (isPipelined(argumentList))
        executePipedCommands(argumentList);
       
    else {
        auto finalArgs = executeRedirections(argumentList);
        executeCommand(finalArgs);
        
    }
    
   
}

//-----------------------------------------------------------------------------

void Shell::runParentProcess(const pid_t& pid, const bool& runInBackground, const std::string& commandLine) {
    /**
     * Executes the parent process for an external command.
     * It waits for the child process to finish if not running in the background.
     * If running in the background, it adds the background process to the backgroundProcesses_ map.
     *
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
        m_backgroundProcesses[pid] = { commandLine, std::time(0) };
        std::cout << "Process running in the background with PID: " << pid << std::endl;
    }

}

//-----------------------------------------------------------------------------

std::vector<std::string> Shell::executeRedirections(std::vector<std::string>& argumentList) {
    /**
     * Processes and executes any input or output redirections specified in the command arguments.
     *
     * This function iterates over the command argument list and looks for the following redirection operators: '<', '>', '>>'.
     * If an input redirection ('<') is found, the function attempts to open the specified input file.
     * If an output redirection ('>') is found, the function attempts to open or create the specified output file, overwriting if it already exists.
     * If an append output redirection ('>>') is found, the function attempts to open or create the specified output file, appending to it if it already exists.
     *
     * After processing all redirections, the function updates the standard input and/or output of the process to match the specified redirections.
     * It then returns the command arguments with the redirection operators and file names removed.
     *
     * In case of failure in opening or creating any of the files, the function displays an error message and terminates the process.
     *
     * @param argumentList The vector of strings representing the command and its arguments.
     * @return A vector of strings representing the command and its arguments, with redirection operators and file names removed.
     */

    std::vector <std::string> finalArgs;
    int inputRedirection = -1;
    int outputRedirection = -1;
    int outputRedirectionAppend = -1;
    for (size_t i = 0; i < argumentList.size(); ++i) {

        if (argumentList[i] == "<") {

            const char* inputFile = const_cast<char*>(argumentList[i + 1].c_str());
            inputRedirection = open(inputFile, O_RDONLY);
            if (inputRedirection == -1) {
                std::cerr << "Error: Failed to open input file: " << inputFile << std::endl;
                exit(1);
            }
            i++;
        }

        else if (argumentList[i] == ">") {

            const char* outputFile = const_cast<char*>(argumentList[i + 1].c_str());
            outputRedirection = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (outputRedirection == -1) {
                std::cerr << "Error: Failed to open output file: " << outputFile << std::endl;
                exit(1);
            }
            i++;
        }

        else if (argumentList[i] == ">>") {
            const char* outputFile = const_cast<char*>(argumentList[i + 1].c_str());
            outputRedirectionAppend = open(outputFile, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (outputRedirectionAppend == -1) {
                std::cerr << "Error: Failed to open output file: " << outputFile << std::endl;
                exit(1);
            }
            i++;
        }

        else {
            finalArgs.push_back(argumentList[i]);
        }



    }

    if (outputRedirectionAppend != -1) {
        dup2(outputRedirectionAppend, STDIN_FILENO);
        close(outputRedirectionAppend);
    }
    if (inputRedirection != -1) {
        dup2(inputRedirection, STDIN_FILENO);
        close(inputRedirection);
    }
    if (outputRedirection != -1) {
        dup2(outputRedirection, STDOUT_FILENO);
        close(outputRedirection);
    }


    return finalArgs;

}

//-----------------------------------------------------------------------------

bool Shell::isPipelined(const std::vector<std::string>& argumentList) {
    /*   This function checks if the provided command string contains a pipeline.
     *
     * @param commands
     *   A vector of strings, where each string represents a command or argument in the user input.
     *
     * @return
     *   Returns true if the command contains a pipe ('|') character, indicating that the command is a pipeline.
     *   Returns false otherwise.
    */
    for (auto& it : argumentList) {
        if (it == "|")
            return true;
    }
    return false;
}

//-----------------------------------------------------------------------------

void Shell::executePipedCommands(std::vector<std::string>& argumentList) {
    /**
     *
     *   This function parses the argument list into separate commands, creates the necessary number of pipes, and then forks for each command.
     *   Each child process redirects its input or output to the appropriate pipe, if needed, and executes the corresponding command.
     *
     *   The order of operations is as follows:
     *   1. Parse the argument list into a list of commands.
     *   2. Create pipes.
     *   3. Loop through each command:
     *      - Fork a new process.
     *      - Handle input and output redirection for the process.
     *      - Execute the command.
     *   4. Close all pipe file descriptors in the parent process.
     *   5. Wait for all child processes to finish.
     *
     *   If an error occurs at any point (for example, if a pipe or fork operation fails), the function outputs an error message and exits with a failure status.
     *   
     *  @param argumentList
     *   A vector of strings, where each string represents a command or an argument. Commands are expected to be separated by pipe characters ('|').
     *
     * @return
     *   No return value.
     *
     */
    std::vector<std::vector<std::string>> commands = parseCommands(argumentList);
    

    int numPipes = commands.size() - 1;
    int pipefds[2 * numPipes];

    for (int i = 0; i < numPipes; i++) {
        if (pipe(pipefds + i * 2) < 0) {
            std::cerr << "Pipe creation failed." << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    int pid, wpid;
    int status;
    int commandIndex = 0;

    for (auto& command : commands) {

        pid = fork();
        if (pid == 0) {
            auto finalArgs = executeRedirections(command);
            
            // If not the first command, redirect previous read end of pipe to stdin
            if (commandIndex != 0) {
                if (dup2(pipefds[(commandIndex - 1) * 2], STDIN_FILENO) < 0) {
                    std::cerr << "dup2 failed." << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            // If not the last command, redirect current write end of pipe to stdout
            if (commandIndex != numPipes) {
                if (dup2(pipefds[commandIndex * 2 + 1], STDOUT_FILENO) < 0) {
                    std::cerr << "dup2 failed." << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            // Close all pipefds
            for (int i = 0; i < 2 * numPipes; i++) {
                close(pipefds[i]);
            }

            executeCommand(finalArgs);
            
        }
        else if (pid < 0) {
            std::cerr << "Fork failed." << std::endl;
            exit(EXIT_FAILURE);
        }
        commandIndex++;
    }

    // Close all pipefds in parent
    for (int i = 0; i < 2 * numPipes; i++) {
        close(pipefds[i]);
    }

    while ((wpid = wait(&status)) > 0);

    exit(0);


}

//-----------------------------------------------------------------------------

std::vector<std::vector<std::string>> Shell::parseCommands(const std::vector<std::string>& argumentList) {
    /**
     *   This function loops through the provided argument list. It groups arguments together into separate commands based on the location of pipe characters ('|').
     *   Each command is represented as a vector of strings. The entire sequence of commands is then returned as a vector of these command vectors.
     *
     *   If the argument list does not contain any pipe characters, the function will return a vector containing one command vector.
     *
     * @param argumentList
     *   A vector of strings, where each string represents a command or an argument. Commands are expected to be separated by pipe characters ('|').
     *
     * @return
     *   Returns a vector of vectors of strings. Each inner vector represents a separate command along with its arguments.
     *
     */
    std::vector<std::vector<std::string>> commands;
    std::vector<std::string> tempCommand;

    for (auto& arg : argumentList) {
        if (arg != "|") {
            tempCommand.push_back(arg);
        }
        else {
            commands.push_back(tempCommand);
            tempCommand.clear();
        }
    }
    commands.push_back(tempCommand);

    return commands;
}

//-----------------------------------------------------------------------------

void Shell::executeCommand(const std::vector<std::string>& commandArguments) {
    /**
     * @details
     *   This function uses the execvp system call to replace the current process image with a new process image specified by the commandArguments parameter.
     *   The arguments to the command are passed as an array of strings to execvp.
     *
     *   The function first converts the vector of strings into an array of char pointers using the createArrayFromVector helper function. This is necessary because execvp expects an array of char pointers, not a vector of strings.
     *
     *   If execvp fails to execute the command (i.e., it returns -1), an error message is printed to the standard error output and the function calls exit(EXIT_FAILURE) to terminate the program.
     *
     * @param commandArguments
     *   A vector of strings, where the first element is the command to be executed and the following elements are the command's arguments.
     *
     * @return
     *   This function does not return a value.
     */
    char** argumentArray = createArrayFromVector(commandArguments);

    if (execvp(argumentArray[0], argumentArray) < 0) {
        std::cerr << argumentArray[0] << ": command not found." << std::endl;
        exit(EXIT_FAILURE);
    }

}

//-----------------------------------------------------------------------------

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
                << " | time running: " << difftime(std::time(0), jobInfo.startTime) << " seconds\n";
        
        }
    }
}

//-----------------------------------------------------------------------------

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