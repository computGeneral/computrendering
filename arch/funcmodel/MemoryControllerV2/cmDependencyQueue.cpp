/**************************************************************************
 *
 */

#include "cmDependencyQueue.h"
#include "cmChannelTransaction.h"
#include <sstream>

using namespace cg1gpu::memorycontroller;
using std::stringstream;
using std::string;

DependencyQueue::DependencyQueue() : qSize(0), qName(string("NO_NAME"))
{}

void DependencyQueue::setName(const std::string& name)
{
    qName = name;
}

const string& DependencyQueue::getName() const
{
    return qName;
}

ChannelTransaction* DependencyQueue::findDependency(const ChannelTransaction* request) const
{
    Queue::const_reverse_iterator it = q.rbegin();
    for ( ; it != q.rend(); it++ )
    {
        if ( request->overlapsWith(*(it->ct)) )
            return it->ct;
    }
    return 0;
}

void DependencyQueue::wakeup(const ChannelTransaction* dependency)
{
    Queue::reverse_iterator it = q.rbegin();
    for ( ; it != q.rend(); it++ )
    {
        if ( it->dependency == dependency )
            it->dependency = 0;
    }
}


U32 DependencyQueue::getConsecutiveAccesses(bool writes) const
{
    if ( q.empty() )
        return 0;

    U32 ccount = 0;
    Queue::const_iterator it = q.begin();

    U32 row = it->ct->getRow();
    bool isWrite = !it->ct->isRead();

    for ( ; it != q.end(); ++it )
    {
        if ( row == it->ct->getRow() && isWrite == writes  )
            ++ccount;
    }

    return ccount;
}


bool DependencyQueue::empty() const
{
    return q.empty();
}

U32 DependencyQueue::size() const
{
    //return q.size();
    return qSize;
}



void DependencyQueue::enqueue(ChannelTransaction* request, 
                              ChannelTransaction* dependency,
                              U64 timestamp)
{
    // 
    // Victor: Removed because this seems like deprecated code or code for a feature that has not been fully implemented.
    // 
    
    //if ( qSize == capacity )
    //{
    //    stringstream ss;
    //    if ( capacity == 0 ) {
    //        ss << "Dependency queue '" << qName << "' with undefined capacity (0)";
    //        CG_ASSERT(ss.str().c_str());
    //    }
    //    else {
    //        ss << "Dependency queue with name '" << qName << "' is full";
    //        CG_ASSERT(ss.str().c_str());
    //    }
    //}
    ++qSize;
    q.push_back(QueueEntry(request, dependency, timestamp));
}

U64 DependencyQueue::getTimestamp() const
{
    if ( q.empty() )
    {
        stringstream ss;
        ss << "Dependency queue with name '" << qName << "' is empty";
        CG_ASSERT(ss.str().c_str());
    }
    
    return q.front().timestamp;
}

bool DependencyQueue::ready() const
{
    return !q.empty() && q.front().dependency == 0;
}

ChannelTransaction* DependencyQueue::front() const
{
    if ( empty() )
    {
        stringstream ss;
        ss << "Dependency queue with name '" << qName << "' is empty";
        CG_ASSERT(ss.str().c_str());
    }

    return q.front().ct;
}

ChannelTransaction* DependencyQueue::frontDependency() const
{
    if ( empty() )
    {
        stringstream ss;
        ss << "Dependency queue with name '" << qName << "' is empty";
        CG_ASSERT(ss.str().c_str());
    }

    return q.front().dependency;
}

void DependencyQueue::pop()
{
    if ( empty() )
    {
        stringstream ss;
        ss << "Dependency queue with name '" << qName << "' is empty";
        CG_ASSERT(ss.str().c_str());
    }
    if ( !ready() )
    {
        stringstream ss;
        ss << "Dependency queue with name '" << qName 
           << "' cannot be popped (front transaction not ready)";
        CG_ASSERT(ss.str().c_str());
    }

    qSize--;
    q.pop_front();
}
