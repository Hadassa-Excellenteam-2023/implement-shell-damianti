#include "Shell.h"
#include <stdexcept>


int main()
{
    try {
        auto shell = Shell();
        
        shell.run();


       

    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}


