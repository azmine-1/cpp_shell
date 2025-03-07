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
    std::vector<std::string> tokenize(const std::string& command) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream stream(command);
        
        while (stream >> token) {
            
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

int main() {
    Shell shell;
    shell.run();
    return 0;
}
