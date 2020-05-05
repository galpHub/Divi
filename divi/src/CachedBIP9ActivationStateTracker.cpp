#include <CachedBIP9ActivationStateTracker.h>
#include <chain.h>

CachedBIP9ActivationStateTracker::CachedBIP9ActivationStateTracker(
    const BIP9Deployment& bip,
    ThresholdConditionCache& thresholdCache
    ): bip_(bip), thresholdCache_(thresholdCache) 
{
}

bool CachedBIP9ActivationStateTracker::bipIsSignaledFor(const CBlockIndex* shallowBlockIndex) const
{
    static int32_t bipMask = ((int32_t)1 << bip_.bit);
    return (shallowBlockIndex->nVersion & VERSIONBITS_TOP_MASK) == VERSIONBITS_TOP_BITS &&
        (shallowBlockIndex->nVersion & bipMask ) != 0;
}
bool CachedBIP9ActivationStateTracker::enoughBipSignalsToLockIn(const CBlockIndex* uncachedStartingBlockIndex) const
{
    int blocksCounted =0;
    int count = 0u;
    while(uncachedStartingBlockIndex && uncachedStartingBlockIndex->pprev && blocksCounted < bip_.nPeriod)
    {
        count += bipIsSignaledFor(uncachedStartingBlockIndex->pprev);
        uncachedStartingBlockIndex = uncachedStartingBlockIndex->pprev;
        blocksCounted++;
    }
    return count >= bip_.threshold;
}

bool CachedBIP9ActivationStateTracker::update(const CBlockIndex* shallowBlockIndex)
{
    std::vector<const CBlockIndex*> startingBlocksForPeriods;
    if(shallowBlockIndex && (shallowBlockIndex->nHeight % bip_.nPeriod )==0) startingBlocksForPeriods.push_back(shallowBlockIndex);
    getStartingBlocksForPeriodsPreceedingBlockIndex(shallowBlockIndex,startingBlocksForPeriods);

    if(startingBlocksForPeriods.empty() ||
        (startingBlocksForPeriods.back() && 
        startingBlocksForPeriods.back()->nHeight>0 && 
        !thresholdCache_.count(startingBlocksForPeriods.back()) )
        )
    {
        return false;
    }

    const CBlockIndex* lastBlockWithCachedState = startingBlocksForPeriods.back();
    startingBlocksForPeriods.pop_back();
    bool stateTransitionOccurred = false;

    ThresholdState lastKnownState = thresholdCache_[lastBlockWithCachedState];
    for(auto it = startingBlocksForPeriods.rbegin(); it != startingBlocksForPeriods.rend(); ++it)
    {
        switch(lastKnownState)
        {
            case ThresholdState::DEFINED:
                if((*it)->nTime >= bip_.nTimeout)
                {
                    thresholdCache_[*it] = ThresholdState::FAILED;
                    stateTransitionOccurred |= true;
                }
                else if((*it)->nTime >= bip_.nStartTime)
                {
                    thresholdCache_[*it] = ThresholdState::STARTED;
                    stateTransitionOccurred |= true;
                }
                break;
            case ThresholdState::STARTED:
                if((*it)->nTime >= bip_.nTimeout)
                {
                    thresholdCache_[*it] = ThresholdState::FAILED;
                    stateTransitionOccurred |= true;
                }
                else if(enoughBipSignalsToLockIn(*it))
                {
                    thresholdCache_[*it] = ThresholdState::LOCKED_IN;
                    stateTransitionOccurred |= true;
                }
                
                break;
            case ThresholdState::LOCKED_IN:
                thresholdCache_[*it] = ThresholdState::ACTIVE;
                stateTransitionOccurred |= true;
                break;
            default:
                thresholdCache_[*it] = lastKnownState;
                stateTransitionOccurred |= false;
                break;
        }
        lastKnownState = thresholdCache_[*it];
    }
    return stateTransitionOccurred;
}

void CachedBIP9ActivationStateTracker::getStartingBlocksForPeriodsPreceedingBlockIndex(
    const CBlockIndex* currentShallowBlockIndex,
    std::vector<const CBlockIndex*>& startingBlocksForPeriods) const
{
    if(!thresholdCache_.count(currentShallowBlockIndex) && currentShallowBlockIndex)
    {
        int predecesorHeight = currentShallowBlockIndex->nHeight - (currentShallowBlockIndex->nHeight % bip_.nPeriod);
        predecesorHeight -= (predecesorHeight == currentShallowBlockIndex->nHeight)? bip_.nPeriod: 0;
        const CBlockIndex* predecesor = currentShallowBlockIndex->GetAncestor(predecesorHeight);
        startingBlocksForPeriods.push_back(predecesor);
        return getStartingBlocksForPeriodsPreceedingBlockIndex(predecesor,startingBlocksForPeriods);
    }
}

ThresholdState CachedBIP9ActivationStateTracker::getStateAtBlockIndex(const CBlockIndex* shallowBlockIndex) const
{
    if(bip_.nStartTime > bip_.nTimeout ||
        bip_.nPeriod < bip_.threshold)
    {
        return ThresholdState::FAILED;
    }
    else
    {
        if(thresholdCache_.count(shallowBlockIndex))
        {
            return thresholdCache_[shallowBlockIndex];
        }
        std::vector<const CBlockIndex*> startingBlocksForPeriods;
        getStartingBlocksForPeriodsPreceedingBlockIndex(
            shallowBlockIndex,
            startingBlocksForPeriods);
        if(!startingBlocksForPeriods.empty() && startingBlocksForPeriods.back()!=NULL)
        {
            return thresholdCache_[startingBlocksForPeriods.back()];
        }
        return ThresholdState::DEFINED;
    }
}
