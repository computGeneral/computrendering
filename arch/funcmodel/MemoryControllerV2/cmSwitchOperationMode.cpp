/**************************************************************************
 *
 */

#include "cmSwitchOperationMode.h"

using namespace arch::memorycontroller;

SwitchOperationModeBase* SwitchOperationModeSelector::create(const ChannelScheduler::CommonConfig& config)
{
    // smpCounters = new SwitchModeCounters(maxConsecutiveReads_, maxConsecutiveWrites_);
    SwitchOperationModeBase* som = 0;
    switch ( config.switchModePolicy ) {
        case ChannelScheduler::SMP_Counters:
            som = new SwitchModeTwoCounters(config.smpMaxConsecutiveReads, config.smpMaxConsecutiveWrites);
            break;
        case ChannelScheduler::SMP_LoadsOverStores:
            som = new SwitchModeLoadOverStores();
            break;
        default:
            CG_ASSERT("SWITCH_MODE_POLICY not implemented/supported !");
    }
    return som;
}


////////////////////////////////////////////////////
/// SwitchModeTwoCounters implementation methods ///
////////////////////////////////////////////////////
SwitchModeTwoCounters::SwitchModeTwoCounters(U32 maxConsecutiveReads, U32 maxConsecutiveWrites) :
_MaxConsecutiveReads(maxConsecutiveReads), _MaxConsecutiveWrites(maxConsecutiveWrites),
_reading(true),_consecutiveOps(0)
{}

bool SwitchModeTwoCounters::reading() const
{
    return _reading;
}
    
void SwitchModeTwoCounters::update(bool readsExist, bool writesExist, bool, bool) // hit information not required
{
    if ( !readsExist && !writesExist )
        return ; // ignore this update

    if ( _reading ) {
        if ( !readsExist || (_consecutiveOps >= _MaxConsecutiveReads && writesExist ) ) {
            _consecutiveOps = 0;
            _reading = false;
        }
    }
    else { // writing
        if ( !writesExist || (_consecutiveOps >= _MaxConsecutiveWrites && readsExist) ) {
            _consecutiveOps = 0;
            _reading = true;
        }
    }

    // If we do a next operation of the same type when the maximum has been reached, operation counter is reset
    if ( !_reading && _consecutiveOps == _MaxConsecutiveWrites )
        _consecutiveOps = 0;
    else if ( _reading && _consecutiveOps == _MaxConsecutiveReads )
        _consecutiveOps = 0;

    ++_consecutiveOps;
}

U32 SwitchModeTwoCounters::moreConsecutiveOpsAllowed() const
{
    if ( _reading )
        return _MaxConsecutiveReads - _consecutiveOps;
    else
        return _MaxConsecutiveWrites - _consecutiveOps;
}

U32 SwitchModeTwoCounters::MaxConsecutiveReads() const
{
    return _MaxConsecutiveReads;
}
    
U32 SwitchModeTwoCounters::MaxConsecutiveWrites() const
{
    return _MaxConsecutiveWrites;
}



///////////////////////////////////////////////////////
/// SwitchModeLoadOverStores implementation methods ///
///////////////////////////////////////////////////////


SwitchModeLoadOverStores::SwitchModeLoadOverStores() : _reading(true)
{}


void SwitchModeLoadOverStores::update(bool readsExist, bool writesExist, bool readIsHit, bool writeIsHit)
{
    GPU_ASSERT(
        if ( !readsExist && readIsHit )
            CG_ASSERT("Read does not exist and read hit is set to TRUE!");
        if ( !writesExist && writeIsHit )
            CG_ASSERT("Read does not exist and read hit is set to TRUE!");
    )

    if ( _reading ) {
        if ( !readsExist && writesExist )
            _reading = false;
    }
    else { // if write exists and write is a hit, keep writing! (but as soon as write is not a hit, go to read !
        if ( readsExist && !writeIsHit )
            _reading = true;
    }
}

bool SwitchModeLoadOverStores::reading() const
{
    return _reading;
}
    
U32 SwitchModeLoadOverStores::moreConsecutiveOpsAllowed() const
{
    return static_cast<U32>(-1);
}

U32 SwitchModeLoadOverStores::MaxConsecutiveReads() const
{
    return static_cast<U32>(-1);
}
    
U32 SwitchModeLoadOverStores::MaxConsecutiveWrites() const
{
    return static_cast<U32>(-1);
}




/// OLD CODE ///

SwitchModeCounters::SwitchModeCounters(U32 maxConsecutiveReads, U32 maxConsecutiveWrites) :
_MaxConsecutiveReads(maxConsecutiveReads), _MaxConsecutiveWrites(maxConsecutiveWrites),
_consecutiveOps(0), _writing(false)
{}

bool SwitchModeCounters::reading()
{
    return !_writing;
}

bool SwitchModeCounters::writing()
{
    return _writing;
}

void SwitchModeCounters::switchMode()
{
    _writing = !_writing;
    _consecutiveOps = 0;
}

void SwitchModeCounters::newOp()
{
    // If we do a next operation of the same type when the maximum has been reached, operation counter is reset
    if ( _writing && _consecutiveOps == _MaxConsecutiveWrites )
        _consecutiveOps = 0;
    else if ( !_writing && _consecutiveOps == _MaxConsecutiveReads )
        _consecutiveOps = 0;

    ++_consecutiveOps;
}

U32 SwitchModeCounters::moreConsecutiveOpsAllowed()
{
    if ( _writing )
        return _MaxConsecutiveWrites - _consecutiveOps;
    else
        return _MaxConsecutiveReads - _consecutiveOps;
}

U32 SwitchModeCounters::MaxConsecutiveReads() const { return _MaxConsecutiveReads; }

U32 SwitchModeCounters::MaxConsecutiveWrites() const { return _MaxConsecutiveWrites; }

