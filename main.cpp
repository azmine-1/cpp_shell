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
int main() {
    Shell shell;
    shell.run();
    return 0;
}
