/**************************************************************************
 *
 */

#include "cmDependencyBuffer.h"
#include <sstream>
#include <iostream>

using namespace arch::memorycontroller;
using namespace std;

DependencyBuffer::DependencyBuffer() : 
    _bufferPolicy(BP_OldestFirst),
    // _bufferPolicy(BP_Fifo),
    _entries(0), 
    _name("UNDEFINED_NAME"), 
    _lastAccessed(NoEntry)
{
}

void DependencyBuffer::setBufferPolicy( BufferPolicy bp )
{
    _bufferPolicy = bp;
}

void DependencyBuffer::add(ChannelTransaction* ct, U64 timestamp)
{
    const ChannelTransaction* dep = _findDependency(ct);
    _buffer.push_back( _BufferEntry(ct, dep, timestamp) );
    ++_entries;
}

ChannelTransaction* DependencyBuffer::extract( BufferEntry entry )
{
    if ( entry >= _entries ) {
        stringstream ss;
        ss << "Buffer entry out of bounds (" << entry << "). DependencyBuffer name = " << getName();
        CG_ASSERT(ss.str().c_str());
    }
        
    _BufferIter bi = _buffer.begin();
    for ( ; entry != 0 && bi != _buffer.end(); ++bi, --entry )
    { ; }

    CG_ASSERT_COND(!( bi == _buffer.end() ), "Code inconsistency (between entries & iterator)");   
    ChannelTransaction* ct = bi->ct;
    _buffer.erase(bi); // remove transaction pointed by 'bi' from _buffer
    --_entries;
    _wakeup(ct);
    _lastAccessed = NoEntry; // invalidate last accessed
    return ct;
}

const ChannelTransaction* DependencyBuffer::getTransaction( BufferEntry entry ) const
{
    return _findEntry(entry, "getTransaction")->ct;
}


U64 DependencyBuffer::getTimestamp( BufferEntry entry )
{
    return _findEntry(entry, "getTimestamp")->timestamp;
}

bool DependencyBuffer::ready(BufferEntry entry) const
{
    return _findEntry(entry, "ready")->dependency == 0;
}

void DependencyBuffer::setName(const std::string& name)
{
    _name = name;
}

const string& DependencyBuffer::getName() const
{
    return _name;
}


U32 DependencyBuffer::getAccesses(BufferEntry entry) const
{
    const U32 row = _findEntry(entry, "getAccesses")->ct->getRow();
    U32 count = 0;
    for ( _BufferConstIter it = _buffer.begin(); it != _buffer.end(); ++it ) {
        if ( it->ct->getRow() == row )
            ++count;
    }
    return count;
}

U32 DependencyBuffer::size() const
{
    return _entries;
}

bool DependencyBuffer::empty() const
{
    return (_entries == 0);
}


void DependencyBuffer::_wakeup(const ChannelTransaction* ct)
{
    _BufferIter it = _buffer.begin();
    for ( ; it != _buffer.end(); ++it ) {
        if ( it->dependency == ct )
            it->dependency = 0; // clear dependency
    }
}

U32 DependencyBuffer::countHits(U32 page, bool findWrites) const
{
    U32 hits = 0;

    if ( _bufferPolicy == BP_OldestFirst ) {
        for ( _BufferConstIter it = _buffer.begin(); it != _buffer.end(); ++it ) {
            if ( it->ct->getRow() == page && !it->ct->isRead() == findWrites )
                ++hits;
        }
    }
    else if ( _bufferPolicy == BP_Fifo ) {
        for ( _BufferConstIter it = _buffer.begin(); it != _buffer.end(); ++it ) { // only consecutive accesses
            if ( it->ct->getRow() == page && !it->ct->isRead() == findWrites )
                ++hits;
            else
                break;
        }
    }

    return hits;
}


DependencyBuffer::BufferEntry DependencyBuffer::getCandidate(bool findWrite) const
{
    if ( _bufferPolicy == BP_OldestFirst ) {
        _BufferConstIter it = _buffer.begin();
        _BufferConstIter end = _buffer.end();
        U32 curpos = 0;
        for ( ; it != end; ++it, ++curpos ) {
            if ( it->ct->isRead() == !findWrite && it->dependency == 0 ) // maybe it->dependency == 0 not required in this case
                return static_cast<BufferEntry>(curpos);
        }
        return NoEntry;
    }
    else if ( _bufferPolicy == BP_Fifo ) {
        if ( !_buffer.empty() && _buffer.front().ct->isRead() == !findWrite && _buffer.front().dependency == 0 )
            return static_cast<BufferEntry>(0); // first entry
        return NoEntry;
    }
    CG_ASSERT("Unsupported policy");
    return 0; 

}


DependencyBuffer::BufferEntry DependencyBuffer::getCandidate(bool findWrite, U32 preferredRow) const
{
    if ( _bufferPolicy == BP_OldestFirst ) {
        BufferEntry firstFound = NoEntry;
        _BufferConstIter it = _buffer.begin();
        U32 curpos = 0;
        for ( ; it != _buffer.end(); ++it, ++curpos ) {
            // Check if the transaction is of the type searched and whether it's ready (aka no dependency)
            if ( it->ct->isRead() == !findWrite && it->dependency == 0 ) {
                if ( firstFound == NoEntry )
                    firstFound = curpos;
                if ( it->ct->getRow() == preferredRow )
                    return static_cast<BufferEntry>(curpos);
            }
        }
        return firstFound;
    }
    else if ( _bufferPolicy == BP_Fifo ) {
        if ( !_buffer.empty() && _buffer.front().ct->isRead() == !findWrite && _buffer.front().dependency == 0 )
            return static_cast<BufferEntry>(0); // first entry
        return NoEntry;
    }
    CG_ASSERT("Unsupported policy");
    return 0;
}


DependencyBuffer::_BufferConstIter DependencyBuffer::_findEntry( BufferEntry entry, const char* callerMethodName) const
{
    if ( _lastAccessed == entry )
        return _lastAccessedIter;

    if ( entry >= _entries ) {
        stringstream ss;
        ss << "Buffer entry ' " << entry << "' out of bounds (available entries = " << _entries << "). Method called from '" << callerMethodName << "'. getName() = " << getName();
        CG_ASSERT(ss.str().c_str());
    }
   
    U32 pos = static_cast<U32>(entry);
    _BufferConstIter it = _buffer.begin();
    for ( ; pos != 0; ++it, --pos ) 
    { ; }

    _lastAccessed = entry;
    _lastAccessedIter = it;

    return it;
}


const ChannelTransaction* DependencyBuffer::_findDependency( const ChannelTransaction* ct ) const
{
    list<_BufferEntry>::const_reverse_iterator it = _buffer.rbegin();
    const bool ctIsRead = ct->isRead();
    for ( ; it != _buffer.rend(); ++it ) {
        if ( ctIsRead && !it->ct->isRead() && ct->overlapsWith(*it->ct) ) // Read after Write dependency
            return it->ct;
        else if ( !ctIsRead && ct->overlapsWith(*it->ct) )
            return it->ct;
    }
    return 0;
}

void DependencyBuffer::dump() const
{
    _BufferConstIter it = _buffer.begin();
    for ( ; it != _buffer.end(); ++it ) {
        const ChannelTransaction* ct = it->ct;
        if ( ct->isRead() ) 
            cout << "| r" << ct->getRow() << "(" << (it->dependency ? "1" : "0") << ") ";
        else 
            cout << "| w" << ct->getRow() << "(" << (it->dependency ? "1" : "0") << ") ";
    }
}

