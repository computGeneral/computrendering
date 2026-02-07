/**************************************************************************
 *
 */

#ifndef SWITCHOPERATIONMODE_H
    #define SWITCHOPERATIONMODE_H

#include "GPUType.h"
#include "cmChannelScheduler.h"

namespace cg1gpu 
{
namespace memorycontroller 
{

class SwitchOperationModeBase
{
public:

    virtual bool reading() const = 0;
    
    bool writing() const { return !reading(); }

    virtual void update(bool reads, bool writes, bool readIsHit, bool writeIsHit) = 0;

    virtual U32 moreConsecutiveOpsAllowed() const = 0;

    virtual U32 MaxConsecutiveReads() const = 0;
    
    virtual U32 MaxConsecutiveWrites() const = 0;
};


class SwitchOperationModeSelector
{
public:

    static SwitchOperationModeBase* create(const ChannelScheduler::CommonConfig& config);
};


class SwitchModeTwoCounters : public SwitchOperationModeBase
{

private:

    friend class SwitchOperationModeSelector;

    SwitchModeTwoCounters(U32 maxConsecutiveReads, U32 maxConsecutiveWrites);

public:

    // Called before the selection between a read and a write.
    // After calling this method call reading() to decide if a read or write should be selected.
    void update(bool reads, bool writes, bool readIsHit, bool writeIsHit);

    bool reading() const;
    
    U32 moreConsecutiveOpsAllowed() const;

    U32 MaxConsecutiveReads() const;
    
    U32 MaxConsecutiveWrites() const;

private:

    const U32 _MaxConsecutiveReads;
    const U32 _MaxConsecutiveWrites;
    U32 _consecutiveOps;
    bool _reading;

};

class SwitchModeLoadOverStores : public SwitchOperationModeBase
{
private:

    friend class SwitchOperationModeSelector;

    SwitchModeLoadOverStores();

public:

    // Called before the selection between a read and a write.
    // After calling this method call reading() to decide if a read or write should be selected.
    void update(bool reads, bool writes, bool readIsHit, bool writeIsHit);

    bool reading() const;
    
    U32 moreConsecutiveOpsAllowed() const;

    U32 MaxConsecutiveReads() const;
    
    U32 MaxConsecutiveWrites() const;

private:

    bool _reading;


};


class SwitchModeCounters
{
public:

    SwitchModeCounters(U32 maxConsecutiveReads, U32 maxConsecutiveWrites);

    bool reading();
    bool writing();
    void switchMode();
    void newOp();
    U32 moreConsecutiveOpsAllowed();

    U32 MaxConsecutiveReads() const;
    U32 MaxConsecutiveWrites() const;

private:

    const U32 _MaxConsecutiveReads;
    const U32 _MaxConsecutiveWrites;
    U32 _consecutiveOps;
    bool _writing;

};


} // namespace memorycontroller
} // namespace cg1gpu



#endif // SWITCHOPERATIONMODE_H
