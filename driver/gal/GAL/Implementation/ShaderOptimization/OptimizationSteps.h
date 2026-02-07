/**************************************************************************
 *
 */

#ifndef OPTIMIZATION_STEPS_H
    #define OPTIMIZATION_STEPS_H

#include "ShaderOptimizer.h"

#include <set>
#include <utility>
#include <vector>

namespace libGAL
{

namespace libGAL_opt
{

class DependencyGraph;

class MaxLiveTempsAnalysis: public OptimizationStep
{
public:
    
    MaxLiveTempsAnalysis(ShaderOptimizer* shOptimizer);

    virtual void optimize();

private:

    gal_uint _countMatchingRanges(std::set<std::pair<gal_uint,gal_uint> >& ranges, gal_uint instructionPos);

};

class RegisterUsageAnalysis: public OptimizationStep
{
public:
    
    RegisterUsageAnalysis(ShaderOptimizer* shOptimizer);

    virtual void optimize();
};

class StaticInstructionScheduling: public OptimizationStep
{
public:

    StaticInstructionScheduling(ShaderOptimizer* shOptimizer);

    virtual void optimize();

private:

    void _schedule(DependencyGraph& dg, std::vector<InstructionInfo*>& code);

    DependencyGraph* _buildDependencyGraph(const std::vector<InstructionInfo*>& code, unsigned int temporaries,
                                           unsigned int outputRegs, unsigned int addrRegs, unsigned int predRegs) const;
    void _printDependencyGraph(const DependencyGraph& dg, unsigned int cycle, bool isInitialGraph = false) const;
    std::list<unsigned int> _buildInitialUnScheduledSet(const std::vector<InstructionInfo*>& code) const;
    std::list<unsigned int> _getReadyOps(const DependencyGraph& dg, unsigned int cycle, const std::vector<IssueInfo>& scheduledSet, const std::vector<InstructionInfo*>& code) const;
    void _sortReadyOps(std::list<unsigned int>& readySet, const DependencyGraph& dg) const;
    void _reorderCode(std::vector<InstructionInfo*>& originalCode, const std::vector<IssueInfo>& orderList) const;
};

} // namespace libGAL_opt

} // namespace libGAL

#endif // OPTIMIZATION_STEPS_H
