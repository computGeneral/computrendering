/**************************************************************************
 *
 */

#ifndef SPTCONSOLE_H
    #define SPTCONSOLE_H

#include "ShaderProgramTestBase.h"
#include <vector>
#include <list>

namespace arch
{

class SPTConsole : public ShaderProgramTestBase
{
private:

    static const unsigned int SPT_INVALID_INTEGER = static_cast<unsigned int>(-1);

    bool quitConsole;

    std::list<std::string> pendingCommands;

    std::string getPrompt() const;

    bool processCompileProgramCommand(const std::vector<std::string>& params);
    bool processDefineProgramInputCommand(const std::vector<std::string>& params);
    bool processDefineProgramConstantCommand(const std::vector<std::string>& params);
    bool processExecuteProgramCommand(const std::vector<std::string>& params);
    bool processDumpProgramCommand(const std::vector<std::string>& params);
    bool processTransformCommand(const std::vector<std::string>& params);
    bool processQuitCommand(const std::vector<std::string>& params);
    bool processHelpCommand(const std::vector<std::string>& params);
    bool processScriptCommand(const std::vector<std::string>& params);

public:

    void printWelcomeMessage() const;

    std::string parseParams(const std::vector<std::string>& params);

    bool finished();

    void execute();

    SPTConsole( const cgsArchConfig& arch, const char* name );

};

} // namespace arch

#endif // SPTCONSOLE_H
