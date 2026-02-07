/**************************************************************************
 *
 */

#ifndef DDRMODULESTATE_h
    #define DDRMODULESTATE_h

#include "GPUType.h"
#include "cmGPUMemorySpecs.h"

namespace cg1gpu
{
namespace memorycontroller
{

/**
 * Class used to query the state of a DDRModule
 */
class DDRModuleState
{
public:

    typedef U32 CommandMask;

    static const CommandMask ActiveBit = 0x1;
    static const CommandMask PrechargeBit = 0x2;
    static const CommandMask ReadBit = 0x4;
    static const CommandMask WriteBit = 0x8;

    static const U32 NoActiveRow = 0xFFFFFFFF;
    static const U32 AllBanks = 0xFFFFFFFE;
    static const U64 WaitCyclesUnknown = 0xFFFFFFFF;

    enum BankStateID
    {
        BS_Idle, ///< Any page opened (after precharge is completed)
        BS_Activating, ///< Opening a page
        BS_Active, ///< A read or write operation can be issued
        BS_Reading, ///< Performing a read
        BS_Writing, ///< Performing a write
        BS_Precharging ///< Performing a precharge operation
    };

    enum CommandID
    {
        C_Active,
        C_Precharge,
        C_Read,
        C_Write,
        C_Unknown
    };


    /**
     * Return a bit mask with the accepted commands in the current cycle for a bank
     *
     * @warning When the list contains read or/and write commands this commands are accepted if
     *          the target row is the current active row
     */
    CommandMask acceptedCommands(U32 bank) const;

    /**
     * Returns the wait cycles required before sending the specified command to a bank
     *
     * @return the cycles to wait before the specified commands
     */
    U64 getWaitCycles(U32 bank, CommandID cmd) const;

    /**
     * Return the number of cycles required by a write command to be completed
     *
     * This query can be useful to known when a write that is issued now
     * will be finished and the input data can be considered free.
     */
    U32 writeBurstRequiredCycles() const;

    /**
     * Return the number of cycles requited by a read command to be completed
     *
     * This query can be useful to known when the data will be available for a 
     * read command issued now.
     */
    U32 readBurstRequiredCycles() const;
    
    /**
     * Check if the specified command can be issued to a given bank
     */ 
    bool canBeIssued(U32 bank, CommandID cmd) const;

    enum IssueConstraint
    {
        CONSTRAINT_NONE, // No constraint found for the command

        CONSTRAINT_ACT_TO_ACT, // tRRD constraint (ACT interbank dependency)
        CONSTRAINT_ACT_TO_READ, // Active to read required delay constraint
        CONSTRAINT_ACT_TO_WRITE, // Active to write required delay constraint
        CONSTRAINT_ACT_TO_PRE,  // Act to precharge delay constraint

        CONSTRAINT_READ_TO_WRITE, // tRTW constraint
        CONSTRAINT_READ_TO_PRE, // tRP constraint (Precharging before meet TRP)

        CONSTRAINT_WRITE_TO_READ, // tWTR constraint
        CONSTRAINT_WRITE_TO_PRE, // tRW constraint (Precharging before meet tRW)

        CONSTRAINT_PRE_TO_ACT, // Act sent to a bank not yet closed
        // CONSTRAINT_READ_TO_READ, // Max read bandwith exceed constraint
        // CONSTRAINT_WRITE_TO_WRITE,// Max write bandwith exceed constraint

        CONSTRAINT_DATA_BUS_CONFLICT, // The data bus being used

        /// Constraint considered errors (AKA incorrect sequence of commands independently of time)
        CONSTRAINT_NOACT_WITH_WRITE, // ERROR: Trying to write a bank without an open row
        CONSTRAINT_ACT_WITH_OPENROW, // ERROR: Act to a bank with a current already opened row
        CONSTRAINT_NOACT_WITH_READ, // ERROR: Trying to read a bank without an open row
        CONSTRAINT_AUTOP_READ, // ERROR: Autoprecharge enabled, no more reads accepted
        CONSTRAINT_AUTOP_WRITE, // ERROR: Autoprecharge enabled, no more writes accepted

        CONSTRAINT_UNKNOWN // ???
    };

    IssueConstraint getIssueConstraint(U32 bank, CommandID cmd) const;


    static std::string getIssueConstraintStr(IssueConstraint ic);

    /**
     * Returns the active row in the current cycle, if there is not a current active row it
     * returns NoActiveRow
     */
    U32 getActiveRow(U32 bank) const;

    /**
     * Returns the current burst length of the DDRModule
     */
    U32 getBurstLength() const;

    /**
     * Return the number of available banks
     */
    U32 banks() const;

    /**
     * Returns the state of the target bank in the current cycle
     */
    BankStateID getState(U32 bank) const;

    /**
     * Returns a string representation of the BankStateID
     */
    std::string getStateStr(U32 bank) const;

    static std::string getCommandIDStr(CommandID cmdid);

    /**
     * Returns the number of cycles remaining to change the current state of the current bank
     *
     * @note Returns 0 when the current state won't change passively (ex: ACTIVE state)
     */
    U32 getRemainingCyclesToChangeState(U32 bank) const;

private:
    
    U64 cycle;

    CommandID lastCommand;
    U32 nBanks;

    /******************************
     * Global timming constraints *
     ******************************/
    U32 tRRD; ///< Active bank x to active bank y delay
    
    /*******************************
     * Per bank timing constraints *
     *******************************/
    U32 tRCD; ///< Active to Read/Write delay
    U32 tWTR; ///< Write time recovery

    /**
     * Write Data cannot be driven onto the DQ bus for 2 clocks after 
     * the READ data is off the bus.
     */
    U32 tRTW;
    U32 tWR; ///< Write recovery time (penalty used with write to precharge)
    U32 tRP; ///< Rwo precharge time
    U32 CASLatency; ///< CAS latency
    U32 WriteLatency; ///< Write latency

    U32 burstLength; ///< Current burst size configuration
    ///< (burstLength/2) OR Zero if setModuleParametersWithDelayZero() is called
    U32 burstTransmissionTime;

    struct BankState
    {
        BankStateID state; ///< Current state
        U64 endCycle; ///< Next cycle that the state will change (current op will finish)
        U64 lastWriteEnd; ///< True is the last operation was a write, false otherwise
        U32 openRow; ///< Current open row
        /**
         * Perform autoprecharge as soon as possible (after or in parallel) with the 
         * current read/write command
         */
        bool autoprecharge;
    };

    BankState* bankState;

    U64 lastActiveBank;
    U64 lastActiveStart;
    U64 lastActiveEnd; // == 0 indicates that no previous active was issued

    U64 lastReadBank;
    U64 lastReadStart;
    U64 lastReadEnd;

    U64 lastWriteBank;
    U64 lastWriteStart;
    U64 lastWriteEnd;


    // Called from ctor() to configure GPU memory parameters
    void setModuleParameters(const GPUMemorySpecs& memSpecs, U32 burstLength);


    DDRModuleState(const DDRModuleState&);
    DDRModuleState& operator=(const DDRModuleState&);

    friend class ChannelScheduler;

    DDRModuleState( U32 nBanks, 
                    U32 burstLength,
                    U32 burstBytesPerCycle, // Typical DDR speed (8B/c)
                    const GPUMemorySpecs& memSpecs );

    /**
     * Performs the timing update required by this class
     * ONLY CALLED BY CHANNELSCHEDULER BASE CLASS
     *
     * It must be called before performing any query or update of this class each time 
     * a new cycle of simulation is started
     */
    void updateState(U64 cycle);
    // update methods (ONLY CALLED BY CHANNELSCHEDULER)
    void postRead(U32 bank, bool autoprecharge = false);
    void postWrite(U32 bank, bool autoprecharge = false);
    void postActive(U32 bank, U32 row);
    void postPrecharge(U32 bank);
    void postPrechargeAll();


    // called by updateState...
    void updateBankState(U64 cycle, U32 bankId);

    bool isAnyBank(BankStateID state) const;


};
} // namespace memorycontroller
} // namespace cg1gpu

#endif // DDRMODULESTATE_h
