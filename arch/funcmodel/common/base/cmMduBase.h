#ifndef __MDU_BASE_H__
#define __MDU_BASE_H__

#include "bmMduBase.h"
#include "GPUType.h"
#include "cmGPUSignal.h"
#include "cmSignalBinder.h"
#include "cmStatisticsManager.h"
#include <string>
#include <sstream>

//using namespace std;

namespace arch
{

#ifdef GPU_DEBUG_ON
    #define GPU_DEBUG_BOX(expr) { expr }
#else
    #define GPU_DEBUG_BOX(expr) if (debugMode) { expr }
#endif

/**
 *  Defines the threshold in cycles that marks when a queue or stage is considered
 *  to be stalled after no input or output is sent or received.
 */
static const U32 STALL_CYCLE_THRESHOLD = 1000000;

/**
 *   cmoMduBase class is a common interface for all Modules
 *
 * - Interface for all Modules, all modules must inherit from this class
 * - It is only an interface, can't be instanciated
 * - Has protected methods for registering signals
 *   ( newInputSignal & newOutputSignal versions )
 * - Has a @b static method @c getMdu( @c name @c ) that give you a pointer
 *   to the mdu with this name
 *
 * @note Latest modifications:
 *   - Added support for name's identification ( @c getMdu() )
 *   - Automatic registration of modules
 *   - Added support for create built-in statistics ( only U32BIT types at the moment )
 */
class cmoMduBase : public bmoMduBase
{
public:

    static void setSignalTracing(bool enabled, U64 startCycle, U64 dumpCycles);
    static bool isSignalTracingRequired(U64 currentCycle);

    /* Basic constructor called in initializers from derived modules to initialize
     * general mdu properties ( name and parent cmoMduBase )
     * Also adds the cmoMduBase to the list of 'alive' Modules
     * @param nameMdu name of the mdu
     * @param parentMdu A pointer to the parent cmoMduBase, if there is not parent mdu defined the parameter must be 0 */
    cmoMduBase( const char* nameMdu, cmoMduBase* parentMdu = 0 );
    /* Cleans static information when a mdu is deleted */
    virtual ~cmoMduBase();
    /* *  Assignment operator.  Implemented to remove some warnings.  */
    cmoMduBase& operator=(const cmoMduBase &in);

    /**
     * Work done in a cycle by the mdu
     * @note Must be implemented by all derived modules ( pure virtual )
     * @param cycle cycle in which clock is performed
     */
    virtual void clock( U64 cycle ) = 0;

    /**
     *  Returns a single line string with information about the state of the mdu.
     *  @param stateString Reference to a string were to return the state line.
     */
    virtual void getState(std::string &stateString);

    /**
     *  Returns a string with debug information about the mdu.  The information can
     *  span over multiple lines.
     *  @param debugInfo Reference to a string where to return debug information.
     */

    virtual void getDebugInfo(std::string &debugInfo) const;

    /**
     *  Returns a string with a list of the commands supported by the mdu.  The information
     *  may span multiple lines.
     *
     *  @param commandList Reference to a string where to return the command list.
     *
     */
    virtual void getCommandList(std::string &commandList);

    /**
     *  Executes a mdu command.
     *  @param commandStream A string stream with a mdu command and parameters.
     */
    virtual void execCommand(std::stringstream &commandStream);

    /**
     *  Sets the debug flag for the mdu.
     *  @param debug New value of the debug flag.
     */
    void setDebugMode(bool debug);
    
    /**
     *  Detects if the mdu is stalled.
     *  @param cycle Current simulation cycle.
     *  @param active Reference to a boolen variable where to store if the stall detection logic is active/implemented.
     *  @param stalled Reference to a boolean variable where to store if the mdu has been detected to be stalled.
     */
    virtual void detectStall(U64 cycle, bool &active, bool &stalled);

    /**
     *  Returns a string reporting the stall condition of the mdu.
     *  @param cycle Current simulation cycle.
     *  @param stallReport Reference to a string where to write the stall condition report for the mdu.
     */
    virtual void stallReport(U64 cycle, std::string &stallReport);
    
    /**
     * Gets name mdu
     * @return the name of the mdu
     */
    const char* getName() const;

    /**
     * Gets a pointer to parent's mdu, the returning can be NULL.
     * @return A pointer to the parent mdu
     */
    const cmoMduBase* getParent() const;

    /**
     * Gets a pointer to the mdu identified by name 'name'
     *
     * @param nameMdu name of the target mdu
     * @return A pointer to the requested mdu ( null if name is not the name of a "alive" mdu )
     */
    static cmoMduBase* getMdu( const char* nameMdu );

    /**
     *  Returns a newline separated list with the names of all the modules alive.
     *  @param boxList A reference to a string were to stored the names of the
     *  modules that are currently alive.
     */
    static void getMduNameList(std::string &boxList);

    // Dumps the name of all modules created so far
    static void dumpMduName();

private:
    // Auxiliar container to implement a list of 'alive' ( created and not destroyed ) modules
    struct cmsMduList {
        cmoMduBase* mdu;
        cmsMduList* next;
        // Basic constructor performs link operation in the list
        cmsMduList( cmoMduBase* aMdu, cmsMduList* theNext ) : mdu(aMdu), next(theNext) {}
    };

    char*         name;   // Name mdu
    cmoMduBase*   parent; // Pointer to the parent's mdu
    cmoSignalBinder& binder; // Reference to the global cmoSignalBinder

    static cmsMduList* mduList; // List of all modules created so far
    static gpuStatistics::StatisticsManager& sManager; // Reference to the global StatisticsManager

    static bool signalTracingFlag;
    static U64  startCycle;
    static U64  dumpCycles;

protected:
    bool debugMode;     //  Flag used to enable or disable debug messages.  

    /**
     * Registers a new input Signal
     *
     * @param name signal's name ( if no prefix is especified )
     * @param bandwidth bandwidth for this signal
     * @param latency latency for this signal
     * @param prefix if specified the the signal's name is prefix/name
     *
     * - Creates a new Signal for reading with defined bandwidth & latency (if the Signal did not exist ).
     * - Otherwise checks matching between previous values and returns the previous Signal created ( a pointer )
     * - An optional name can be appended to the first name: "prefix/name".
     * - 'bandwidth' should be defined with value > 0 to fast errors detecting.
     * - Defining latency or bandwidth like 0 makes latency or bandwidth remain undefined.
     *
     * @return a pointer to the signal
     */
    Signal* newInputSignal( const char* name, U32 bandwidth, U32 latency = 0, const char* prefix = 0 );

    /**
     * Registers a new input Signal ( preferable version ) prefix
     *
     * @param name signal's name ( if no prefix is especified )
     * @param bandwidth bandwidth for this signal
     * @param prefix if specified the the signal's name is prefix/name
     *
     * - Creates a new Signal for reading without defined latency  (if the Signal did not exist ).
     * - Otherwise checks matching between previous values and returns the previous Signal created ( a pointer )
     * - An optional name can be append to the first name: "prefix/name".
     * - 'bandwidth' should be defined with value > 0 to fast errors detecting.
     * - If bandwidth is set to 0 then bandwidth remains undefined.
     *
     * @return a pointer to the signal
     */
    Signal* newInputSignal( const char* name, U32 bandwidth, const char* prefix );

    /**
     * Registers a new output Signal ( preferable version )
     *
     * @param name signal's name ( if no prefix is especified )
     * @param bandwidth bandwidth for this signal
     * @param latency latency for this signal
     * @param prefix if specified the the signal's name is prefix/name
     *
     * - Creates a new Signal for writing with defined bandwidth & latency (if the Signal did not exist ).
     * - Otherwise checks matching between previous values and returns the previous Signal created ( a pointer )
     * - An optional name can be append to the first name: "prefix/name".
     * - 'bandwidth' should be defined with value > 0 to fast errors detecting.
     * - Defining latency or bandwidth like 0 makes latency or bandwidth remain undefined.
     *
     * @return a pointer to the signal
     */
    Signal* newOutputSignal( const char* name, U32 bandwidth, U32 latency, const char* prefix = 0);


    /**
     * Registers a new output Signal
     *
     * @param name signal's name ( if no prefix is especified )
     * @param bandwidth bandwidth for this signal
     * @param prefix if specified the the signal's name is prefix/name
     *
     * - Creates a new Signal for writing without defined bandwidth & latency  (if the Signal did not exist ).
     * - Otherwise checks matching between previous values and returns the previous Signal created ( a pointer )
     * - An optional name can be append to the first name: "prefix/name".
     * - 'bandwidth' should be defined with value > 0 to fast errors detecting.
     * - If bandwidth is set to 0 then bandwidth remains undefined.
     *
     * @return a pointer to the signal
     */
    Signal* newOutputSignal( const char* name, U32 bandwidth, const char* prefix = 0);

    /**
     * Gets a reference to StatisticsManager
     */
    static gpuStatistics::StatisticsManager& getSM();


};

} // namespace arch

#endif
