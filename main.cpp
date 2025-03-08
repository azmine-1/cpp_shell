#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstring>
#include <map>
#include <pwd.h>
#include <filesystem>

class Shell {
private:
    std::string current_dir;
    std::map<std::string, std::string> env_vars;
    bool running;
    
    // Split command into tokens
    std::vector<std::string> tokenize(const std::string& command) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream stream(command);
        
        while (stream >> token) {
            // Handle quoted strings
            if ((token.front() == '"' || token.front() == '\'') && 
                token.front() != token.back()) {
                std::string quoted = token;
                std::string extra;
                while (stream >> extra) {
                    quoted += " " + extra;
                    if (extra.back() == token.front()) {
                        break;
                    }
                }
                
                if (quoted.length() >= 2 && quoted.front() == quoted.back()) {
                    quoted = quoted.substr(1, quoted.length() - 2);
                }
                tokens.push_back(quoted);
            } else if (token.front() == token.back() && 
                      (token.front() == '"' || token.front() == '\'') && 
                      token.length() > 1) {
                
                tokens.push_back(token.substr(1, token.length() - 2));
            } else {
                tokens.push_back(token);
            }
        }
        
        return tokens;
    }
    
    
    struct Command {
        std::vector<std::string> args;
        std::string input_file;
        std::string output_file;
        bool append_output;
        std::vector<Command> pipe_commands;
    };
    
    
    std::string expand_env_vars(const std::string& token) {
        std::string result = token;
        size_t pos = 0;
        
        while ((pos = result.find('$', pos)) != std::string::npos) {
            if (pos + 1 < result.length()) {
                size_t end = pos + 1;
                while (end < result.length() && (isalnum(result[end]) || result[end] == '_')) {
                    end++;
                }
                
                std::string var_name = result.substr(pos + 1, end - pos - 1);
                std::string var_value;
                
                if (env_vars.find(var_name) != env_vars.end()) {
                    var_value = env_vars[var_name];
                } else if (const char* env = getenv(var_name.c_str())) {
                    var_value = env;
                }
                
                result.replace(pos, end - pos, var_value);
                pos += var_value.length();
            } else {
                pos++;
            }
        }
        
        return result;
    }
    
    Command parse_command(const std::vector<std::string>& tokens) {
        Command cmd;
        cmd.append_output = false;
        
        for (size_t i = 0; i < tokens.size(); i++) {
            std::string expanded_token = expand_env_vars(tokens[i]);
            
            
            if (expanded_token == "|") {
                if (i + 1 < tokens.size()) {
                    std::vector<std::string> remainder(tokens.begin() + i + 1, tokens.end());
                    cmd.pipe_commands.push_back(parse_command(remainder));
                    break;
                }
            }
            
            else if (expanded_token == "<") {
                if (i + 1 < tokens.size()) {
                    cmd.input_file = expand_env_vars(tokens[++i]);
                }
            }
            
            else if (expanded_token == ">") {
                if (i + 1 < tokens.size()) {
                    cmd.output_file = expand_env_vars(tokens[++i]);
                    cmd.append_output = false;
                }
            }
            
            else if (expanded_token == ">>") {
                if (i + 1 < tokens.size()) {
                    cmd.output_file = expand_env_vars(tokens[++i]);
                    cmd.append_output = true;
                }
            }
            else {
                cmd.args.push_back(expanded_token);
            }
        }
        
        return cmd;
    }
    
    
    int execute_command(const Command& cmd) {
        if (cmd.args.empty()) {
            return 0;
        }
        
        // Handle built-in commands
        if (cmd.args[0] == "cd") {
            return handle_cd(cmd.args);
        } else if (cmd.args[0] == "exit") {
            running = false;
            return 0;
        } else if (cmd.args[0] == "export") {
            return handle_export(cmd.args);
        } else if (cmd.args[0] == "pwd") {
            std::cout << current_dir << std::endl;
            return 0;
        } else if (cmd.args[0] == "echo") {
            return handle_echo(cmd.args);
        }
        
        return execute_external_command(cmd);
    }
    
    int handle_cd(const std::vector<std::string>& args) {
        std::string path;
        
        if (args.size() == 1 || args[1] == "~") {
            struct passwd* pw = getpwuid(getuid());
            path = pw->pw_dir;
        } else {
            path = args[1];
        }
        
        if (chdir(path.c_str()) == 0) {
            char buffer[1024];
            if (getcwd(buffer, sizeof(buffer)) != nullptr) {
                current_dir = buffer;
                env_vars["PWD"] = current_dir;
                return 0;
            }
        }
        
        std::cerr << "cd: " << path << ": No such file or directory" << std::endl;
        return 1;
    }
    
    int handle_export(const std::vector<std::string>& args) {
        for (size_t i = 1; i < args.size(); i++) {
            size_t pos = args[i].find('=');
            if (pos != std::string::npos) {
                std::string name = args[i].substr(0, pos);
                std::string value = args[i].substr(pos + 1);
                env_vars[name] = value;
                setenv(name.c_str(), value.c_str(), 1);
            }
        }
        return 0;
    }
    
    int handle_echo(const std::vector<std::string>& args) {
        for (size_t i = 1; i < args.size(); i++) {
            std::cout << expand_env_vars(args[i]);
            if (i < args.size() - 1) {
                std::cout << " ";
            }
        }
        std::cout << std::endl;
        return 0;
    }
    
    int execute_external_command(const Command& cmd) {
        
        if (cmd.pipe_commands.empty()) {
            return execute_simple_command(cmd);
        }
        

        int prev_pipe[2] = {-1, -1};
        int status = 0;
        
        
        if (pipe(prev_pipe) == -1) {
            perror("pipe");
            return 1;
        }
        
        pid_t first_pid = fork();
        if (first_pid == 0) {
            close(prev_pipe[0]);  
            
            
            if (dup2(prev_pipe[1], STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            close(prev_pipe[1]);
            
            
            if (!cmd.input_file.empty()) {
                int in_fd = open(cmd.input_file.c_str(), O_RDONLY);
                if (in_fd == -1) {
                    perror("open input");
                    exit(EXIT_FAILURE);
                }
                if (dup2(in_fd, STDIN_FILENO) == -1) {
                    perror("dup2 input");
                    exit(EXIT_FAILURE);
                }
                close(in_fd);
            }
            
            execute_program(cmd.args);
            exit(EXIT_FAILURE);  
        }
        
        
        for (size_t i = 0; i < cmd.pipe_commands.size() - 1; i++) {
            int current_pipe[2];
            if (pipe(current_pipe) == -1) {
                perror("pipe");
                return 1;
            }
            
            pid_t pid = fork();
            if (pid == 0) {
                
                close(current_pipe[0]);
                
                
                if (dup2(prev_pipe[0], STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
                close(prev_pipe[0]);
                close(prev_pipe[1]);
                
                
                if (dup2(current_pipe[1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
                close(current_pipe[1]);
                
                execute_program(cmd.pipe_commands[i].args);
                exit(EXIT_FAILURE);  
            }
            
            
            close(prev_pipe[0]);
            close(prev_pipe[1]);
            prev_pipe[0] = current_pipe[0];
            prev_pipe[1] = current_pipe[1];
        }
        
        
        pid_t last_pid = fork();
        if (last_pid == 0) {
            
            if (dup2(prev_pipe[0], STDIN_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            close(prev_pipe[0]);
            close(prev_pipe[1]);
            
            n
            if (!cmd.pipe_commands.back().output_file.empty()) {
                int flags = O_WRONLY | O_CREAT;
                flags |= cmd.pipe_commands.back().append_output ? O_APPEND : O_TRUNC;
                
                int out_fd = open(cmd.pipe_commands.back().output_file.c_str(), 
                                 flags, 0644);
                if (out_fd == -1) {
                    perror("open output");
                    exit(EXIT_FAILURE);
                }
                if (dup2(out_fd, STDOUT_FILENO) == -1) {
                    perror("dup2 output");
                    exit(EXIT_FAILURE);
                }
                close(out_fd);
            }
            
            execute_program(cmd.pipe_commands.back().args);
            exit(EXIT_FAILURE);  
        }
        
        
        close(prev_pipe[0]);
        close(prev_pipe[1]);
        
        
        waitpid(first_pid, &status, 0);
        for (size_t i = 0; i < cmd.pipe_commands.size(); i++) {
            int child_status;
            wait(&child_status);
            if (WIFEXITED(child_status) && WEXITSTATUS(child_status) != 0) {
                status = child_status;
            }
        }
        
        return WEXITSTATUS(status);
    }
    
    int execute_simple_command(const Command& cmd) {
        pid_t pid = fork();
        if (pid == 0) {
        
            
            
            if (!cmd.input_file.empty()) {
                int in_fd = open(cmd.input_file.c_str(), O_RDONLY);
                if (in_fd == -1) {
                    perror("open input");
                    exit(EXIT_FAILURE);
                }
                if (dup2(in_fd, STDIN_FILENO) == -1) {
                    perror("dup2 input");
                    exit(EXIT_FAILURE);
                }
                close(in_fd);
            }
            
            
            if (!cmd.output_file.empty()) {
                int flags = O_WRONLY | O_CREAT;
                flags |= cmd.append_output ? O_APPEND : O_TRUNC;
                
                int out_fd = open(cmd.output_file.c_str(), flags, 0644);
                if (out_fd == -1) {
                    perror("open output");
                    exit(EXIT_FAILURE);
                }
                if (dup2(out_fd, STDOUT_FILENO) == -1) {
                    perror("dup2 output");
                    exit(EXIT_FAILURE);
                }
                close(out_fd);
            }
            
            execute_program(cmd.args);
            exit(EXIT_FAILURE);  
        } else if (pid > 0) {
            
            int status;
            waitpid(pid, &status, 0);
            return WEXITSTATUS(status);
        } else {
            // Fork failed
            perror("fork");
            return 1;
        }
    }
    
    void execute_program(const std::vector<std::string>& args) {
        if (args.empty()) {
            exit(EXIT_FAILURE);
        }
        
        
        std::vector<char*> c_args;
        for (const auto& arg : args) {
            c_args.push_back(const_cast<char*>(arg.c_str()));
        }
        c_args.push_back(nullptr);
        
        
        execvp(c_args[0], c_args.data());
        
        
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    
public:
    Shell() : running(true) {
        char buffer[1024];
        if (getcwd(buffer, sizeof(buffer)) != nullptr) {
            current_dir = buffer;
        } else {
            current_dir = "/";
        }
        
       
        env_vars["PWD"] = current_dir;
        env_vars["SHELL"] = "myshell";
        
        
        char** env = environ;
        while (*env) {
            std::string env_var = *env;
            size_t pos = env_var.find('=');
            if (pos != std::string::npos) {
                env_vars[env_var.substr(0, pos)] = env_var.substr(pos + 1);
            }
            env++;
        }
    }
    
    void run() {
        std::string command;
        
        while (running) {
           
            std::string username = getenv("USER") ? getenv("USER") : "user";
            std::string hostname = "localhost";
            char host_buffer[256];
            if (gethostname(host_buffer, sizeof(host_buffer)) == 0) {
                hostname = host_buffer;
            }
            
            std::cout << username << "@" << hostname << ":" << current_dir << "$ ";
            
            
            if (!std::getline(std::cin, command)) {
                break;
            }
            
            
            if (command.empty()) {
                continue;
            }
            
            
            std::vector<std::string> tokens = tokenize(command);
            if (tokens.empty()) {
                continue;
            }
            
            
            Command cmd = parse_command(tokens);
            
            
            execute_command(cmd);
        }
    }
};


int main() {
    Shell shell;
    shell.run();
    return 0;
}

