# CMakeList.txt : CMake project for linux-project, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (linux-project  "main.cpp" "Shell.h" "Shell.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET linux-project PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
