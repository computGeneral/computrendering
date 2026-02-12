/**************************************************************************
 *
 * Rasterizer state definition file.
 *
 */


/**
 *
 *  @file cmRasterizerState.h
 *
 *  This file just defines the rasterizer state.
 *
 */

#ifndef _RASTERIZERSTATE_

#define _RASTERIZERSTATE_

namespace arch
{

//**  Rasterizer states.  
enum RasterizerState
{
    RAST_RESET,         //  The Rasterizer is resetting.  
    RAST_READY,         //  The Rasterizer is ready.  
    RAST_DRAWING,       //  The Rasterizer is displaying a batch of vertexes.  
    RAST_BUSY,          //  The Rasterizer is busy.  
    RAST_END,           //  The cmRasterizer.has finished displaying a batch of vertexes.  
    RAST_SWAP,          //  Fragment Operations swap buffers state.  
    RAST_DUMP_BUFFER,   //  Dump buffer (debug only!).  
    RAST_FLUSH,         //  Fragment Operations flush state. 
    RAST_SAVE_STATE,    //  Fragment Operations save block state info state.  
    RAST_RESTORE_STATE, //  Fragment Operations restore block state info state.  
    RAST_RESET_STATE,   //  Fragment Operations buffer block state reset state.  
    RAST_BLIT,          //  Fragment Operations blit state. 
    RAST_CLEAR,         //  Fragment Operations clear buffers state.  
    RAST_CLEAR_END      //  End of clear buffer state.  
};

} // namespace arch

#endif
