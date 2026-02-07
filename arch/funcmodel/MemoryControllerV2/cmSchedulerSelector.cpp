/**************************************************************************
 *
 */

#include "cmSchedulerSelector.h"
#include "cmFifoScheduler.h"
#include "cmRWFifoScheduler.h"
#include "cmBankQueueScheduler.h"
#include "cmGPUMemorySpecs.h"

#include <sstream>
#include <iostream>

using std::cout;
using std::stringstream;

using namespace cg1gpu;
using namespace cg1gpu::memorycontroller;

typedef MemoryControllerParameters MCParams;

ChannelScheduler* cg1gpu::memorycontroller::createChannelScheduler(const char* name, 
                                         const char* prefix, 
                                         const MemoryControllerParameters& params,
                                         cmoMduBase* parent)
{

    ChannelScheduler::MemoryClientsInfo mci;

    mci.numStampUnits = params.numStampUnits;
    mci.numTextureUnits = params.numTexUnits;
    mci.streamerLoaderUnits = params.streamerLoaderUnits;

    // Hardcoded param for now
    bool enableProtocolMonitoring = true;

    ChannelScheduler* sched = 0;

    // Select page policy
    FifoSchedulerBase::PagePolicy policy;
    if ( params.schedulerPagePolicy == MCParams::ClosePage )
        policy = FifoSchedulerBase::ClosePage;
    else if ( params.schedulerPagePolicy == MCParams::OpenPage )
        policy = FifoSchedulerBase::OpenPage;
    else
    {
        stringstream ss;
        ss << "Page policy " << (U32)params.schedulerPagePolicy << " unknown";
        CG_ASSERT(ss.str().c_str());
    }


    if ( params.memSpecs == 0 ) {
        CG_ASSERT("GPU memory specs is NULL!");
    }

    ChannelScheduler::CommonConfig commonConfig(
        mci,
        params.banksPerMemoryChannel,
        params.burstLength,
        params.burstBytesPerCycle,
        enableProtocolMonitoring,
        *params.memSpecs,
        static_cast<ChannelScheduler::SwitchModePolicy>(params.switchModePolicy),
        params.rwfifo_maxConsecutiveReads,
        params.rwfifo_maxConsecutiveWrites,
        policy,
        params.maxChannelTransactions, // CG1_ISA_OPCODE_MAX on-flight transactions per channel 
        params.dedicatedChannelReadTransactions, // Dedicated reads (0 means that the number of reads an writes are maxTrans/2)
        *params.debugString,
        params.useClassicSchedulerStates,
        params.bankfifo_activeManagerMode,
        params.bankfifo_prechargeManagerMode,
        params.bankfifo_disableActiveManager,
        params.bankfifo_disablePrechargeManager,
        params.bankfifo_managerSelectionAlgorithm,
        0, // priorize hits over BankSelectionPolicy
        *params.bankfifo_bankSelectionPolicy,
        true);


    if ( params.schedulerType == MCParams::Fifo )
    {
        CG_INFO("Creating FifoScheduler: %s::%s", prefix, name);
        sched = new FifoScheduler( name, prefix, parent, commonConfig ); //, params.fifo_maxChannelTransactions);
    }
    else if ( params.schedulerType == MCParams::RWFifo )
    {
        CG_INFO("Creating RWFifoScheduler: %s::%s", prefix, name);
        sched = new RWFifoScheduler( name, prefix, parent, commonConfig );
                                     // params.rwfifo_maxReadChannelTransactions, params.rwfifo_maxWriteChannelTransactions, 
                                     // params.rwfifo_maxConsecutiveReads, params.rwfifo_maxConsecutiveWrites
    }
    else if ( params.schedulerType == MCParams::BankQueueFifo )
    {
        CG_INFO("Creating BankQueueScheduler: %s::%s", prefix, name);
        sched = new BankQueueScheduler( name, prefix, parent, commonConfig );
                                        // params.fifo_maxChannelTransactions, 
                                        // params.rwfifo_maxConsecutiveReads, params.rwfifo_maxConsecutiveWrites );
    }
    else
        CG_ASSERT("Unknown scheduler type");

    return sched;
}

