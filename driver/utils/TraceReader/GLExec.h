#ifndef GLEXEC_H
#define GLEXEC_H

#include <vector>
#include "GLResolver.h"
#include "GLJumpTable.h"
#include "TraceReader.h"
#include "GenericParam.h"

/**
 * Wrapper class for easy identification an execution of opengl commands
 * contained in a tracefile
 * It wraps TraceFile access, Buffers access, TraceFile modes and
 * function text checking, opengl32.dll functions loading and more
 *
 * @code
 * // common use
 *     GLExec gle;
 *     int error = gle.init("traceCommands.dat", "traceBuffers.dat");
 *     // error == -1 : Error loading opengl32.dll functions
 *     // error == -2 : Error opening tracefile ( not found or cannot be opened )
 *     // error == -2 : Error opening buffer file ( not found or cannot be opened )
 *     APICall currentCommand = gle.getCurrentCall();
 *     while ( currectCall != APICall_UNDECLARED )
 *     {
 *         if ( CHECKCALL(currentCall) )
 *         {
 *             // do some special things with this call
 *             // i.e Statistics, update simething...
 *         }
 *         gle.executeCurrentCall(); // consume this call (execute it)
 *         currentCall = gle.getCurrentCall(); // take next call ID
 *     }
 *
 *  @endcode
 *
 *  A more simple code can be writen if CHECKCALL is not required
 *  @code
 *
 *      // this code execute all calls in a tracefile
 *      while ( gle.executeCurrentCall() != APICall_UNDECLARED ) ;
 *
 *  @endcode
 */
class GLExec
{
private:
    bool                currentExec; // current call executed ?
    APICall             current; // current call identifier
    TraceReader         tr; // opened in init call time
    GLJumpTable         jt; // loaded in init call time
    GLuint              savedTracePos;
    bool                enableStats;
    ofstream            statsFile;
    static const char   sepStats = ';'; 
protected:
    // User call table
    GLJumpTable         uct; // Allows to execute user code each time a opengl function is called    
    /**
     * This call is called just before finishing the init method.
     * In this call you should define the callbacks for each opengl
     * call you are interested in
     * By default does nothing
     */
    virtual void addUserCalls();
    virtual void processPerFrameStats();    // Called just after finish a frame
public:
    GLExec(); // * File from where trace will be read
    long saveTracePosition(); //* Used to save/restore a point into trace file
    void restoreTracePosition();
    void setDirectivesEnabled(bool enabled);
    bool areDirectivesEnabled() const;
    bool checkBatchesAsFrames() { return tr.checkBatchesAsFrames(); }
    /*
     *  0 : OK
     * -1 : Error loading opengl32.dll functions
     * -2 : Error opening tracefile
     * -3 : Error opening buffer descriptors file
     * -4 : Error opening memory regions file
     */
    int init( const char* trace, 
              const char* buffersFile, 
              const char* regionsFile,
              bool enableStats = false );

    /**
     * Gets the current APICall identifier ( for the current call )
     */
    APICall getCurrentCall();

    /**
     * 
     */
    void skipCurrentCall();

    /**
     * Returns the defined resolution in the trace
     * if 0,0 returned means that there is not a resolution defined in the trace
     */
    void getTraceResolution(unsigned int& width, unsigned int& height);

    /**
     * Returns the value of the selected Param from the last call executed
     */
    Param getCurrentParam(unsigned int iParam);

    /**
     * Sets new contents for a given param
     */ 
    void setCurrentParam(unsigned int iParam, const Param& p);

    /**
     * Execute the current call and update TraceReader pointing to next call
     *
     * returns call just executed
     */
    APICall executeCurrentCall();
    GLJumpTable& jTable() { return jt; }    
    GLJumpTable& ucTable() { return uct; }

    BufferManager& getBufferManager() { return tr.getBufferManager(); }
    std::string getSpecialString() const { return tr.getSpecialString(); }
    void resetSpecialString() { tr.resetSpecialString(); }
    unsigned int currentTracefileLine() const { return tr.currentLine(); }

    /**
     * Checks if an apicall is currently available (exists a pointer != 0) in the internal function 
     * table used by GLExec
     */
    bool isCallAvailable(APICall apicall);
};

#endif // GLEXEC_H
