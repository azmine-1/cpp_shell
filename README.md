
A simple C++ implementation of a Linux-like shell that provides core shell functionality.


- **Built-in Commands**:
  - `cd` - Change directory
  - `pwd` - Print working directory
  - `echo` - Display text with variable expansion
  - `export` - Set environment variables
  - `exit` - Exit the shell

- **Piping**: Chain commands with the pipe operator (`|`)
- **I/O Redirection**: 
  - Input redirection (`<`)
  - Output redirection (`>`)
  - Append output (`>>`)
- **Environment Variables**: Use and expand variables (e.g., `$HOME`)
- **Quoted Strings**: Support for quoted arguments


```bash
g++ -std=c++17 shell.cpp -o myshell
```


Run the shell:
```bash
./myshell
```

Example commands:
```bash
# Basic commands
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
```
![image](https://github.com/user-attachments/assets/4512e869-7009-4b5c-a10b-37c66b4a223e)
