/**************************************************************************
 *
 * cmoMduBase class implementation file.
 *
 */

#include "cmMduBase.h"
#include "cmStatisticsManager.h"

#include <iostream>

// definition for cmsMduList

using namespace arch::gpuStatistics;
using namespace arch;
using namespace std;

bool cmoMduBase::signalTracingFlag = false;
U64  cmoMduBase::startCycle = 0;
U64  cmoMduBase::dumpCycles = 0;

void cmoMduBase::setSignalTracing(bool flag, U64 startCycle_, U64 dumpCycles_)
{
    signalTracingFlag = flag;
    startCycle = startCycle_;
    dumpCycles = dumpCycles_;
}

bool cmoMduBase::isSignalTracingRequired(U64 cycle)
{
    if ( signalTracingFlag )
        return (startCycle <= cycle) && (cycle <= startCycle + dumpCycles - 1);
    return false;
}

StatisticsManager& cmoMduBase::getSM()
{
    return sManager;
}

// Static list of boxes
cmoMduBase::cmsMduList* cmoMduBase::mduList = 0; // redundant, done by compiler



StatisticsManager& cmoMduBase::sManager = StatisticsManager::instance();

// Performs basic initializatin for all boxes
cmoMduBase::cmoMduBase( const char* nameMdu, cmoMduBase* parentMdu ) :
parent(parentMdu), binder(cmoSignalBinder::getBinder()), debugMode(false)
{
    // register new mdu
    mduList = new cmoMduBase::cmsMduList( this, mduList );
    name = new char[strlen(nameMdu)+1];
    strcpy( name, nameMdu );
}


cmoMduBase::~cmoMduBase()
{
    // we have to remove the mdu from the mdu list
    GPU_MESSAGE(
        cout << "Trying to deleting static information of cmoMduBase with name: " <<
        name << endl << "..." << endl;
    );

    cmsMduList* actual = mduList;
    cmsMduList* prev = 0;
    while ( actual ) {
        if ( strcmp( name, actual->mdu->name ) == 0 ) {
            if ( actual == mduList && !actual->next) {// only one mdu
                delete mduList;
                mduList = 0;
            }
            else if ( actual == mduList && actual->next ) {
                mduList = actual->next; // delete cmsMduList pointed by mduList and update mduList
                delete actual; // delete cmsMduList node
            }
            else
                prev->next = actual->next;

            GPU_MESSAGE(
                cout << "static info from cmoMduBase" << name << "deleted successfuly" << endl;
            );

            break; // end while
        }
        prev = actual;
        actual = actual->next;
    }

    GPU_ERROR_CHECK(
        if ( !actual )
            cout << "Error. No static information found !!!!" << endl;
    );

    // delete dinamyc private contents ( name )
    delete[] name;
}

cmoMduBase& cmoMduBase::operator=(const cmoMduBase& in)
{
    CG_ASSERT("Assignment operator not supported for boxes.");

    return *this;
}



cmoMduBase* cmoMduBase::getMdu( const char* nameMdu )
{
    cmsMduList* actual = mduList;
    while ( actual ) {
        if ( strcmp( actual->mdu->name, nameMdu ) == 0 )
            return actual->mdu;
        actual = actual->next;
    }
    return 0;
}


Signal* cmoMduBase::newInputSignal( const char* name, U32 bw, U32 latency, const char* prefix )
{
    char fullName[255];

    //  Check if there is a prefix.  
    if (prefix == NULL)
        sprintf(fullName, "%s", name);
    else
        sprintf(fullName, "%s::%s", prefix, name );

    return ( binder.registerSignal( fullName, cmoSignalBinder::BIND_MODE_READ, bw, latency ) );
}


// Not tested
Signal* cmoMduBase::newInputSignal( const char* name, U32 bw, const char* prefix )
{
    return newInputSignal( name, bw, 0, prefix );
}


Signal* cmoMduBase::newOutputSignal( const char* name, U32 bw, U32 latency, const char* prefix )
{
    char fullName[255];
    //  Check if prefix exists.  
    if(prefix == NULL)
        sprintf(fullName, "%s", name);
    else
        sprintf( fullName, "%s::%s", prefix, name );
    return ( binder.registerSignal( fullName, cmoSignalBinder::BIND_MODE_WRITE, bw, latency ) );
}


Signal* cmoMduBase::newOutputSignal( const char* name, U32 bw, const char* prefix )
{
    return newOutputSignal( name, bw, 0, prefix );
}

// inline
const char* cmoMduBase::getName() const
{
    return name;
}


// inline
const cmoMduBase* cmoMduBase::getParent() const
{
    return parent;
}

void cmoMduBase::getState(string &stateString)
{
    stateString = "No state info available";
}

void cmoMduBase::getDebugInfo(string &debugInfo) const
{
    debugInfo = "No debug info available";
}

void cmoMduBase::setDebugMode(bool mode)
{
    debugMode = mode;
}

void cmoMduBase::getCommandList(string &commandList)
{
    commandList = "No mdu commands available";
}

void cmoMduBase::execCommand(stringstream &commandStream)
{
    string command;

    commandStream >> ws;
    commandStream >> command;

    cout << "Unsupported mdu command >> " << command;

    while (!commandStream.eof())
    {
        commandStream >> command;
        cout << " " << command;
    }

    cout << endl;
}

//  Default function to detect stalls on boxes.
void cmoMduBase::detectStall(U64 cycle, bool &active, bool &stalled)
{
    active = false;
    stalled = false;
}

void cmoMduBase::stallReport(U64 cycle, string &stallReport)
{
    stringstream report;
    
    report << name << " stall report not available" << endl;
    
    stallReport.assign(report.str());
}

void cmoMduBase::getMduNameList(string &boxList)
{
    boxList.clear();

    cmsMduList* actual = mduList;

    while ( actual )
    {
        boxList.append(actual->mdu->name);
        boxList.append("\n");
        actual = actual->next;
    }
}

void cmoMduBase::dumpMduName()
{
    cout << "Mdues registered:" << endl;

    cmsMduList* actual = mduList;

    if ( !actual )
        cout << "No Mdues registered yet" << endl;
    while ( actual ) {
        cout << "cmoMduBase name: " << actual->mdu->name << endl;
        actual = actual->next;
    }
}
