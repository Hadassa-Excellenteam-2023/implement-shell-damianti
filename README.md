# Linux Shell

This is a basic Linux shell program implemented in C++. It provides a basic command-line interface where the user can execute various commands,
both built-in and external.

## Features

- Accepts user commands and executes them.
- Supports built-in commands:
  - `cd`: Change directory.
  - `myjobs`: List background processes.
- Executes external commands by forking child processes and using `execvp`.
- Supports running commands in the background using the `&` symbol.
- Tracks and displays information about background processes.
- Cleans up finished background processes.

## Getting Started

1. Clone the repository:

   ```shell
   git clone https://github.com/Hadassa-Excellenteam-2023/implement-shell-damianti

2. Compile the source code:
   ```shell
   g++ -o shell linux-project.cpp

3. Run the shell:
	```shell
	./shell

4. Enter commands at the prompt and press Enter to execute them.

## Usage

- Enter any Linux command to execute it.
- Use the cd command to change directories.
- Use the myjobs command to list background processes.


## Examples
1. Execute an external command:
	```shell
	ls -l

2. Change directory:
	```shell
	cd /home

3. List background processes:
	```shell
	myjobs