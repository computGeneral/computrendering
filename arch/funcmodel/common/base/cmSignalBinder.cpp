/**************************************************************************
 *
 * Signal Binder class implementation file.
 *
 */


#include "cmSignalBinder.h"
#include <sstream>
#include <iostream>

using namespace std;
using namespace cg1gpu;

cmoSignalBinder cmoSignalBinder::binder;

const flag cmoSignalBinder::BIND_MODE_READ = 0;
const flag cmoSignalBinder::BIND_MODE_WRITE = 1;
const flag cmoSignalBinder::BIND_MODE_RW = 2;


// Private ( only called once, for static initialization )
cmoSignalBinder::cmoSignalBinder( U32 capacity ) : elements(0) , capacity(capacity)
{
    signals = new Signal*[capacity];
    bindingState = new flag[capacity];
}

cmoSignalBinder& cmoSignalBinder::getBinder() { return binder; }


Signal* cmoSignalBinder::registerSignal( char* name, flag type, U32 bw, U32 latency )
{
    Signal **signalAux;
    flag *bindingAux;
    S32 pos;
    U32 i;
    char buff[256];

    GPU_DEBUG(
        printf("cmoSignalBinder => Registering signal %s Bw %d Lat %d as %s\n",
            name, bw, latency, (type == BIND_MODE_READ)?"input":"output");
    )

    //  Search the signal in the register.  
    pos = find( name );

    //  Check if it is a new signal.  
    if ( pos < 0 )
    {
        //  Register the new signal.  

        //  Check if there is enough space in the register.  
        if ( elements >= capacity )
        {
            //  Increase the signal register.  

            GPU_DEBUG(
                printf("cmoSignalBinder:registerSignal=> Signal Binder grows...\n");
            )

            //  Create the new larger signal buffer.  
            signalAux = new Signal*[capacity + growth];

            //  Create the new  larger signal binding state buffer.  
            bindingAux = new flag[capacity + growth];

            //  Copy the old buffers to the new one.  
            for (i = 0; i < capacity; i++ )
            {
                //  Copy pointer to signals.  
                signalAux[i] = signals[i];

                //  Copy the signal binding state.  
                bindingAux[i] = bindingState[i];
            }

            //  Update signal register capacity counter.  
            capacity = capacity + growth;

            //  Delete the old signal buffer.  
            delete[] signals; // destroy old array space

            //  Delete the old signal binding state buffer.  
            delete[] bindingState;

            //  Set new buffer as signal buffer.  
            signals = signalAux;

            //  Set new buffer as binding buffer.  
            bindingState = bindingAux;
        }

        //  Add new signal to the signal buffer.  
        signals[elements] = new Signal( name, bw, latency );

        //  Set signal binding mode.  
        if ( type == BIND_MODE_READ )
            bindingState[elements] = BIND_MODE_READ;
        else
            bindingState[elements] = BIND_MODE_WRITE;

        return signals[elements++];
    }
    // signal "fully binded" implies no more registrations allowed for this signal
    if ( bindingState[pos] == BIND_MODE_RW ) 
    {
        stringstream ss;
        ss << "No more registration allowed for signal: '" << name << "'";
        CG_ASSERT(ss.str().c_str());
    }

    // At most one read or write registration allowed
    if ( type == bindingState[pos] ) {

        if ( type == BIND_MODE_READ )
            CG_ASSERT("Not allowed two registrations for reading in same signal.");
        else
            CG_ASSERT("Not allowed two registrations for writing in same signal.");
    }

    // Test signal bandwidth integrity
    if ( !signals[pos]->isBandwidthDefined() ) {
        if ( bw <= 0 ) {
            CG_ASSERT("Bandwidth must be defined.");
        }
        else // first definition for bandwidth
            signals[pos]->setBandwidth( bw );
    }
    else { // bandwidth was defined previously ( new definition must be equal )
        if ( signals[pos]->getBandwidth() != bw && bw != 0 ) {
            sprintf(buff, "No matching between actual and previous bandwidth.  Signal: %s", name);
            CG_ASSERT(buff);
        }
    }


    // Test signal latency integrity
    if ( !signals[pos]->isLatencyDefined() ) {
        if ( latency <= 0 ) {
            CG_ASSERT("Latency must be defined.");
        }
        else // definition for latency
            signals[pos]->setLatency( latency );
    }
    else { // latency was defined previously ( new definition must be equal or 0)
        if ( signals[pos]->getLatency() != latency && latency != 0 ) {
            sprintf(buff, "No matching between actual and previous latency. Signal: %s", name);
            CG_ASSERT(buff);
        }
    }

    bindingState[pos] = BIND_MODE_RW;

    return signals[pos];
}

// inline
U32 cmoSignalBinder::getCapacity() const
{
    return capacity;
}

// inline
U32 cmoSignalBinder::getElements() const
{
    return elements;
}

Signal* cmoSignalBinder::getSignal( const char* name ) const
{
    S32 pos = find( name );
    return ( pos < 0 ? 0 : signals[pos] );
}

bool cmoSignalBinder::checkSignalBindings() const
{
    for ( U32 i = 0; i < elements; i++ ) {
        if ( bindingState[i] != BIND_MODE_RW )
            return false;
    }
    return true;
}

void cmoSignalBinder::dump(bool showOnlyNotBoundSignals) const
{
    cout << "Capacity: " << capacity << endl;
    cout << "Elements: " << elements << endl;
    cout << "INFO: " << endl;
    if ( elements == 0 )
        cout << "NO registered signals yet" << endl;
    for ( U32 i = 0; i < elements; i++ ) {
        // Print actual signal info
        if ( bindingState[i] != BIND_MODE_RW || !showOnlyNotBoundSignals )
            cout << signals[i]->getName();
        switch ( bindingState[i] ) {
            case BIND_MODE_READ:
                cout << "   State: R binding";
                break;
            case BIND_MODE_WRITE:
                cout << "   State: W binding";
                break;
            case BIND_MODE_RW:
                if ( !showOnlyNotBoundSignals )
                    cout << "   State: RW binding";
                break;
        }
        if ( bindingState[i] != BIND_MODE_RW || !showOnlyNotBoundSignals ) {
            cout << "   isDefined?: " << ( signals[i]->isSignalDefined() ? "DEFINED" : "NOT DEFINED" );
            cout << "   BW: " << signals[i]->getBandwidth();
            cout << "   LAT: " << signals[i]->getLatency() << endl;
        }
    }
    cout << "-------------------" << endl;
}

// Private method ( auxiliar )
S32 cmoSignalBinder::find( const char* name ) const
{
    for ( U32 i = 0; i < elements; i++ ) {
        if ( strcmp( name, signals[i]->getName() ) == 0 )
            return i;
    }
    return -1;
}

//  Start the signal trace.  
void cmoSignalBinder::initSignalTrace(ostream *trFile)
{
    U32 i;
    char lineBuffer[1024];
    
    //  Set the tracefile
    ProfilingFile = trFile;
    
    //  Dump signal names.  
    (*ProfilingFile) << "Signal Trace File v. 1.0" << endl << endl;
    
    (*ProfilingFile) << "Signal Name\t\t\tSignal ID.\tBandwidth\tLatency" << endl << endl;
    
    for(i = 0; i < elements; i++)
    {
        sprintf(lineBuffer,"%s\t\t\t%d\t\t%d\t\t%d\n", signals[i]->getName(), i,
            signals[i]->getBandwidth(), signals[i]->getLatency());
        (*ProfilingFile) << lineBuffer;
    }

    (*ProfilingFile) << endl << endl;
    
//    printf("\n\n");
}

//  End the signal trace.  
void cmoSignalBinder::endSignalTrace()
{
    //  Check if trace file was open.  
    CG_ASSERT_COND(!(ProfilingFile == NULL), "Signal trace file was not open.");
    //fprintf(ProfilingFile,"\n\nEnd of Trace\n");
    (*ProfilingFile) << endl << endl << "End of Trace" << endl;

    //  Close signal trace file.  
    //fclose(ProfilingFile);
    //ProfilingFile->close();
}

//  Dump the signal trace for a given cycle.  
void cmoSignalBinder::dumpSignalTrace(U64 cycle)
{
    U32 i;
    char bufferLine[1024];

    //  Dump the current cycle.  
    //fprintf(ProfilingFile,"C %ld\n", cycle);
    sprintf(bufferLine, "C %ld\n", cycle);
    (*ProfilingFile) << bufferLine;

    /*  Dump all what is stored in all the signals for the
        current cycle.  */
    for (i = 0; i < elements; i++)
    {
        //  Dump the signal number/id.  
        //fprintf(ProfilingFile, "S %d:\n", i);    
        sprintf(bufferLine, "S %d:\n", i);    
        (*ProfilingFile) << bufferLine;
        
        //  Dump the objects in the signal for that cycle.  
        signals[i]->traceSignal(ProfilingFile, cycle);
    }
}
