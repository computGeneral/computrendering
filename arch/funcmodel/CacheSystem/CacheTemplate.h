/**************************************************************************
 *
 * Cache class definition file.
 *
 */

/**
 *
 * @file bmoCacheTemplate.h
 *
 * Defines the Cache class.  This class defines a generic and configurable
 * memory cache model.  Version for 64 bit addresses.
 *
 */


#include "support.h"
#include "GPUType.h"
#include "CacheReplacement.h"

#include "GPUMath.h"
#include <cstdio>
#include <cstring>

#ifndef __BM_CACHEBASE_H__
#define __BM_CACHEBASE_H__


namespace arch
{

/**
 *  The Cache class defines and implements a generic and
 *  configurable cache model.  
 *  for 64 bit addr width, pass ADDRTYP with U64
 *  for 32 bit addr width, pass ADDRTYP with U32 
 *
 */
template< typename ADDRTYP>
class bmoCacheTemplate
{

private:

    //  Cache parameters.  
    CacheReplacePolicy replacePolicy;   //  Replacement policy for the cache.  

    //  Cache masks and shifts.  
    U32 byteMask;        //  Address line byte mask.  
    U32 lineMask;        //  Address line mask.  
    U32 lineShift;       //  Address line shift.  
    U32 tagShift;        //  Address tag shift.  

    //  Cache replacement mechanism.  
    CacheReplacementPolicy *policy;      //  Cache replacement policy.  

protected:

    //  Cache parameters.  
    U32 numWays;         //  Number of ways in the cache.  
    U32 numLines;        //  Number of lines in the cache (total).  
    U32 lineSize;        //  Size of a cache line in bytes.  

    //  Cache structures.  
    U08 ***cache;         //  Cache memory.  
    bool **valid;           //  Valid line bit.  
    ADDRTYP **tags;          //  Cache tag file.  


    /**
     *
     *  Search if a requested address is in the cache.
     *
     *  @param address Address to search in the cache tag file.
     *  @param line Reference to a variable where to store the
     *  cache line for the address.
     *  @param way Reference to a variable where to store the
     *  cache way for the address.
     *
     *  @return If the address was found in the cache tag file.
     *
     */
    bool search(ADDRTYP address, U32 &line, U32 &way)
    {
        U32 wayAux;
        bool found;
    
        //  Check for fully associative cache.  
        if (numLines == 1)
        {
            //  Fully associative cache, no line index used.  
            line = 0;
        }
        else
        {
            //  Get the line index from the address.  
            line = (static_cast<U32>(address) >> lineShift) & lineMask;
        }
    
        //  Search the line in the different ways for the line index.  
        for(wayAux = 0, found = FALSE; (wayAux < numWays) && !found; wayAux++)
            found = ((tags[wayAux][line] == (address >> tagShift)) && valid[wayAux][line]);
    
        //  Adjust way.  
        way = wayAux - 1;
    
        //  Check hit.  
        return found;
    }


    /**
     *
     *  Calculates the line tag for an address.
     *
     *  @param address The address for which to calcultate the tag.
     *
     *  @return The calculated tag.
     *
     */
    ADDRTYP tag(ADDRTYP address)
    {
        return (address >> tagShift);
    }

    /**
     *
     *  Retrieves the address of the first byte of the specified line.
     *
     *  @param way The way of the cache line for which to get the memory
     *  address.
     *  @param line The line of the cache line for which to get the memory
     *  address.
     *
     *  @return The memory address of the line stored in the
     *  cache line specified.
     *
     */

    ADDRTYP line2address(U32 way, U32 line)
    {
        return (((tags[way][line] << (tagShift - lineShift)) + line) << lineShift);
    }

    /**
     *
     *  Calculates the offset inside of a cache line for a given address.
     *
     *  @param address Address for which to calculate the line offset.
     *
     *  @return The calculated line offset.
     *
     */

    U32 offset(ADDRTYP address)
    {
        return (static_cast<U32>(address) & byteMask);
    }

public:

    /**
     *
     *  Cache constructor.
     *
     *  Creates and initializes a Cache object.
     *
     *  @param numWays Number of ways in the cache.
     *  @param numLines Number of lines in the cache.
     *  @param bytesLine Line size in bytes.
     *  @param replacePolicy Replacement policy for the cache.
     *
     *  @return A new initialized cache object.
     *
     */

    bmoCacheTemplate(U32 ways, U32 lines, U32 bytesLine,
        CacheReplacePolicy repPolicy) :
        numWays(ways), numLines(lines), lineSize(bytesLine),
        replacePolicy(repPolicy)
    {
        U32 i, j;
    
        //  Check number of lines.  
        CG_ASSERT_COND(!(numLines == 0), "At least a line per way is required.");    
        //  Check number of ways.  
        CG_ASSERT_COND(!(numWays == 0), "At least a way is required.");    
        //  Check line size.  
        CG_ASSERT_COND(!(lineSize == 0), "At least a byte per line is required.");    
        //  Allocate cache ways.  
        cache = new U08**[numWays];
    
        //  Check allocation.  
        CG_ASSERT_COND(!(cache == NULL), "Error allocating pointer to the ways.");    
        //  Allocate tag file for the ways.  
        tags = new ADDRTYP*[numWays];
    
        //  Check allocation.  
        CG_ASSERT_COND(!(tags == NULL), "Error allocating pointers for the ways tag files.");    
        //  Allocate the valid bit for ways.  
        valid = new bool*[numWays];
    
        CG_ASSERT_COND(!(valid == NULL), "Error allocating the pointers for the ways valid bits.");    
        //  Allocate cache memory, tags and valid bits.  
        for (i = 0; i < numWays; i++)
        {
            //  Allocate the way cache lines.  
            cache[i] = new U08*[numLines];
    
            //  Cehck allocation.  
            CG_ASSERT_COND(!(cache[i] == NULL), "Error allocating line pointers for the way.");    
            //  Allocate tag file for the way lines.  
            tags[i] = new ADDRTYP[numLines];
    
            //  Check allocation.  
            CG_ASSERT_COND(!(tags[i] == NULL), "Error allocating tag file for the lines in a way.");    
            //  Allocate the valid bits  for the way lines.  
            valid[i] = new bool[numLines];
    
            CG_ASSERT_COND(!(valid[i] == NULL), "Error allocating the valid bits for the lines in a way.");    
            //  Allocate cache lines.  
            for (j = 0; j < numLines; j++)
            {
                //  Allocate line.  
                cache[i][j] = new U08[lineSize];
    
                //  Check memory allocation.  
                CG_ASSERT_COND(!(cache[i][j] == NULL), "Error allocating cache line.");    
                //  Reset valid bit for the line.  
                valid[i][j] = FALSE;
            }
        }
    
        //  Calculate masks and shifts.  
        byteMask = GPUMath::buildMask(lineSize);
        lineMask = GPUMath::buildMask(numLines);
        lineShift = GPUMath::calculateShift(lineSize);
    
        //  Check for fully associative cache.  
        if (numLines == 1)
            tagShift = lineShift;
        else
            tagShift = GPUMath::calculateShift(numLines) + lineShift;
    
        //  Create replacemente policy object.  
        switch(repPolicy)
        {
            case CACHE_DIRECT:
                CG_ASSERT("Not Implemented.");
                break;
    
            case CACHE_LRU:
    
                //  LRU replacemente policy.  
                policy = new LRUPolicy(numWays, numLines);
                break;
    
            case CACHE_PSEUDOLRU:
    
                //  Pseudo LRU replacement policy.  
                policy = new PseudoLRUPolicy(numWays, numLines);
                break;
    
            case CACHE_FIFO:
    
                //  FIFO replacement policy.  
                policy = new FIFOPolicy(numWays, numLines);
                break;
    
            case CACHE_NONE:
    
                //  No replacement policy defined.  
                policy = NULL;
    
                break;
    
            default:
                CG_ASSERT("Unsupported cache replacemente policy.");
                break;
        }
    }

    /**
     *
     *  Reads a 32 bit word from the cache.
     *
     *  @param address Address (64 bits) of the word to be read
     *  from the cache.
     *  @param data Reference to the variable where to store the
     *  read 32 bit word.
     *
     *  @return TRUE if the address was found (hit) in the cache and
     *  data was returned, FALSE if the address was not found in the
     *  cache (miss).
     *
     */

    bool read(ADDRTYP address, U32 &data)
    {
        U32 line;
        U32 way;
    
        //  Check hit.  
        if (search(address, line, way))
        {
            //  Read the data.  
            data = *((U32 *) &cache[way][line][address & byteMask]);
    
            //  Check if replacement policy is defined.  
            if (policy != NULL)
            {
                //  Update replacemente algorithm.  
                policy->access(way, line);
            }
    
            //  Return a hit.  
            return TRUE;
        }
        else
        {
            //  Return a miss.  
            return FALSE;
        }
    }

    /**
     *
     *  Writes a 32 bit word to the cache.
     *
     *  @param address Address (64 bits) of the word to be written
     *  in the cache.
     *  @param data Reference to the variable where is the data
     *  to be written to the cache.
     *
     *  @return TRUE if the address was in the cache (hit), FALSE
     *  if the address was not in the cache (miss).
     *
     */

    bool write(ADDRTYP address, U32 &data)
    {
        U32 line;
        U32 way;
    
        //  Check hit.  
        if (search(address, line, way))
        {
            //  Write the data.  
            *((U32 *) &cache[way][line][address & byteMask]) = data;
    
            //  Check if replacement policy defined.  
            if (policy != NULL)
            {
                //  Update replacemente algorithm.  
                policy->access(way, line);
            }
    
            //  Return a hit.  
            return TRUE;
        }
        else
        {
            //  Return a miss.  
            return FALSE;
        }
    
    }


    /**
     *
     *  Selects a way for the line associated to the address
     *  as a victim for replacement.
     *
     *  @param address Address (64 bits) for which a line
     *  is needed.
     *
     *  @return The victim way for the line defined by the
     *  input address.
     *
     */

    U32 selectVictim(ADDRTYP address)
    {
        U32 line;
        U32 way;
    
        //  Check for fully associative cache.  
        if (numLines == 1)
        {
            //  Fully associative cache, no line index used.  
            line = 0;
        }
        else
        {
            //  Get the line index from the address.  
            line = (static_cast<U32>(address) >> lineShift) & lineMask;
        }
    
        //  First search the ways in the line index for an invalid line.  
        for(way = 0;(way < numWays) && valid[way][line]; way++);
    
        //  Check if an invalid line was found for the line index.  
        if (way == numWays)
        {
            //  Check if replacement policy is defined.  
            if (policy != NULL)
            {
                //  Ask the replacement protocol for a line.  
                return policy->victim(line);
            }
            else
            {
                CG_ASSERT("No replacement policy defined.");
            }
        }
        // else  // Use the invalid line.
        return way;
    
    }

    /**
     *
     *  Replaces and fills a cache line.
     *
     *  @param address Address of the new line being loaded in the
     *  cache.
     *  @param way Way for the line defined by the address in which
     *  the data is going to be stored.
     *  @param data Pointer to a byte array with the new line data.
     *
     */

    void replace(ADDRTYP address, U32 way, U08 *data)
    {
        U32 line;
    
        CG_ASSERT_COND(!(way >= numWays), "Out of range cache way number.");    
        //  Check for fully associative cache.  
        if (numLines == 0)
        {
            //  There is only a cache line per way.  
            line = 0;
        }
        else
        {
            //  Get the line in the way from the address.  
            line = (static_cast<U32>(address) >> lineShift) & lineMask;
        }
    
        //  Copy the data.  
        memcpy(cache[way][line], data, lineSize);
    
        //  Set the tag for the line.  
        tags[way][line] = (address >> tagShift);
    
        //  Set the valid bit for the line.  
        valid[way][line] = TRUE;
    
        //  Check if replacement policy is defined.  
        if (policy != NULL)
        {
            //  Update replacement algorithm.  
            policy->access(way, line);
        }
    }

    /**
     *
     *  Replaces a cache line.
     *
     *  Only updates the tag file, valid bit and replacement
     *  policy algorithm.  The cache line is not filled with
     *  the replacing line data.
     *
     *  @param address Address of the line replacing the cache
     *  line.
     *  @param way Way index for the cache line being replaced
     *  (for n-associative caches).
     *
     */

    void replace(ADDRTYP address, U32 way)
    {
        U32 line;
    
        CG_ASSERT_COND(!(way >= numWays), "Out of range cache way number.");    
        //  Check for fully associative cache.  
        if (numLines == 0)
        {
            //  There is only a cache line per way.  
            line = 0;
        }
        else
        {
            //  Get the line in the way from the address.  
            line = (static_cast<U32>(address) >> lineShift) & lineMask;
        }
    
        //  Set the tag for the line.  
        tags[way][line] = (address >> tagShift);
    
        //  Set the valid bit for the line.  
        valid[way][line] = TRUE;
    
        //  Check if replacement policy is defined.  
        if (policy != NULL)
        {
            //  Update replacement algorithm.  
            policy->access(way, line);
        }
    }

    /**
     *
     *  Fills a cache line.
     *
     *  Only fills the cache line with data, tag file, valid bit
     *  and replacement policy algorithm are not changed.
     *  The line being filled must already exist in the cache, if
     *  not the function causes a panic exit.
     *
     *  @param address Address of the line to be filled with data
     *  in the cache.
     *  @param data Pointer to a byte array of the size of a cache
     *  line with the data for filling the cache line.
     *
     */

    void fill(ADDRTYP address, U08 *data)
    {
        U32 line;
        U32 way;
    
        //  Check hit.  
        if (search(address, line, way))
        {
            //  Copy the data.  
            memcpy(cache[way][line], data, lineSize);
        }
        else
            CG_ASSERT("Trying to fill a non allocated line.");
    }

    /**
     *
     *  Invalidates the cache line that contains the parameter
     *  address.
     *
     *  @param address An address inside the line to invalidate.
     *
     */

    void invalidate(ADDRTYP address)
    {
        U32 line;
        U32 way;
    
        //  Check hit.  
        if (search(address, line, way))
       {
            //  Invalidate line.  
            valid[way][line] = FALSE;
        }
    }

    /**
     *  Resets the cache.
     *  Invalidates all the cache lines.
     */
    void reset()
    {
        U32 i, j;
        for(i = 0; i < numWays; i++)
        {
            for(j = 0; j < numLines; j++)
            {
                valid[i][j] = FALSE;
            }
        }
    }

};

} // namespace arch

#endif
