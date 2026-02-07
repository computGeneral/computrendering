/**************************************************************************
 *
 */

#ifndef WRAPPERGENERATOR_H
    #define WRAPPERGENERATOR_H

#include "CodeGenerator.h"
#include <set>
#include <string>

/** 
 * generateCode: Generates files glWrapper.h & glWrapper.cpp
 */
class WrapperGenerator : public FuncGenerator
{
private:
    
    char oDirGen[256];
    const FuncExtractor& fe;
    const SpecificExtractor& se;
    const FuncExtractor& feStats;
    bool *isCallPlayable;
    std::set<std::string>* candidates; // avoid or generate code for tracing these calls
    bool genTraceCode; // avoid(false)/generate(true) flag
    
    /**
     * Dumps a list of unplayable call
     *
     * That is, the call is logged but cannot be player by GLTracePlayer
     */
    //std::ofstream listOfUnplayableCalls;

    int countUnplayables; // How many calls are unplayable 

    bool generateWrapperCall( std::ostream& fout, const char* writerName,
                              const char* jumpTableName, const FuncDescription& fd,
                              const SpecificItem* sfd, bool hasSpecificStat = false );

    bool generateAllWrapperCalls( std::ostream& f, const char* writerName, const char* jumpTableName, 
                              const FuncExtractor& fe );

public:
    
    WrapperGenerator( const FuncExtractor& fe, const SpecificExtractor& se, 
                      const FuncExtractor& feStats, const char* oDirWrapper, 
                      const char* oDirGen );
    
    WrapperGenerator( const FuncExtractor& fe, const SpecificExtractor& se, 
                      const FuncExtractor& feStats, const char* oDirWrapper, 
                      const char* oDirGen, std::set<std::string>& candidates,
                      bool genTraceCode);


    bool generateCode();

    void dumpInfo() const;
};

#endif // WRAPPERGENERATOR_H
