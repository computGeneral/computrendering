/**************************************************************************
 *
 */

#ifndef DDRMODULE_H
    #define DDRMODULE_H

#include "cmMduBase.h"
#include "cmDDRBank.h"
#include "cmDDRCommand.h"
#include <queue>
#include <vector>
#include <utility>

namespace arch
{
namespace memorycontroller
{

class GPUMemorySpecs;

/**
 * Class implementing a DDR Module chip
 *
 * This class is implemented as a cmoMduBase so the communication interface is performed
 * through two signals
 *
 * The input signals receives DDRCommand objects and the output signal sends
 * DDRBurst objects
 *
 * - prefix value concatenated with "DDRModuleRequest" is the name of the signal 
 *   that is read by the DDRModule to receive new commands (and data in write case)
 * 
 * - prefix value concatenated with "DDRModuleReply" is the name of the signal 
 *   that is written by DDRModule to send read data
 *
 * @code
 *   // Example:
 *   // Creates a DDR module chip with 8 banks
 *   DDRModule* module = new DDRModule("DDRChip0", "chip0", 8, 0);
 *   // signals after creatial of the module are:
 *   // in: chip0.DDRModuleRequest
 *   // out: chip0.DDRModuleReply
 * @endcode
 */
class DDRModule : public cmoMduBase
{
private:

    gpuStatistics::Statistic& transmissionCyclesStat; // Cycles transmiting data
    gpuStatistics::Statistic& transmissionBytesStat; // Total bytes transmited
    gpuStatistics::Statistic& readTransmissionCyclesStat; // Total read cycles
    gpuStatistics::Statistic& readTransmissionBytesStat; 
    gpuStatistics::Statistic& writeTransmissionCyclesStat;
    gpuStatistics::Statistic& writeTransmissionBytesStat;

    gpuStatistics::Statistic& actCommandsReceived;
    gpuStatistics::Statistic& allBanksPrechargedCyclesStat;

    gpuStatistics::Statistic& idleCyclesStat; // Data pins unused due to there is nothing to do

    gpuStatistics::Statistic& casCyclesStat;
    gpuStatistics::Statistic& wlCyclesStat;
    gpuStatistics::Statistic* constraintCyclesStats[DDRCommand::PC_count];

    /**
     * Retains the cycle used with the last clock method call
     */
    U64 lastClock;
    DDRCommand::DDRCmd lastCmd;
    U32 lastColumnAccessed; ///< last column accessed


    //DDRBank* banks; ///< banks of this DDR module
    std::vector<DDRBank> banks; ///< banks of this DDR module
    U32 nBanks; ///< Number of banks of this DDR Module
    U32 bankRows;    ///<  Number of rows in a bank
    U32 bankCols;    ///<  Number of columns in a row in a bank

    /**
     * Global timing constraints
     */
    U32 tRRD; ///< Active bank x to active bank y delay

    /**
     * Per bank timing constraints
     */
    U32 tRCD; ///< Active to Read/Write delay
    U32 tWTR; ///< Write to Read delay
    /**
     * Write Data cannot be driven onto the DQ bus for 2 clocks after 
     * the READ data is off the bus.
     */
    U32 tRTW;
    U32 tWR; ///< Write recovery time (penalty used with write to precharge)
    U32 tRP; ///< Rwo precharge time
    U32 CASLatency; ///< CAS latency
    U32 WriteLatency; ///< Write latency

    ///< DDR ratio 32-bit datums per cycle (tipically 2 in DDR (double data rate) memory)
    ///U32 burstElementsPerCycle; 
    U32 burstBytesPerCycle;

    U32 burstLength; ///< Current burst size configuration
    ///< (burstLength/2) OR Zero if setModuleParametersWithDelayZero() is called
    U32 burstTransmissionTime; 

    enum BankStateID
    {
        BS_Idle, ///< Any page opened (after precharge is completed)
        BS_Activating, ///< Opening a page
        BS_Active, ///< A read or write operation can be issued
        BS_Reading, ///< Performing a read
        BS_Writing, ///< Performing a write
        BS_Precharging ///< Performing a precharge operation
    };

    struct BankState
    {
        BankStateID state; ///< Current state
        U64 endCycle; ///< Next cycle that the state will change (current op will finish)
        U64 lastWriteEnd; ///< True is the last operation was a write, false otherwise
        /**
         * Perform autoprecharge as soon as possible (after or in parallel) with the 
         * current read/write command
         */
        bool autoprecharge;
    };

    BankState* bankState;

    Signal* cmdSignal; ///< Receives commands (and write data included in write commands)
    Signal* replySignal; ///< Transmits DDRBurst objects

    Signal* moduleInfoSignal; ///< Used for debug purposes
    std::string lastModuleInfoString; ///< Used to compare with previous one and set a different color when a state change has occurred

    struct DataPinItem : public DynamicObject
    {
        enum DataPinItemColor
        {
            STV_COLOR_READ  = 0,
            STV_COLOR_WRITE = 1,
            STV_COLOR_PROTOCOL_CONSTRAINT_BASE = 10, // (Base) color = BASE + getProtocolConstraint()
            STV_COLOR_PROTOCOL_CONSTRAINT_MAX  = 100,
            STV_COLOR_CAS   = 101,
            STV_COLOR_WL    = 102
        };

        enum DataPinItemType
        {
            IS_READ,
            IS_WRITE,
            IS_PROTOCOL_CONSTRAINT,
            IS_CAS_LATENCY,
            IS_WRITE_LATENCY
        };
        // bool isRead;

        DataPinItemColor _whatis ;


        DataPinItemType whattype() const
        {
            if ( _whatis == STV_COLOR_READ )
                return IS_READ;
            if ( _whatis == STV_COLOR_WRITE )
                return IS_WRITE;
            if ( STV_COLOR_PROTOCOL_CONSTRAINT_BASE <= _whatis && _whatis < STV_COLOR_PROTOCOL_CONSTRAINT_MAX )
                return IS_PROTOCOL_CONSTRAINT;
            if ( _whatis == STV_COLOR_CAS )
                return IS_CAS_LATENCY;
            if ( _whatis == STV_COLOR_WL )
                return IS_WRITE_LATENCY;
            CG_ASSERT("Unknown type!");
            return IS_PROTOCOL_CONSTRAINT; // to avoid stupid compiler warnings...
        }

        DataPinItem(DataPinItemColor wi) : _whatis(wi) {}

        std::string toString() const {
            std::string str;
            switch ( whattype() ) {
                case IS_READ:
                    str = str + "IS_READ"; break;
                case IS_WRITE:
                    str = str + "IS_WRITE"; break;
                case IS_PROTOCOL_CONSTRAINT:
                    str = str + "PROTOCOL_CONSTRAINT (";
                    {
                        str = str + DDRCommand::protocolConstraintToString(
                            DDRCommand::ProtocolConstraint(_whatis - STV_COLOR_PROTOCOL_CONSTRAINT_BASE)) +
                            ")";
                    }
                    break;
                case IS_CAS_LATENCY:
                    str = str + "IS_CAS_LATENCY"; break;
                case IS_WRITE_LATENCY:
                    str = str + "IS_WRITE_LATENCY"; break;
                default:
                    str = "UNKNOWN!";

            } 
            return str;
        }
    };

    typedef DataPinItem::DataPinItemColor DataPinItemColor;
    typedef DataPinItem::DataPinItemType DataPinItemType;


    // Used to debug DDRModule state
    struct DDRModuleInfo : public DynamicObject
    {};
    
    // inner signals to visualize data pins transmissions
    Signal* dataPinsSignal;

    DataPinItem* bypassConstraint;

    // This queue is processed each cycle writing in dataPinsSignal if the front
    // cycle of the front pair matches with the current cycle
    std::deque<std::pair<U64,DataPinItem*> > dataPinsItem;

    std::deque<std::pair<U64, DDRBurst*> > readout; ///< in progress reads
    //std::deque<U64> readin; ///< in progress writes
    std::deque<std::pair<U64, const DDRBurst*> > readin; ///< in progress writes

    U64 lastActiveBank;
    U64 lastActiveStart;
    U64 lastActiveEnd; // == 0 indicates that no previous active was issued

    U64 lastReadBank;
    U64 lastReadStart;
    U64 lastReadEnd;

    U64 lastWriteBank;
    U64 lastWriteStart;
    U64 lastWriteEnd;

    /// Cannot be 0!!! It will skip the deleting of DDR Commands
    static const U32 LIST_OF_LATESTS_COMMANDS_MAX_SIZE = 10;
    /// List to keep track of latests DDR Commands received
    std::deque<std::pair<U64,DDRCommand*> > listOfLastestProcessedCommands;

    // This attribute is updated by each write command and indicates when a new
    // read will be able to be processed
    
    void updateBankState(U64 cycle, U32 bankId);
    

    void preprocessCommand(U64 cycle, const DDRCommand* ddrCommand); // Called by processCommand()
    void processCommand(U64 cycle, DDRCommand* cmd); 
    void processActiveCommand(U64  cycle, DDRCommand* activeCommand);
    void processPrechargeCommand(U64  cycle, DDRCommand* prechargeCommand);
    void processReadCommand(U64  cycle, DDRCommand* readCommand);
    void processWriteCommand(U64  cycle, DDRCommand* writeCommand);
    void processDummyCommand(U64 cycle, DDRCommand* dummyCommand);

    bool isAnyBank(BankStateID state) const;

    bool processDataPins(U64 cycle);
    bool processDataPinsConstraints(U64 cycle);

    // info methods

    std::string getStateStr(U32 bank) const;


public:

    /**
     * Method available to implemented PRELOAD DATA into memory modules
     */
    void preload(U32 bank, U32 row, U32 startCol, 
                 const U08* data, U32 dataSz, const U32* mask = 0 );

    /**
     * Creates a DDRModule object
     *
     * @param name cmoMduBase name
     * @param prefix prefix added to the basic cmoMduBase signal names to create unique signal names
     * @param banks Number of banks within this module
     * @param parentBox cmoMduBase which is the parent of this memory module
     */
    DDRModule(const char* name, 
              const char* prefix,
              U32 burstLength,             
              U32 banks,
              U32 bankRows,
              U32 bankCols,
              //U32 burstElementsPerCycle, // 2 = Typical DDR speed
              U32 burstBytesPerCycle, // 8 = Typical DDR speed
              const GPUMemorySpecs& memSpecs,
              cmoMduBase* parentBox = 0
              );

    void setModuleParameters( const GPUMemorySpecs& memSpecs, U32 burstLength );

    /**
     * Simulates 1 cycle of simulation time of this memory module
     *
     * @param cycle cycle to be simulated
     */
    void clock(U64 cycle);

    /**
     * Dump the current operating parameters of the memory module
     */
    void printOperatingParameters() const;

    /**
     * Gets a DDRModule bank
     *
     * @param bankId bank identifier
     * @return a reference to the DDRBank identified by bankId
     */
    DDRBank& getBank(U32 bankId);

    /**
     * Returns the number of banks within this memory module
     *
     * @return number of banks within this memory module
     */
    U32 countBanks() const;

    /**
     * Dump the memory module internal state
     *
     * @param bankContens true if the data contents of the bank will be printed, 
     *                    false otherwise
     * @param txtFormat true if byte will be printed as characters, false
     *                  if byte contents will be printed as hexadecimal values
     */
    void dump(bool bankContents = false, bool txtFormat = false) const;

    /**
     * Direct access to read data from a bank and place it into an output stream (ie: ofstream)
     *
     * @note this method access data directly without changing chip state at all (any simulation is performed)
     */
    void readData(U32 bank, U32 row, U32 startCol, U32 bytes, std::ostream& outStream) const;

    /**
     * Direct access to write data into a bank reading the data from an input stream (ie: ifstream)
     *
     * @note this method access data directly without changing chip state at all (any simulation is performed)
     */
    void writeData(U32 bank, U32 row, U32 startCol, U32 bytes, std::istream& inStream);
    
};

} // namespace memorycontroller

} // namespace arch

#endif // DDRMODULE_H
