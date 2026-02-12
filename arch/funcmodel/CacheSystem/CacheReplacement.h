/**************************************************************************
 *
 * Cache replacement policy class definition file.
 *
 */

/**
 *
 * @file CacheReplacement.h
 *
 * Defines the Cache Replace Policy classes.  This classes define how a line
 * can be selected for replacing in a cache.
 *
 */


#include "support.h"
#include "GPUType.h"

#ifndef _CACHEREPLACEMENT_

#define _CACHEREPLACEMENT_

namespace arch
{

/**
 *
 *  Defines the replacement policies for the cache.
 *
 */

enum CacheReplacePolicy
{
    CACHE_DIRECT,       //  Direct mapping.  
    CACHE_FIFO,         //  FIFO replacement policy.  
    CACHE_LRU,          //  LRU replacement policy.  
    CACHE_PSEUDOLRU,    //  Pseudo LRU replacement policy.  
    CACHE_NONE          //  No predefined replacement policy.  
};



/**
 *
 *  This abstract class defines classes that implement
 *  replacement policies for a cache.
 *
 *
 */

class CacheReplacementPolicy
{
protected:

    U32 numBias;     //  Number of bias/ways in the cache.  
    U32 numLines;    //  Number of lines per bia/way in the cache.  

public:

    /**
     *
     *  Constructor.
     *
     *  @param numBias Number of bias in the cache.
     *  @param numLines Number of lines per bia/way in the cache.
     *
     *  @return An initialized Cache Replacement Policy object.
     *
     */

    CacheReplacementPolicy(U32 numBias, U32 numLines);

    /**
     *
     *  Virtual function.
     *
     *  Updates the replacement policy state with a new
     *  access to a line and bia.
     *
     *  @param bia The bia that was accessed.
     *  @param line The line in the bia that was accessed.
     *
     */

    virtual void access(U32 bia, U32 line) = 0;

    /**
     *
     *  Virtual function.
     *
     *  Updates the replacement policy state and returns
     *  the next victim for a line.
     *
     *  @param line The line index for which to select a
     *  victim
     *
     *  @return The bia for the line index to be replaced.
     *
     */

    virtual U32 victim(U32 line) = 0;

};

//  Replacement policy classes.  

/**
 *
 *  This class implements a LRU replacement policy for a
 *  cache.
 *
 */

class LRUPolicy : public CacheReplacementPolicy
{
private:

    U32 **accessOrder;    //  Stores the bias for a line index sorted by last access.  

public:

    /**
     *
     *  Creates a LRU policy object.
     *
     *  @param numBias Number of bias in the cache.
     *  @param numLines Number of lines per bia/way in the cache.
     *
     *  @return An initialized LRU Policy object.
     *
     */

    LRUPolicy(U32 numBias, U32 numLines);

    /**
     *
     *  Updates the replacement policy state with a new
     *  access to a line and bia.
     *
     *  @param bia The bia that was accessed.
     *  @param line The line in the bia that was accessed.
     *
     */

    void access(U32 bia, U32 line);

    /**
     *
     *  Updates the replacement policy state and returns
     *  the next victim for a line.
     *
     *  @param line The line index for which to select a
     *  victim
     *
     *  @return The bia for the line index to be replaced.
     *
     */

    U32 victim(U32 line);
};

/**
 *
 *  This class implements a Pseudo LRU replacement policy for a
 *  cache.
 *
 */

class PseudoLRUPolicy : public CacheReplacementPolicy
{
private:

    U32 *lineState;      //  Current state per line index.  
    U32 biaShift;        //  Shift for the bias, or number of bits that define a bia.  

public:

    /**
     *
     *  Creates a Pseudo LRU policy object.
     *
     *  @param numBias Number of bias in the cache.
     *  @param numLines Number of lines per bia/way in the cache.
     *
     *  @return An initialized LRU Policy object.
     *
     */

    PseudoLRUPolicy(U32 numBias, U32 numLines);

    /**
     *
     *  Updates the replacement policy state with a new
     *  access to a line and bia.
     *
     *  @param bia The bia that was accessed.
     *  @param line The line in the bia that was accessed.
     *
     */

    void access(U32 bia, U32 line);

    /**
     *
     *
     *  Updates the replacement policy state and returns
     *  the next victim for a line.
     *
     *  @param line The line index for which to select a
     *  victim
     *
     *  @return The bia for the line index to be replaced.
     *
     */

    U32 victim(U32 line);
};

/**
 *
 *  This class implements a FIFO replacement policy for a
 *  cache.
 *
 */

class FIFOPolicy : public CacheReplacementPolicy
{
private:

    U32 *next;       //  Next victim per line index.  

public:

    /**
     *
     *  Creates a FIFO policy object.
     *
     *  @param numBias Number of bias in the cache.
     *  @param numLines Number of lines per bia/way in the cache.
     *
     *  @return An initialized LRU Policy object.
     *
     */

    FIFOPolicy(U32 numBias, U32 numLines);

    /**
     *
     *  Updates the replacement policy state with a new
     *  access to a line and bia.
     *
     *  @param bia The bia that was accessed.
     *  @param line The line in the bia that was accessed.
     *
     */

    void access(U32 bia, U32 line);

    /**
     *
     *
     *  Updates the replacement policy state and returns
     *  the next victim for a line.
     *
     *  @param line The line index for which to select a
     *  victim
     *
     *  @return The bia for the line index to be replaced.
     *
     */

    U32 victim(U32 line);
};

} // namespace arch

#endif
