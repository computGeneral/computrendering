/**************************************************************************
 *
 * FilterOperation implementation file.
 *
 */

/**
 *
 *  @file FilterOperation.cpp
 *
 *  This file implements the FilterOperation class that carrier information
 *  about the current operation in the texture unit filter pipeline.
 *
 */

#include "cmFilterOperation.h"

using namespace arch;

//  Creates a new Filter Operation object.  
FilterOperation::FilterOperation(U32 entry) : filterEntry(entry)
{
    setTag("FiltOp");
}


//  Returns the queue entry for the filter operation.  
U32 FilterOperation::getFilterEntry()
{
    return filterEntry;
}
