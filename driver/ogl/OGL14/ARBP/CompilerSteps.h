/**************************************************************************
 *
 */

#ifndef COMPILERSTEPS_H
    #define COMPILERSTEPS_H

#include "Scheduler.h"
#include "DependencyGraph.h"
#include <vector>
#include <list>
#include <bitset>
#include <queue>
#include <set>

namespace libgl
{

namespace GenerationCode
{
/*
 * Performs previous basic optimizations such as: 
 *   * operation folding,
 *   * copy propagation,
 *   * common subexpression elimination,
 *   * dead code elimination
 *  and some important analysis as:
 *   * liveness analysis
 *  NOT IMPLEMENTED YET
 */
void basicOptimizations(std::vector<InstructionInfo*>& code);

/* 
 * Assigns an unused temporary each time to instructions that write a temporary while temporaries
 * are available. Removes false WAR and WAW dependencies.
 * NOT IMPLEMENTED YET
 */
void registerRenaming(std::vector<InstructionInfo*>& code, unsigned int temporaries);

/* 
 * Return the list of ready for execution instructions 
 * according to source operand values availability 
 */
std::list<unsigned int> getReadyOps(const DependencyGraph& dg,
                                    unsigned int cycle,
                                    const std::vector<IssueInfo>& scheduledSet,
                                    const std::vector<InstructionInfo*>& code);

/*
 * Sorts list of ready for execution instructions
 * according to some policy as number of child dependent instructions
 * that instruction can free if scheduled
 */
void sortReadyOps(std::list<unsigned int>& readySet, const DependencyGraph& dg);


U32 computeMaxLiveTemps(std::vector<InstructionInfo*>& code);

// Aux, used inside computeMaxLiveTemps
U32 countMatchingRanges(std::set<std::pair<U32,U32> >& ranges, U32 instructionPos);

} // namespace GenerationCode

} // namespace libgl

#endif // COMPILERSTEPS_H
