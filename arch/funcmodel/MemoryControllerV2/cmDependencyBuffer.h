/**************************************************************************
 *
 */

#ifndef DEPENDENCYBUFFER_H
    #define DEPENDENCYBUFFER_H

#include "GPUType.h"
#include "cmChannelTransaction.h"
#include <list>
#include <vector>
#include <set>

namespace cg1gpu
{
namespace memorycontroller
{

class DependencyBuffer
{

public:

    enum BufferPolicy
    {
        BP_Fifo, // For debug purposes (Dependency buffer behaves like a FIFO)
        BP_OldestFirst,
    };

    typedef U32 BufferEntry;

    static const BufferEntry NoEntry = 0xFFFFFFFF;

    DependencyBuffer();

    void setBufferPolicy( BufferPolicy bp );

    void add(ChannelTransaction* ct, U64 timestamp);

    // Extracts the transaction and wakes up all dependent transactions
    ChannelTransaction* extract( BufferEntry entry );

    const ChannelTransaction* getTransaction( BufferEntry entry ) const;

    // Check if exists a pending transaction targeting a given page
    U32 countHits(U32 page, bool findWrites) const;    

    U64 getTimestamp( BufferEntry entry );
    U32 getAccesses(BufferEntry entry) const;

    bool empty() const;

    U32 size() const;

    // Gets a candidate preferring to hit a specific page
    BufferEntry getCandidate(bool findWrite, U32 preferredRow ) const;
    // Gets a candidate without try to match a specific page
    BufferEntry getCandidate(bool findWrite) const;

    bool ready(BufferEntry entry) const;

    void setName(const std::string& name);

    const std::string& getName() const;

    void dump() const;


private:

    struct _BufferEntry
    {
        ChannelTransaction* ct;
        const ChannelTransaction* dependency;
        U64 timestamp;

        _BufferEntry(ChannelTransaction* ct, const ChannelTransaction* dependency, U64 timestamp) :
            ct(ct), dependency(dependency), timestamp(timestamp) {}
    };

    typedef std::list<_BufferEntry> _Buffer;
    typedef _Buffer::iterator _BufferIter;
    typedef _Buffer::const_iterator _BufferConstIter;

    _BufferConstIter _findEntry( BufferEntry entry, const char* callerMethodName) const;

    _Buffer _buffer;
    BufferPolicy _bufferPolicy;
    U32 _entries;
    std::string _name;
    mutable BufferEntry _lastAccessed;
    mutable _BufferConstIter _lastAccessedIter;

    const ChannelTransaction* _findDependency(const ChannelTransaction* ct) const;
    void _wakeup(const ChannelTransaction*);

};

} // namespace memorycontroller
} // namespace cg1gpu

#endif // DEPENDENCYBUFFER_H
