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


struct JobInfo {
	std::string command;
	int status;
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
	bool isBuiltInProgram(const std::vector<std::string>& argumentList, char* const* argumentArray);
	void runChildProcess( char* const* argumentArray);
	void runParentProcess(char* const* argumentArray, const pid_t&, const bool& runInBackground, const std::string& commandLine);
	void showBackgroundProcesses();
	void cleanUpFinishedProcesses();
	std::map< pid_t, JobInfo> m_backgroundProcesses;
};


