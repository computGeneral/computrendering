/**************************************************************************
 *
 */

#ifndef GLIENTRYPOINTSGENERATOR_H
    #define GLIENTRYPOINTSGENERATOR_H

#include "CodeGenerator.h"
#include <string>
#include <vector>

/**
 * Generator code for file GLIEntryPoints required in GLInstrument project
 */
class GLIEntryPointsGenerator : public FuncGenerator
{
private:

    const FuncExtractor& fe;

    bool generateEntryPointCall(std::ostream& out, std::string gliName, 
                                const FuncDescription& fd);

    bool generateAllEntryPointCalls(std::ostream& out, std::string gliName,
                                const FuncExtractor& fe);

    // return parameters name's
    std::vector<std::string> generateHeader(std::ostream& out, const FuncDescription& fd);

public:

    GLIEntryPointsGenerator(const FuncExtractor& fe, std::string outputDir);

    bool generateCode();

    void dumpInfo() const;
};



#endif // GLIENTRYPOINTSGENERATOR_H
