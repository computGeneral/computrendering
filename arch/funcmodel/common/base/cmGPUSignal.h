/**************************************************************************
 *
 * Signal class definition file.
 *
 */

#ifndef __SIGNAL__
#define __SIGNAL__

#include "GPUType.h"
#include "Vec4FP32.h"
#include "DynamicObject.h"
#include <cstring>
#include <ostream>
#include <cstdio>

namespace arch
{

/**
 * @b Signal class implements the Signal concept
 *
 * @b Files:  Signal.h, Signal.cpp
 *
 * - Implements the Signals concept. Only pointer are used ( fast )
 * - Limitations:
 *    - Latency 0 in writes NOT SUPPORTED
 *
 * - About implementation:
 *    - FIFO - Circular queue. Support arbitrary ordering in
 *      the same cycle for read and writes.
 *      - read(CYCLE,data1), write(CYCLE,data2)
 *      - write(CYCLE,data2),read(CYCLE,data1)
 *      - Both sequences has exact behaviour
 *    - Supports variable latency ( latency must be equal or less than maxLatency )
 *    - Latency 0 unsupported
 *    - Supports pre-initialization ( since version 1.1 )
 *
 * @version 1.1
 * @date 07/02/2003 ( previous 28/11/2002 )
 * @author Carlos Gonzalez Rodriguez - cgonzale@ac.upc.es
 *
 */
class Signal {


    static const U32 CAPACITY = 512;
    static const U32 CAPACITY_MASK = CAPACITY - 1;

private:

    char*   name; // Name identifier for the signal
    U32     bandwidth;  // Bandwidth allowed ( writes per cycle ), 0 implies undefined
    U32     maxLatency;   // Signal's latency ( 0 implies undefined )

    U32     capacity;        // Precalculated: 2^(ceil(log2(maxLatency + 1)))
    U32     capacityMask;    // Bitmask used for the capacity module.

    DynamicObject*** data; // Matrix data

    U32     nWrites;    // Number of writes in actual cycle
    U32*    nReads;     // Reads that must be done in every cycle to avoid data losses
    U32     readsDone;  // reads in actual cycle

    U64     lastCycle;  // Last cycle with a read or write operation
    U64     lastRead;   // Last cycle with a read operation
    U64     lastWrite;  // Last cycle with a write operation

    U32     nextRead;
    U32     nextWrite;
    U32     pendentReads;

    /**
     * Position in the matrix where we are going to write or read.
     *
     * Calculated in every read/write operation
     */
    U32 in;

    std::string strFormatObject(const DynamicObject* dynObj);

    /**
     * Check data losses in signal ( only used if GPU_ERROR_CHECK is enabled )
     *
     * @param cycle current cycle
     * @param index1 first position with valid data
     * @param index2 last position with valid data
     *
     * @return true if there are not data loss, false otherwise
     */
    bool checkReadsMissed( U64 cycle, U32 index1, U32 index2 );


    /**
     * Called for constructor and setParameters to create necesary dynamic structures
     *
     * Dynamic memory allocation for atribute 'name' is not responsibility of this method
     * NO initialization performed for not-dynamic structures and atributes
     * @warning maxLatency and bandwidth must be defined before calling this method
     */
    void create();

    /**
     * Called by destructor and setParameters for release dynamic memory
     *
     * Dynamic memory deallocation for atribute 'name' is not responsibility of this method
     */
    void destroy();

    /**
     * Generic write
     *
     * Implements all the work, specific write is only an interface to avoid
     * explicit casting
     */
    // bool writeGen( U64 cycle, char* dataW, U32 latency );
    bool writeGen( U64 cycle, DynamicObject* dataW, U32 latency );

    /**
     * Generic write using maximum latency allowed
     *
     * Implements all the work, specific write is only an interface to avoid
     * explicit casting
     */
    //bool writeGen( U64 cycle, char* dataW );
    bool writeGen( U64 cycle, DynamicObject* dataW );

    /**
     * Generic read
     *
     * Implements all the work, specific read is only an interface to avoid
     * explicit casting
     */
    // bool readGen( U64 cycle, char* &dataR );
    bool readGen( U64 cycle, DynamicObject* &dataR );

    /**
     * Generic write
     *
     * Implements all the work, specific write is only an interface to avoid
     * explicit casting
     */
    // bool writeGenFast( U64 cycle, char* dataW, U32 latency );
    bool writeGenFast( U64 cycle, DynamicObject* dataW, U32 latency );

    /**
     * Generic write using maximum latency allowed
     *
     * Implements all the work, specific write is only an interface to avoid
     * explicit casting
     */
    // bool writeGenFast( U64 cycle, char* dataW );
    bool writeGenFast( U64 cycle, DynamicObject* dataW );

    /**
     * Generic read
     *
     * Implements all the work, specific read is only an interface to avoid
     * explicit casting
     */
    // bool readGenFast( U64 cycle, char* &dataR );
    bool readGenFast( U64 cycle, DynamicObject* &dataR );

    /**
     *
     *  Updates the signal clock.
     *
     *  @param cycle The next signal cycle.
     *
     */

    void clock(U64 cycle);
    void clockFast(U64 cycle);

public:


    /**
     * Creates a new Signal
     *
     * Creates a new Signal. If bandwidth or latency are not specified creates an
     * undefined signal
     *
     * @param name signal's name
     * @param bandwidth maximum signal's bandwidth supported
     * @param maxLatency maximum signal's latency
     */
    Signal( const char* name, U32 bandwidth = 0, U32 maxLatency = 0 );

    /// Destructor
    ~Signal();


    /**
     * Sets a default contents ( it only should be used at the beginning, before reading or writing in the Signal )
     *
     * @param initialData array of data used to fill the signal
     * @param firstCycleForReadOrWrite first cycle with data available ( defaults to 0 )
     *
     * @note setData is typically used without specifying firstCycleForReadOrWrite, 0 it is assumed.
     *
     * @code
     *    // example of use of setData(), general use ( specifying a firstCycleForReadOrWrite greater than 0 )
     *
     *    // initialContents + firstCycle represents this logical initialization for the signal:
     *    // <-----> maxLatency ( 3 )
     *    // |5|3|1|  |
     *    // |6|4|2|  | bandwidth ( 2 )
     *    //
     *    //  data 1 and 2 must be read in cycle 1 ( assumption: data written in cycle -1 in this case )
     *    //  data 3 and 4 must be read in cycle 2 ( assumption: data written in cycle 0 in this case )
     *    //  data 5 and 6 must be read in cycle 3 ( assumption: data written in cycle 1 in this case )
     *    //  The assumption of when data was written ( with maxLatency ) is deduced like this :
     *    //     ( firstCycleForReadOrWrite - maxLatency + 1 )
     *    //  Note that is possible "the assumption" of negative cycles, but this is supported.
     *    //  User of this method must not worry about that.
     *
     *    // Asumptions based on initial cycle specified ( 1 in this case )
     *    // 1,2 -> data available in cycle 1
     *    // 3,4 -> data available in cycle 2
     *    // 5,6 -> data available in cycle 3
     *    void* initialContents[6] = { (void*)1, (void*)2, (void*)3, (void*)4, (void*)5 ,(void*)6 };
     *
     *    Signal s( "fooSignal", 2, 3 ); // Signal with bandwith 2 and maxLatency 3
     *
     *    s.setData( initialContents, 1 ); // cycle 1 with data available ( 1 and 2 )
     *
     *    // now when can read from the signal
     *    s.read( 1, myData ); // OK !
     *
     * @endcode
     */
    bool setData( DynamicObject* initialData[], U64 firstCycleForReadOrWrite = 0 );


    /**
     * Write interface for void pointers ( using max latency allowed )
     *
     * @param cycle cycle in which write is performed
     * @param dataW pointer to the data
     *
     * @return returns true if the write was successfully done, false otherwise
     */
    bool write( U64 cycle, DynamicObject* dataW );

    /**
     * Write interface for void pointers
     *
     * @param cycle cycle in which write is performed
     * @param dataW pointer to the data
     * @param latency Latency for the data to be written
     *
     * @return returns true if the write was successfully done, false otherwise
     */
    bool write( U64 cycle, DynamicObject* dataW, U32 latency );

    /**
     * Read interface for void pointers
     *
     * @param cycle cycle in which read is performed
     * @param dataR a reference to a pointer to the Quadfloat read ( not a pointer but a pointer reference )
     *
     * @return returns true if the read was successfully done, false otherwise
     */
    bool read( U64 cycle, DynamicObject* &dataR );

    /**
     * Obtains signal's name identifier
     *
     * @return signal's name
     */
    const char* getName() const;

    /**
     * Obtains max signal's bandwidth ( writes per cycle allowed )
     *
     * @return signal's bandwidth
     *
     * @note If signal's bandwidth is not yet defined returns 0
     */
    U32 getBandwidth() const;

    /**
     * Obtains max signal's latency
     *
     * @return signal's latency
     *
     * @note If signal's maximum latency is not yet defined returns 0
     */
    U32 getLatency() const;

    /**
     *
     *  Dumps to the trace file the signal trace for the cycle.
     *
     *  @param ProfilingFile A pointer to the trace file output stream object.
     *  @param cycle The simulation cycle for which to dump
     *  the trace signal.
     *
     */

    void traceSignal(std::ostream *ProfilingFile, U64 cycle);
    //void traceSignal(gzofstream *ProfilingFile, U64 cycle);

    /// For debug purpose
    void dump() const;

    /**
     * Test if signal is fully defined
     *
     * Tests if signal has bandwidth and latency defined
     *
     * @return true if signal is fully defined, false otherwise
     *
     * @note If signal is not defined, all operations except setParameters are undefined
     */
    bool isSignalDefined() const;

    /**
     * Test if bandwidth is defined
     *
     * @return true if signal's bandwidth is WELL-DEFINED
     *
     * @note BANDWIDTH is WELL-DEFINED ( bandwidth > 0 )
     */
    bool isBandwidthDefined() const;

    /**
     * Test if latency is defined
     *
     * @return true if signal's latency is WELL-DEFINED
     *
     * @note LATENCY is WELL-DEFINED ( latency > 0 )
     */
    bool isLatencyDefined() const;


   /**
     * Sets new bandwidth and latency ( hard reset )
     *
     * @return true if SIGNAL is WELL-DEFINED, false ( 0 ) otherwise
     * @note SIGNAL WELL-DEFINED : latency > 0 && bandwidth > 0
     */
    bool setParameters( U32 newBandwidth, U32 newLatency );

    /**
     * Sets new bandwidth
     *
     * Sets new signal bandwidth ( hard reset )
     *
     * @return true if SIGNAL is WELL-DEFINED, false ( 0 ) otherwise
     */
    bool setBandwidth( U32 newBandwidth );

    /**
     * Sets new signal latency
     *
     * Sets new latency's signal ( hard reset )
     *
     * @return true if signal is WELL-DEFINED, false ( 0 ) otherwise
     */
    bool setLatency( U32 newLatency );

};

}

#endif
