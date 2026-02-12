/**************************************************************************
 *
 */
#ifndef GPU3D_TOOLS_QUEUE_H
    #define GPU3D_TOOLS_QUEUE_H

#include <queue>
#include <list>
#include <vector>
#include <iostream>
#include "support.h"
#include "GPUType.h"
#include <string>
#include <map>
#include <cmath>
#include <algorithm>
#include <cstring>

//#define USE_STD_QUEUE

using namespace std;

namespace arch
{
namespace tools
{

class GenericQueue
{
protected:
    std::string name;    

public:
    GenericQueue(std::string name)
        : name(name)
    {
    };
};


/**
 * Templatized class implementing a generic queue
 */
template<class Item>
class Queue: public GenericQueue
{
public:

    /**
     * Creates a Queue object
     *
     * @note Capacity is set to 0, it must be selected with resize() before add
     * any item. This constructor is supplied to allow the creation of arrays of
     * queues with low overhead and unknown size
     */
    Queue();

    /**
     * Creates a Queue object
     *
     * @param size maximum number of items that can be hold by the queue
     */
    Queue(U32 size);

    /**
     * Resizes the queue, the previous contents are lost
     *
     */
    void resize(U32 newSize);

    /**
     * Add a new item to the queue
     *
     * @param item the item to be added
     */
    void add(Item item);

    /**
     * Remove the item in the head (the first) of the queue
     */
    void remove();

    /**
     * Returns the head (the first element) of the queue
     *
     * @return a reference to the element at the queue head
     */
    Item& head();

    /**
     * Returns a copy of the head (the first element) of the queue and
     * remove this element from the queue
     *
     * @code
     *     Queue<Object> queue;
     *     // insert some "Objects"...
     *
     *     Object o = queue.head();
     *     queue.remove();
     *     // These to calls are almost equivalent to:
     *     Object o = queue.pop();
     * @endcode
     *
     * @note This method is slightly different in the sense that it returns a copy
     *       of the head and not a reference of the head. If the Queue stores pointers
     *       the behaviour is exactly the same (a copy of the pointer is the same pointer)
     *       that with the sequence head/remove. Otherwise, if the queue contains objects
     *       the copy constructor is used to return a copy
     */
    Item pop();

    /**
     * Reserves a number of entries in the queue.
     *
     * It doesn�t actually add new items to the queue. It only decrements
     * the number of free entries returned by the free() method.
     *
     * @param entries the number of entries to reserve in the queue.
     *
     * @note throws an exception (panic) if not enough free entries in the queue.
     */
    void reserve(U32 entries);

    /**
     * Cancels previous reserves.
     *
     * It only increments the number of free entries returned by the free()
     * method and consecuently the full() returned bool.
     *
     * @param entries the number of entries to unreserve.
     *
     * @note throws an exception (panic) if trying to unreserve more entries
     *       than previously reserved or total available.
     */
    void unreserve(U32 entries);

    /**
     * Checks if the queue is empty
     *
     * @return true if the queue is empty, false otherwise
     */
    bool empty() const;

    /**
     * Checks if the queue is full
     *
     * @return true if the queue is full, false otherwise
     */
    bool full() const;

    /**
     * Returns the number of items that the queue holds
     *
     * @return items hold by the queue
     */
    U32 items() const;

    /**
     * Returns the number of free entries in the queue
     *
     * @return free entries in the queue
     */
    U32 free() const;

    /**
     * Returns the maximum number of items that can be hold by the queue
     *
     * @return the capacity of the queue
     */
    U32 capacity() const;

    /**
     * remove all the items in the queue
     */
    void clear();

    /**
     *  Set queue name.
     *
     *  @param name The queue name.
     *
     */

    void setName(std::string name);

    /**
     * Gets the queue name
     *
     * @return the name of this queue
     */
    std::string getName() const;

    /**
     * Dumps the queue contents
     *
     * @note Requires operator << defined
     */
    void dump() const;

private:

#ifdef USE_STD_QUEUE
    std::queue<Item> q;
#endif

    Item *data;
    U32 first;
    U32 last;
    U32 elements;    
    U32 storageSize;
    U32 storageSizeMask;
   
    U32 freeEntries;
    bool isEmpty;
    bool isFull;
    
    U32 maxSize;
    U32 reserves;
};

/**
 * Templatized class implementing an addressable queue
 */
template<class Item>
class AddressableQueue: public GenericQueue
{
public:

    /**
     * Creates a AddressableQueue object
     *
     * @note Capacity is set to 0, it must be selected with resize() before add
     * any item. This constructor is supplied to allow the creation of arrays of
     * queues with low overhead and unknown size
     */
    AddressableQueue();

    /**
     * Creates a AddressableQueue object
     *
     * @param size maximum number of items that can be hold by the queue
     */
    AddressableQueue(U32 size);

    /**
     * Resizes the queue, the previous contents are lost
     *
     */
    void resize(U32 newSize);

    /**
     * Add a new item to the queue
     *
     * @param item the item to be added
     * @return The item stored position in the queue.
     */
    U32 add(Item item);

    /**
     * Remove the item in the head (the first) of the queue
     */
    void remove();

    /**
     * Returns the head (the first element) of the queue
     *
     * @return a reference to the element at the queue head
     */
    Item& head();

    /**
     * Returns a copy of the head (the first element) of the queue and
     * remove this element from the queue
     *
     * @code
     *     AddressableQueue<Object> queue;
     *     // insert some "Objects"...
     *
     *     Object o = queue.head();
     *     queue.remove();
     *     // These to calls are almost equivalent to:
     *     Object o = queue.pop();
     * @endcode
     *
     * @note This method is slightly different in the sense that it returns a copy
     *       of the head and not a reference of the head. If the Queue stores pointers
     *       the behaviour is exactly the same (a copy of the pointer is the same pointer)
     *       that with the sequence head/remove. Otherwise, if the queue contains objects
     *       the copy constructor is used to return a copy
     */
    Item pop();

    /**
     * Returns the element stored at the specified position.
     *
     * @param position the element position.
     * @return a pointer to the element located at the specified queue position.
     *
     * @note A panic is thrown if not valid position.
     */
    Item* lookup(U32 position);

    /**
     * Reserves a number of entries in the queue.
     *
     * It does not actually add new items to the queue. It only decrements
     * the number of free entries returned by the free() method.
     *
     * @param entries the number of entries to reserve in the queue.
     *
     * @note throws an exception (panic) if not enough free entries in the queue.
     */
    void reserve(U32 entries);

    /**
     * Cancels previous reserves.
     *
     * It only increments the number of free entries returned by the free()
     * method and consecuently the full() returned bool.
     *
     * @param entries the number of entries to unreserve.
     *
     * @note throws an exception (panic) if trying to unreserve more entries
     *       than previously reserved or total available.
     */
    void unreserve(U32 entries);

    /**
     * Checks if the queue is empty
     *
     * @return true if the queue is empty, false otherwise
     */
    bool empty() const;

    /**
     * Checks if the queue is full
     *
     * @return true if the queue is full, false otherwise
     */
    bool full() const;

    /**
     * Returns the number of items that the queue holds
     *
     * @return items hold by the queue
     */
    U32 items() const;

    /**
     * Returns the number of free entries in the queue
     *
     * @return free entries in the queue
     */
    U32 free() const;

    /**
     * Returns the maximum number of items that can be hold by the queue
     *
     * @return the capacity of the queue
     */
    U32 capacity() const;

    /**
     * remove all the items in the queue
     */
    void clear();

    /**
     *  Set queue name.
     *
     *  @param name The queue name.
     *
     */

    void setName(std::string name);

    /**
     * Gets the queue name
     *
     * @return the name of this queue
     */
    std::string getName() const;

    /**
     * Dumps the queue contents
     *
     * @note Requires operator << defined
     */
    void dump() const;

private:

    std::queue<U32> q;
    std::vector<Item> contents;
    std::list<U32> freeList;

    U32 maxSize;
    U32 reserves;
};


/**
 * Templatized class implementing a content addressable queue with associated
 * data per entry.
 */
template<class Key, class Data>
class ContentAddressableQueue: public GenericQueue
{
public:

    /**
     * Creates a ContentAddressableQueue object
     *
     * @note Capacity is set to 0, it must be selected with resize() before add
     * any item. This constructor is supplied to allow the creation of arrays of
     * queues with low overhead and unknown size
     */
    ContentAddressableQueue();

    /**
     * Creates a ContentAddressableQueue object
     *
     * @param size maximum number of items that can be hold by the queue
     */
    ContentAddressableQueue(U32 size);

    /**
     * Resizes the queue, the previous contents are lost
     *
     */
    void resize(U32 newSize);

    /**
     * Add a new item to the queue
     *
     * @param key The new item key.
     * @param data The new item data.
     * @param position Returns the stored position in the queue.
     *
     * @return True if item could be added, false if key already exists.
     */
    bool add(Key key, Data data, U32& position);

    /**
     * Remove the item in the head (the first) of the queue
     */
    void remove();

    /**
     * Returns the head (the first element) of the queue
     *
     * @return a reference to the element at the queue head
     */
    Data& head();

    /**
     * Returns a copy of the head (the first element) of the queue and
     * remove this element from the queue
     *
     * @code
     *     ContentAddressableQueue<Object> queue;
     *     // insert some "Objects"...
     *
     *     Object o = queue.head();
     *     queue.remove();
     *     // These to calls are almost equivalent to:
     *     Object o = queue.pop();
     * @endcode
     *
     * @note This method is slightly different in the sense that it returns a copy
     *       of the head and not a reference of the head. If the Queue stores pointers
     *       the behaviour is exactly the same (a copy of the pointer is the same pointer)
     *       that with the sequence head/remove. Otherwise, if the queue contains objects
     *       the copy constructor is used to return a copy
     */
    Data pop();

    /**
     * Searches for the key-matching entry in the queue.
     *
     * @param searchKey The key to search for in the queue.
     * @param position Returns the address of the single matching position in the queue.
     *
     * @return If the key was found in the queue.
     */

    bool search(Key searchKey, U32& position);

    /**
     * Returns the element stored at the specified position.
     *
     * @param position the element position.
     * @return a pointer to the element located at the specified queue position.
     *
     * @note A panic is thrown if not valid position.
     */
    Data* lookup(U32 position);

    /**
     * Reserves a number of entries in the queue.
     *
     * It does not actually add new items to the queue. It only decrements
     * the number of free entries returned by the free() method.
     *
     * @param entries the number of entries to reserve in the queue.
     *
     * @note throws an exception (panic) if not enough free entries in the queue.
     */
    void reserve(U32 entries);

    /**
     * Cancels previous reserves.
     *
     * It only increments the number of free entries returned by the free()
     * method and consecuently the full() returned bool.
     *
     * @param entries the number of entries to unreserve.
     *
     * @note throws an exception (panic) if trying to unreserve more entries
     *       than previously reserved or total available.
     */
    void unreserve(U32 entries);

    /**
     * Checks if the queue is empty
     *
     * @return true if the queue is empty, false otherwise
     */
    bool empty() const;

    /**
     * Checks if the queue is full
     *
     * @return true if the queue is full, false otherwise
     */
    bool full() const;

    /**
     * Returns the number of items that the queue holds
     *
     * @return items hold by the queue
     */
    U32 items() const;

    /**
     * Returns the number of free entries in the queue
     *
     * @return free entries in the queue
     */
    U32 free() const;

    /**
     * Returns the maximum number of items that can be hold by the queue
     *
     * @return the capacity of the queue
     */
    U32 capacity() const;

    /**
     * remove all the items in the queue
     */
    void clear();

    /**
     *  Set queue name.
     *
     *  @param name The queue name.
     *
     */

    void setName(std::string name);

    /**
     * Gets the queue name
     *
     * @return the name of this queue
     */
    std::string getName() const;

    /**
     * Dumps the queue contents
     *
     * @note Requires operator << defined
     */
    void dump() const;

private:

    std::map<Key, U32> searchTable;
    std::queue<U32> q;
    std::vector<std::pair<Key, Data> > contents;
    std::list<U32> freeList;

    U32 maxSize;
    U32 reserves;
};

////////////////////////////////////////////////////////////////////////////
//                         Queue Implementation                           //
////////////////////////////////////////////////////////////////////////////

template<class Item>
Queue<Item>::Queue() :
    data(NULL), first(0), last(0), elements(0), freeEntries(0), isEmpty(true), isFull(true), storageSize(0), storageSizeMask(0),
    maxSize(0), reserves(0), GenericQueue("Queue")
{}

template<class Item>
Queue<Item>::Queue(U32 size) :
    first(0), last(0), elements(0), freeEntries(size), isEmpty(true), isFull(true), maxSize(size), reserves(0), GenericQueue("Queue")
{
#ifndef USE_STD_QUEUE
    if (size > 0)
    {
        U32 log2MaxSize = (U32) ceil(log(double(maxSize)) / log(2.0));
        storageSize = 1 << log2MaxSize;
        storageSizeMask = storageSize - 1;
        data = new Item[storageSize];
        last = storageSizeMask;
        isFull = false;
    }
#endif    
}

template<class Item>
void Queue<Item>::resize(U32 newSize)
{
#ifndef USE_STD_QUEUE

    if (newSize == maxSize)
        return;

    GPU_ASSERT(
        if (newSize < elements)
        {
            char buffer[256];
            sprintf(buffer, "Trying to resize to a size smaller than the current number of elements in queue of class %s.", name.c_str());
            CG_ASSERT(buffer);
        }
    )

    if (storageSize > 0)
    {
        if (newSize > 0)
        {
            U32 log2NewMaxSize = (U32) ceil(log(double(newSize)) / log(2.0));
            U32 newStorageSize = 1 << log2NewMaxSize;
            storageSizeMask = storageSize - 1;

            Item *dataAux = data;
            data = new Item[newStorageSize];
            memcpy(data, dataAux, sizeof(Item) * storageSize);
            storageSize = newStorageSize;
            last = storageSizeMask;
            delete[] dataAux;
        }
        else
        {
            storageSize = 0;
            storageSizeMask = 0;
            delete[] data;
        }
    }
    else
    {
        U32 log2MaxSize = (U32) ceil(log(double(newSize)) / log(2.0));
        storageSize = 1 << log2MaxSize;
        storageSizeMask = storageSize - 1;
        data = new Item[storageSize];
        last = storageSizeMask;
    }
    
    isFull = false;
    
    freeEntries = freeEntries + (newSize - maxSize);
#endif
    
    maxSize = newSize;
}

template<class Item>
void Queue<Item>::add(Item item)
{
    GPU_ASSERT(
        if (items() == maxSize)
        {
            char buffer[256];
            sprintf(buffer, "Maximum capacity exceed in queue of class %s.", name.c_str());
            CG_ASSERT(buffer);
        }
    )
    
#ifdef USE_STD_QUEUE
    q.push(item);
#else
    last = (last + 1) & storageSizeMask;
    data[last] = item;
    elements++;
    isEmpty = false;
#endif

    if (reserves > 0)
        reserves--;
#ifndef USE_STD_QUEUE
    else
        freeEntries--;

    isFull = (freeEntries == 0);
#endif    
}

template<class Item>
std::string Queue<Item>::getName() const
{
    return name;
}

template<class Item>
void Queue<Item>::remove()
{
#ifdef USE_STD_QUEUE
    q.pop();
#else
    GPU_ASSERT(
        if (elements == 0)
        {
            char buffer[256];
            sprintf(buffer, "Queue of class %s is empty.", name.c_str());
            CG_ASSERT(buffer);
        }
    )
    
    first = (first + 1) & storageSizeMask;
    elements--;   
    freeEntries++;
    isEmpty = (elements == 0);
    isFull = false;
#endif    
}

template<class Item>
Item Queue<Item>::pop()
{
#ifdef USE_STD_QUEUE
    Item i = q.front(); // copy constructor used
    q.pop();
    return i;
#else
    GPU_ASSERT(
        if (elements == 0)
        {
            char buffer[256];
            sprintf(buffer, "Queue of class %s is emtpy.", name.c_str());
            CG_ASSERT(buffer);
        }
    )
    
    Item i = data[first];
    remove();
    return i;
#endif    
}

template<class Item>
void Queue<Item>::reserve(U32 entries)
{
    if (free() < entries)
    {
        char buffer[256];
        sprintf(buffer, "Requested reserves exceeds available free entries for queue of class %s.", name.c_str());
        CG_ASSERT(buffer);
    }

    reserves += entries;

#ifndef USE_STD_QUEUE    
    freeEntries -= reserves;
    isFull = (freeEntries == 0);
#endif    
}

template<class Item>
void Queue<Item>::unreserve(U32 entries)
{
    if (entries > reserves)
    {
        char buffer[256];
        sprintf(buffer, "Exceeded previous reserves for queue class %s.", name.c_str());
        CG_ASSERT(buffer);
    }

    reserves -= entries;

#ifndef USE_STD_QUEUE
    freeEntries += entries;
    isFull = false;
#endif    
}


template<class Item>
Item& Queue<Item>::head()
{
#ifdef USE_STD_QUEUE
    return q.front();
#else
    GPU_ASSERT(
        if (elements == 0)
        {
            char buffer[256];
            sprintf(buffer, "Queue of class %s is empty.", name.c_str());
            CG_ASSERT(buffer);
        }
    )

    return data[first]; 
#endif
}

template<class Item>
bool Queue<Item>::empty() const
{
#ifdef USE_STD_QUEUE
    return q.empty();
#else
    //return (elements == 0);
    return isEmpty;
#endif    
}

template<class Item>
U32 Queue<Item>::items() const
{
#ifdef USE_STD_QUEUE
    return q.size();
#else
    return elements;
#endif    
}

template<class Item>
U32 Queue<Item>::free() const
{
#ifdef USE_STD_QUEUE
    return (maxSize - (q.size() + reserves));
#else
    //return (maxSize - (elements + reserves));
    return freeEntries;
#endif
}

template<class Item>
U32 Queue<Item>::capacity() const
{
    return maxSize;
}

template<class Item>
bool Queue<Item>::full() const
{
#ifdef USE_STD_QUEUE
    return ((q.size() + reserves) == maxSize);
#else
    //return ((elements + reserves) == maxSize);
    return isFull;
#endif
}

template<class Item>
void Queue<Item>::clear()
{
#ifdef USE_STD_QUEUE
    while ( !q.empty() )
        q.pop();
#else
    first = (last + 1) & storageSizeMask;
    elements = 0;
    reserves = 0;
    freeEntries = maxSize;
    isEmpty = true;
    isFull = false;
#endif
}

template<class Item>
void Queue<Item>::setName(std::string qName)
{
    name = qName;
}

template<class Item>
void Queue<Item>::dump() const
{
#ifdef USE_STD_QUEUE
    std::queue<Item> qaux(q);
    cout << "{";
    while ( !qaux.empty() )
    {
        Item i = qaux.front();
        cout << i << ",";
        qaux.pop();
    }
    cout << "}";
#else
    for(U32 i = 0, next = first; i < elements; i++)
    {
        Item item = data[next];
        cout << item << ",";
        next = (next + 1) & storageSizeMask;
    }
#endif    
}

////////////////////////////////////////////////////////////////////////////
//                     AddressableQueue Implementation                    //
////////////////////////////////////////////////////////////////////////////

template<class Item>
AddressableQueue<Item>::AddressableQueue()
: maxSize(0), reserves(0), GenericQueue("AddressableQueue")
{}

template<class Item>
AddressableQueue<Item>::AddressableQueue(U32 size)
: maxSize(size), contents(size), reserves(0), GenericQueue("AddressableQueue")
{
    for (U32 i = 0; i < size; i++)
    {
        freeList.push_back(i);
    }
}

template<class Item>
void AddressableQueue<Item>::resize(U32 newSize)
{
    maxSize = newSize;

    contents.resize(newSize);

    freeList.clear();

    for (U32 i = 0; i < newSize; i++)
    {
        freeList.push_back(i);
    }
}

template<class Item>
U32 AddressableQueue<Item>::add(Item item)
{
    U32 newPos;
    std::string s;

    if ( q.size() == maxSize )
    {
        s.clear();
        s.append("Maximum capacity exceed in queue : ");
        s.append(name);
        CG_ASSERT(s.c_str());
    }

    newPos = freeList.front();
    contents[newPos] = item;
    q.push(newPos);
    freeList.pop_front();

    if (reserves > 0)
        reserves--;

    return newPos;
}

template<class Item>
std::string AddressableQueue<Item>::getName() const
{
    return name;
}

template<class Item>
void AddressableQueue<Item>::remove()
{
    U32 pos;

    pos = q.front();
    freeList.push_back(pos);
    q.pop();
}

template<class Item>
Item AddressableQueue<Item>::pop()
{
    U32 pos;

    pos = q.front();
    freeList.push_back(pos);
    q.pop();

    return contents[pos]; // copy constructor used when returning
}

template<class Item>
Item* AddressableQueue<Item>::lookup(U32 position)
{
    std::string s;
    std::list<U32>::iterator result;

    if (position > maxSize)
    {
        s.clear();
        s.append("Queue Position Out-of-range: ");
        s.append(name);
        CG_ASSERT(s.c_str());
    }

    result = find(freeList.begin(), freeList.end(), position);

    if (result == freeList.end())
    {
        // position not found in the free list. It is a valid position.
        return &(contents[position]);
    }
    else
    {
        s.clear();
        s.append("Unoccuppied queue position : ");
        s.append(name);
        CG_ASSERT(s.c_str());
    }

    return 0;
}

template<class Item>
void AddressableQueue<Item>::reserve(U32 entries)
{
    std::string s;

    if (free() < entries)
    {
        s.clear();
        s.append("Requested reserves exceeds available free entries : ");
        s.append(name);
        CG_ASSERT(s.c_str());
    }

    reserves += entries;
}

template<class Item>
void AddressableQueue<Item>::unreserve(U32 entries)
{
    std::string s;

    if (entries > reserves)
    {
        s.clear();
        s.append("Exceeded previous reserves  : ");
        s.append(name);
        CG_ASSERT(s.c_str());
    }

    reserves -= entries;
}

template<class Item>
Item& AddressableQueue<Item>::head()
{
    U32 pos;

    pos = q.front();

    return contents[pos];
}

template<class Item>
bool AddressableQueue<Item>::empty() const
{
    return q.empty();
}

template<class Item>
U32 AddressableQueue<Item>::items() const
{
    return q.size();
}

template<class Item>
U32 AddressableQueue<Item>::free() const
{
    return (maxSize - (q.size() + reserves));
}

template<class Item>
U32 AddressableQueue<Item>::capacity() const
{
    return maxSize;
}

template<class Item>
bool AddressableQueue<Item>::full() const
{
    return ((q.size() + reserves) == maxSize);
}

template<class Item>
void AddressableQueue<Item>::clear()
{
    while ( !q.empty() )
    {
        freeList.push_back(q.front());
        q.pop();
    }
}

template<class Item>
void AddressableQueue<Item>::setName(std::string qName)
{
    name = qName;
}

template<class Item>
void AddressableQueue<Item>::dump() const
{
    std::queue<U32> qaux(q);
    cout << "{";
    while ( !qaux.empty() )
    {
        U32 i = qaux.front();
        cout << i << ",";
        qaux.pop();
    }
    cout << "}";
}

////////////////////////////////////////////////////////////////////////////
//                ContentAddressableQueue Implementation                  //
////////////////////////////////////////////////////////////////////////////

template<class Key, class Data>
ContentAddressableQueue<Key, Data>::ContentAddressableQueue()
: maxSize(0), reserves(0), GenericQueue("ContentAddressableQueue")
{}

template<class Key, class Data>
ContentAddressableQueue<Key, Data>::ContentAddressableQueue(U32 size)
: maxSize(size), contents(size), reserves(0), GenericQueue("ContentAddressableQueue")
{
    for (U32 i = 0; i < size; i++)
    {
        freeList.push_back(i);
    }
}

template<class Key, class Data>
void ContentAddressableQueue<Key, Data>::resize(U32 newSize)
{
    maxSize = newSize;

    contents.resize(newSize);

    freeList.clear();

    for (U32 i = 0; i < newSize; i++)
    {
        freeList.push_back(i);
    }
}

template<class Key, class Data>
bool ContentAddressableQueue<Key, Data>::add(Key key, Data data, U32& position)
{
    U32 newPos;
    std::string s;
    typename std::pair<typename std::map<Key, U32>::iterator, bool> insertResult;

    if ( q.size() == maxSize )
    {
        s.clear();
        s.append("Maximum capacity exceed in queue : ");
        s.append(name);
        CG_ASSERT(s.c_str());
    }

    newPos = freeList.front();

    insertResult = searchTable.insert(std::make_pair(key, newPos));

    if (!insertResult.second)
    {
        //  The key already exists.
        return false;
    }

    position = newPos;

    q.push(newPos);

    contents[newPos] = make_pair(key, data);

    freeList.pop_front();

    if (reserves > 0)
        reserves--;

    return true;
}

template<class Key, class Data>
std::string ContentAddressableQueue<Key, Data>::getName() const
{
    return name;
}

template<class Key, class Data>
void ContentAddressableQueue<Key, Data>::remove()
{
    U32 pos;
    Key key;

    pos = q.front();

    key = contents[pos].first;

    searchTable.erase(key);

    freeList.push_back(pos);

    q.pop();
}

template<class Key, class Data>
Data ContentAddressableQueue<Key, Data>::pop()
{
    U32 pos;
    Key key;

    pos = q.front();

    key = contents[pos].first;

    searchTable.erase(key);

    freeList.push_back(pos);

    q.pop();

    return contents[pos].second; // copy constructor used when returning
}

template<class Key, class Data>
bool ContentAddressableQueue<Key, Data>::search(Key searchKey, U32 &position)
{
    typename std::map<Key, U32>::iterator iter;

    iter = searchTable.find(searchKey);

    if (iter == searchTable.end())
    {
        //  Key not found.
        return false;
    }

    position = iter->second;

    return true;
}

template<class Key, class Data>
Data* ContentAddressableQueue<Key, Data>::lookup(U32 position)
{
    std::string s;
    std::list<U32>::const_iterator result;

    if (position > maxSize)
    {
        s.clear();
        s.append("Queue Position Out-of-range: ");
        s.append(name);
        CG_ASSERT(s.c_str());
    }

    result = find(freeList.begin(), freeList.end(), position);

    if (result == freeList.end())
    {
        // position not found in the free list. It is a valid position.
        return &(contents[position].second);
    }
    else
    {
        s.clear();
        s.append("Unoccuppied queue position : ");
        s.append(name);
        CG_ASSERT(s.c_str());
        return 0;  //  To prevent compiler warning.
    }
}

template<class Key, class Data>
void ContentAddressableQueue<Key, Data>::reserve(U32 entries)
{
    std::string s;

    if (free() < entries)
    {
        s.clear();
        s.append("Requested reserves exceeds available free entries : ");
        s.append(name);
        CG_ASSERT(s.c_str());
    }

    reserves += entries;
}

template<class Key, class Data>
void ContentAddressableQueue<Key, Data>::unreserve(U32 entries)
{
    std::string s;

    if (entries > reserves)
    {
        s.clear();
        s.append("Exceeded previous reserves  : ");
        s.append(name);
        CG_ASSERT(s.c_str());
    }

    reserves -= entries;
}

template<class Key, class Data>
Data& ContentAddressableQueue<Key, Data>::head()
{
    U32 pos;

    pos = q.front();

    return contents[pos].second;
}

template<class Key, class Data>
bool ContentAddressableQueue<Key, Data>::empty() const
{
    return q.empty();
}

template<class Key, class Data>
U32 ContentAddressableQueue<Key, Data>::items() const
{
    return q.size();
}

template<class Key, class Data>
U32 ContentAddressableQueue<Key, Data>::free() const
{
    return (maxSize - (q.size() + reserves));
}

template<class Key, class Data>
U32 ContentAddressableQueue<Key, Data>::capacity() const
{
    return maxSize;
}

template<class Key, class Data>
bool ContentAddressableQueue<Key, Data>::full() const
{
    return ((q.size() + reserves) == maxSize);
}

template<class Key, class Data>
void ContentAddressableQueue<Key, Data>::clear()
{
    searchTable.clear();

    while ( !q.empty() )
    {
        freeList.push_back(q.front());
        q.pop();
    }
}

template<class Key, class Data>
void ContentAddressableQueue<Key, Data>::setName(std::string qName)
{
    name = qName;
}

template<class Key, class Data>
void ContentAddressableQueue<Key, Data>::dump() const
{
    std::queue<U32> qaux(q);
    cout << "{";
    while ( !qaux.empty() )
    {
        U32 i = qaux.front();
        cout << i << ",";
        qaux.pop();
    }
    cout << "}";
}

} // namespace arch::tools

} // namespace arch

#endif // GPU3D_TOOLS_QUEUE_H
