#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <sys/wait.h>
#include <stdexcept>
#include <map>
#include <ctime>
#include <fcntl.h>
#include <memory>

struct JobInfo {
	std::string command;
	std::time_t startTime;

};

class Shell {
public:
	Shell();
	void run();
private:
	std::vector<std::string> getArgumentListFromUser(std::istringstream& iss);
	bool checkIfRunInBackground(std::vector<std::string>& argumentList);
	char** createArrayFromVector(const std::vector<std::string>& argumentList);
	bool isBuiltInProgram(const std::vector<std::string>& argumentList);
	void runChildProcess(std::vector<std::string>& argumentList);
	void runParentProcess(const pid_t&, const bool& runInBackground, const std::string& commandLine);
	std::vector<std::string> executeRedirections(std::vector<std::string>& argumentList);
	bool isPipelined(const std::vector<std::string>& argumentList);
	void executePipedCommands(std::vector<std::string>& commandArguments);
	std::vector<std::vector<std::string>> parseCommands(const std::vector<std::string>& argumentList);
	void executeCommand(const std::vector<std::string>& argumentList);
	void showBackgroundProcesses();
	void cleanUpFinishedProcesses();
	std::map< pid_t, JobInfo> m_backgroundProcesses;
};


