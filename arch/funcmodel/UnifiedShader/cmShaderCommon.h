/**************************************************************************
 *
 * Shader Common definitions
 *
 */

/**
 *  @file ShadeCommon.h
 *
 *  This file contains definitions that are common for all the shader implementations.
 *
 *
 */

#ifndef _SHADERCOMMON_

#define _SHADERCOMMON_

namespace cg1gpu
{
/**
 *
 *  Execution signal bandwidth (instructions that can be issued per cycle and thread).
 *
 */

static const U32 MAX_EXEC_BW = 2;

/***
 *
 *  Maximun number of dynamic instructions that can execute a shader
 *  thread.  Used to prevent infinite loops.
 *
 */
static const U32 MAXSHADERTHREADINSTRUCTIONS = 65536;

/***
 *
 *  Number of cycles to send a single shader output to the consumer unit
 *  after the shader.
 *
 */

static const F32 OUTPUT_TRANSMISSION_LATENCY = 0.5;

/***
 *
 *  Transmission latency due to the wire delay.  To be added
 *  to the calculated transmission latency for the number of
 *
 */

static const U32 OUTPUT_DELAY_LATENCY = 3;

/**
 *  Defines the cycle threshold after the last received shader input from which
 *  a shader input batch becomes closed.
 *
 */

static const U32 BATCH_CYCLE_THRESHOLD = 64;

/**
 *  Execution signal maximum latency (the longer it takes an instruction
 *  to execute).
 *
 */

static const U32 MAX_EXEC_LAT = 17;

/**
 *
 *  Number registers in the address register bank.
 *
 */

static const U32 MAX_ADDR_BANK_REGS = 2;

/**
 *
 *  Number of registers in the temporal register bank.
 *
 */

static const U32 MAX_TEMP_BANK_REGS = 32;

/**
 *
 *  Number of registers in the output register bank.
 *
 */

static const U32 MAX_OUTP_BANK_REGS = 16;

/**
 *
 *  Number of register in the predicate register bank.
 *
 */
static const U32 MAX_PRED_BANK_REGS = 32;


}   // namespace cg1gpu

#endif
