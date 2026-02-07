/**************************************************************************
 *
 */

#ifndef BANKSELECTIONPOLICY_H
    #define BANKSELECTIONPOLICY_H

#include "GPUType.h"
#include <vector>
#include <string>

namespace cg1gpu {
namespace memorycontroller {

class BankSelectionPolicy;

/**
 * Factory intended to create Bank SelectionPolicy objects based on a textual description
 *
 * @code
 *     BankSelectionPolicy* bsp = bspFactory->create("MORE_CONSECUTIVE_HITS OLDEST_FIRST RANDOM");
 * @endcode
 *
 * This call would create a bank selection policy that utilizes firstly the More Consetuve Hits policy
 * to select a request and in case of a tie, would utilize the Oldest First criteria and in a new
 * tie would utilize a random policy among the canditates filtered by the two previous policies
 */
class BankSelectionPolicyFactory
{
public:

    /**
     * Bank selection policies supported:
     *
     * "RANDOM" : selects a random bank
     * "ROUND_ROBIN" : sorts banks in a round robin fashion each time a BankSelectionPolicy::sortBanks() is called
     * "OLDEST_FIRST" : sorts banks in an oldest first fashion (based on age)
     * "YOUNGEST_FIRST" : sorts banks in an youngest first fashion (based on age)
     * "MORE_CONSECUTIVE_HITS" : sorts banks using the consecutive requests expected to each bank (banks with more first)
     * "LESS_CONSECUTIVE_HITS" : sorts banks using the consecutive requests expected to each bank (banks with less first)
     * "MORE_PENDING_REQUESTS" : sorts banks based on the number of pending requests ( aka queue size), more requests first
     * "LESS_PENDING_REQUESTS" : sorts banks based on the number of pending requests ( aka queue size), less requests first
     * "ZERO_PENDING_FIRST" : classifies banks between banks without pending reqs and banks with pending reqs (first banks without pending requests)
     */
    static BankSelectionPolicy* create(const std::string& definition, U32 banks);

}; // class BankSelectionPolicyFactory



/**
 * Class responsible of manage a complex bank selection policy combining basic bank selection policies
 */
class BankSelectionPolicy
{
public:

    /**
     * This struct can be augmented with more info required by future bank selection policies 
     */
    struct BankInfo
    {
        U32 bankID;
        U64 age;
        U32 queueSize;
        U32 consecHits;
    };

    typedef std::vector<BankInfo*> BankInfoArray;

    /**
     * Implement your policy extending from that object
     */
    class BankInfoComparator 
    {
    public:

        /**
         * This method will be called every time just before sorting the banks
         * It is helful for setting up internal state required by BankInfoComparators
         */
        virtual void update() { /* empty by default*/  }

        /**
         * Method automatically called that decides what bank has more priority based on the comparison being implemented
         *
         * Expected values: 
         *    -1 if 'a' has more priority than 'b
         *     0 if both have the same priority
         *     1 if 'b' has more priority than 'a'
         */
        virtual S32 compare(const BankInfo* a, const BankInfo* b) = 0;
        
    };

    /**
     * Add bank compare policies. Each new added policy is used as a tie-breaker in case the
     * previous policy cannot determine which bank has to be selected
     */
    void addPolicy(BankInfoComparator* bic);

    void sortBanks(BankInfoArray& bia);

    static void debug_printBanks(const BankInfoArray& bia);

    BankSelectionPolicy();

private:

    BankSelectionPolicy(const BankSelectionPolicy&);
    BankSelectionPolicy& operator=(const BankSelectionPolicy&);

    class BankCompareObject
    {
    private:
        std::vector<BankInfoComparator*> policies;
    public:
        void update();
        void addPolicy(BankInfoComparator* bankComparator);
        bool operator()(const BankInfo* a, const BankInfo* b);
        bool ready() const;
    };
    BankCompareObject bankCompareObject;

}; // class BankSelectionPolicy


///////////////////////////////////////////////////////////////////////////
////////////////// BankInfoComparator definitions /////////////////////////
///////////////////////////////////////////////////////////////////////////

class RandomComparator : public BankSelectionPolicy::BankInfoComparator
{
public:

    RandomComparator(U32 banks);
    S32 compare(const BankSelectionPolicy::BankInfo* a, const BankSelectionPolicy::BankInfo* b);
    void update();

private:

    RandomComparator(const RandomComparator&);
    RandomComparator& operator=(const RandomComparator&);

    const U32 Banks;
    std::vector<U32> randomWeights;

}; // class RandomComparator

class RoundRobinComparator : public BankSelectionPolicy::BankInfoComparator
{
public:

    RoundRobinComparator(U32 banks);

    S32 compare(const BankSelectionPolicy::BankInfo* a, const BankSelectionPolicy::BankInfo* b);
    void update();

private:

    RoundRobinComparator(const RoundRobinComparator&);
    RoundRobinComparator& operator=(const RoundRobinComparator&);

    const U32 Banks;
    U32 nextRR;
}; // class RoundRobinComparator


class AgeComparator : public BankSelectionPolicy::BankInfoComparator
{
public:

    enum ComparatorType
    {
        OLDEST_FIRST,
        YOUNGEST_FIRST
    };

    AgeComparator(ComparatorType type);
    S32 compare(const BankSelectionPolicy::BankInfo* a, const BankSelectionPolicy::BankInfo* b);

private:

    ComparatorType compType;
}; // class AgeComparator

class ConsecutiveHitsComparator : public BankSelectionPolicy::BankInfoComparator
{
public:

    enum ComparatorType
    {
        LESS_CONSECUTIVE_HITS_FIRST,
        MORE_CONSECUTIVE_HITS_FIRST
    };

    ConsecutiveHitsComparator(ComparatorType type);
    S32 compare(const BankSelectionPolicy::BankInfo* a, const BankSelectionPolicy::BankInfo* b);

private:

    ComparatorType compType;
    
}; // class ConsecutiveHitsComparator


class QueueSizeComparator : public BankSelectionPolicy::BankInfoComparator
{
public:

    enum ComparatorType
    {
        LESS_PENDING_REQUESTS,
        MORE_PENDING_REQUESTS,
        ZERO_PENDING_FIRST,
    };

    QueueSizeComparator(ComparatorType type);

    S32 compare(const BankSelectionPolicy::BankInfo* a, const BankSelectionPolicy::BankInfo* b);

private:

    ComparatorType compType;
};

} // namespace memorycontroller
} // namespace cg1gpu


#endif // BANKSELECTIONPOLICY_H
