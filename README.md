A simple C++ implementation of a Linux-like shell that provides core shell functionality.
Features

Command Execution: Run both built-in and external commands
Built-in Commands:

cd - Change directory
pwd - Print working directory
echo - Display text with variable expansion
export - Set environment variables
exit - Exit the shell


Piping: Chain commands with the pipe operator (|)
I/O Redirection:

Input redirection (<)
Output redirection (>)
Append output (>>)


Environment Variables: Use and expand variables (e.g., $HOME)
Quoted Strings: Support for quoted arguments

Building
Compile with a C++17 compatible compiler:
bashCopyg++ -std=c++17 shell.cpp -o myshell
Usage
Run the shell:
bashCopy./myshell
Example commands:
bashCopy# Basic commands
echo Hello World
pwd
cd /tmp

# Environment variables
export NAME=value
echo $NAME

# Redirection
echo hello > file.txt
cat < file.txt

# Piping
ls | grep .txt
