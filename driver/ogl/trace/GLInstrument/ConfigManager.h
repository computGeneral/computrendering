/**************************************************************************
 *
 */

#ifndef CONFIGMANAGER_H
    #define CONFIGMANAGER_H
#include <string>
#include <map>

class ConfigManager
{
public:

    bool loadConfigFile(std::string path, std::string separator = "=");
    bool saveConfigFile(std::string path, std::string separator = "=");
    std::string getOption(std::string option);
    std::pair<std::string, std::string> getOption(int pos);
    void setOption(std::string option, std::string value);

    std::map<std::string,std::string> getOptions() const { return options; }
    int countOptions() const;


private:

    std::map<std::string, std::string> options;
    std::string separator;
};

#endif // CONFIGMANAGER_H
