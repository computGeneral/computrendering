/**************************************************************************
 *
 */

#ifndef LOGOBJECT_H
#define LOGOBJECT_H

#include <ostream>
#include <string>
#include <fstream>
#include <queue>

/**
 * Log class allowing 32 different log levels [0..31]
 */
class LogObject
{
public:

    typedef unsigned int LevelMask;
    typedef unsigned int Line;

    LogObject();

    void enable(); // first call before using the log (enable log methods, including open())
    void disable(); // disable all method calls to log object (preserve log state)

    bool close();

    // read configuration parameters from a file
    bool config(std::string path);

    // Write into the current logfile the current configuration of this log object
    void writeConfig();

    bool open(std::string path);

    // by default is 0 (all is logged)

    void enableLevels(LevelMask lMask);

    void logTime(bool yes); // by default false
    void logFile(bool yes); // by default false
    void logFunction(bool yes); // by default false
    void logLine(bool yes); // by default false

    void pushInfo(std::string curFile, std::string curFunc = "", Line curLine = 0);

    void popInfo();

    // by default is true
    void setAutoFlush(bool enable);

    bool write(LevelMask l, const std::string& message, bool dumpLogExtras = true);
    bool write(LevelMask l, const char* message, bool dumpLogExtras = true);

private:

    struct PosInfo
    {
        std::string file;
        std::string func;
        Line line;

        PosInfo(std::string file, std::string func, Line line) :
            file(file), func(func), line(line) {}
    };

    std::queue<PosInfo> posInfo;

    std::ofstream o;
    std::string opath;
    LevelMask levels;
    
    bool writeTime;
    bool writeFile;
    bool writeFunction;
    bool writeLine;    

    bool autoflush;

    bool enabled;
};

#endif // LOGOBJECT_H
