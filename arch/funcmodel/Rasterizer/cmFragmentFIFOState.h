/**************************************************************************
 *
 * Fragment FIFO state definition file.
 *
 */

/**
 *
 *  @file cmFragmentFIFOState.h
 *
 *  This file defines the Fragment FIFO states.
 *
 */


#ifndef _FRAGMENTFIFOSTATE_

#define _FRAGMENTFIFOSTATE_


/**
 *
 *  Defines the Fragment FIFO states for the Hierarchical/Early Z mdu.
 *
 */

enum FFIFOState
{
    FFIFO_READY,        //  The Fragment FIFO test can receive new stamps.  
    FFIFO_BUSY          //  The Fragment FIFO can not receive new stamps.  
};

#endif
