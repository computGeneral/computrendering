/**************************************************************************
 * Signal Binder class definition file.
 * cmoSignalBinder class manages with Signal connections [SINGLETON]
 *
 * Object reposible of all signal bindings and integrity of conections
 * Simple array of Signal pointers, with sequential access by name
 */

#ifndef __SIGNAL_BINDER__
   #define __SIGNAL_BINDER__

#include "GPUType.h"
#include "cmGPUSignal.h"
#include <cstdio>
#include <ostream>

namespace arch
{

class cmoSignalBinder {
private:
    enum { growth = 50 }; // Increase capacity when needed
    Signal** signals; // Signals registered
    char* bindingState; // Binding control
    U32 elements; // Count of signals registered
    U32 capacity; // Max capacity allowed

    std::ostream *ProfilingFile;    // Trace file handle.
    S32 find( const char* name ) const; // Aux method for finding positions in the binder
    cmoSignalBinder( U32 capacity = growth ); // cmoSignalBinder can't be instanciated directly and can't be copied
    cmoSignalBinder( const cmoSignalBinder& );// Copy constructor ( private avoid binder1 = binder2 )
    static cmoSignalBinder binder; // The unique cmoSignalBinder object
    static const flag BIND_MODE_RW; // Signal successfully binded
public:
    static const flag BIND_MODE_READ; // Binding read mode
    static const flag BIND_MODE_WRITE; // Binding write mode

    /**
     * Gets a reference to the binder ( an alias )
     * You must get and alias before doing something else
     *
     * @return A cmoSignalBinder reference to the global cmoSignalBinder
     */
    static cmoSignalBinder& getBinder();

    /**
     * Registers a name for a Signal
     * Type indicates if the signal is for reading or writing
     * Bandwidth and latency are registered only once, the first time they are specified.
     *
     * @param name Signal's name ( must be unique )
     * @param type kind of binding, possible values are ( BIND_MODE_READ, BIND_MODE_WRITE )
     * @param bw bandwidth for this signal ( it can be left unspecified )
     * @param latency latency for this signal ( it can be left unspecified )
     *
     * @return A pointer to the Signal with name 'name', if the Signal did not exist, this methods creates one
     *         with the specified characteristics. If the signal exist previously 'bw' and  'latency' must
     *         match with previous values ( note: undefined values always match )
     */
    Signal* registerSignal( char* name, flag type, U32 bw = 0, U32 latency = 0 );

    /**
     * Obtains the signal with name 'name'
     *
     * @param name Obtains a pointer to the Signal identified with 'name'
     * @return Signal's pointer, the Signal with name 'name'
     */
    Signal* getSignal( const char* name ) const;

    /**
     * Obtains maximum capacity allowed within binder without 'growing'
     * @return maximum capacity supported before the binder grows
     */
    U32 getCapacity() const;

    /**
     * Obtains how many Signals are inside the binder
     * @return number of signal's registered in the binder
     */
    U32 getElements() const;

    /**
     * Checks if all signals registered has 1 and only 1 registration for reading
     * and another for writing and latency and bandwidth are well defined
     *
     * @return true if the checking is satisfactory, false otherwise
     */
    bool checkSignalBindings() const;


    /**
     *  Start signal tracing.
     *
     *  Initializes the signal tracing and creates the signal trace file.
     *  @param ProfilingFile A pointer to the trace file output stream object.
     */
    void initSignalTrace(std::ostream *ProfilingFile);

    /**
     *  End signal tracing.
     *  Finishes the signal tracing and closes the signal trace file.
     */
    void endSignalTrace();

    /**
     *  Dumps the signal trace for a simulation cycle.
     *  @param cycle The simulation cycle for which to dump the
     *  signal trace.
     */
    void dumpSignalTrace(U64 cycle);
    // Debug purpose only
    void dump(bool showOnlyNotBoundSignals = false) const;
};

} // namespace arch

#endif
