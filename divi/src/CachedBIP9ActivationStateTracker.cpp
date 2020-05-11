#include <CachedBIP9ActivationStateTracker.h>
#include <chain.h>

CachedBIP9ActivationStateTracker::CachedBIP9ActivationStateTracker(
    const BIP9Deployment& bip,
    ThresholdConditionCache& thresholdCache
    ): bip_(bip)
    , thresholdCache_(thresholdCache)
    , bipIsViable_( !(bip_.nStartTime > bip_.nTimeout || bip_.nPeriod < bip_.threshold) )
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

const CBlockIndex* CachedBIP9ActivationStateTracker::getMostRecentStartingBlock(const CBlockIndex* shallowBlockIndex) const
{
    if(shallowBlockIndex)
    {
        int heightOffset = (shallowBlockIndex->nHeight % bip_.nPeriod);
        return shallowBlockIndex->GetAncestor( shallowBlockIndex->nHeight - heightOffset);
    }
    return NULL;
}

void CachedBIP9ActivationStateTracker::computeStateTransition(
    ThresholdState& lastKnownState,
    const CBlockIndex* blockIndex) const
{
    switch(lastKnownState)
    {
        case ThresholdState::DEFINED:
            if(blockIndex->GetMedianTimePast() >= bip_.nTimeout)
            {
                lastKnownState = ThresholdState::FAILED;
            }
            else if(blockIndex->GetMedianTimePast() >= bip_.nStartTime)
            {
                lastKnownState = ThresholdState::STARTED;
            }
            else
            {
                lastKnownState = ThresholdState::DEFINED;
            }
            break;
        case ThresholdState::STARTED:
            if(blockIndex->GetMedianTimePast() >= bip_.nTimeout)
            {
                lastKnownState = ThresholdState::FAILED;
            }
            else if(enoughBipSignalsToLockIn(blockIndex))
            {
                lastKnownState = ThresholdState::LOCKED_IN;
            }
            else
            {
                lastKnownState = ThresholdState::STARTED;
            }
            break;
        case ThresholdState::LOCKED_IN:
            lastKnownState = ThresholdState::ACTIVE;
            break;
        case ThresholdState::FAILED:
        case ThresholdState::ACTIVE:
            break;
    }
}

bool CachedBIP9ActivationStateTracker::update(const CBlockIndex* shallowBlockIndex)
{
    if(bip_.nStartTime==BIP9Deployment::ALWAYS_ACTIVE) return true;

    shallowBlockIndex = getMostRecentStartingBlock(shallowBlockIndex);

    std::vector<const CBlockIndex*> startingBlocksForPeriods = {shallowBlockIndex};
    getStartingBlocksForPeriodsPreceedingBlockIndex(startingBlocksForPeriods);

    const CBlockIndex* lastBlockWithCachedState = startingBlocksForPeriods.back();
    startingBlocksForPeriods.pop_back();
    const CBlockIndex* blockIndexToCache = startingBlocksForPeriods.empty()? NULL: startingBlocksForPeriods.front();
    
    if(!blockIndexToCache) return true;

    ThresholdState lastKnownState = thresholdCache_[lastBlockWithCachedState];
    for(auto it = startingBlocksForPeriods.rbegin(); it != startingBlocksForPeriods.rend(); ++it)
    {
        if(!bipIsViable_)
        {
            thresholdCache_[*it] = ThresholdState::FAILED;
            continue;
        }
        computeStateTransition(lastKnownState, *it );
        thresholdCache_[*it] = lastKnownState;
    }
    return thresholdCache_.count(blockIndexToCache)!=0;
}

void CachedBIP9ActivationStateTracker::getStartingBlocksForPeriodsPreceedingBlockIndex(
    std::vector<const CBlockIndex*>& startingBlocksForPeriods) const
{
    const CBlockIndex* currentShallowBlockIndex = (startingBlocksForPeriods.empty())? NULL :startingBlocksForPeriods.back();
    if(currentShallowBlockIndex && !thresholdCache_.count(currentShallowBlockIndex))
    {
        const CBlockIndex* predecesor = currentShallowBlockIndex->GetAncestor(
            currentShallowBlockIndex->nHeight - bip_.nPeriod);
        
        startingBlocksForPeriods.push_back(predecesor);
        if(!startingBlocksForPeriods.back())
        { 
            thresholdCache_[startingBlocksForPeriods.back()] = ThresholdState::DEFINED;
            return;
        }
        if(predecesor->GetMedianTimePast() < bip_.nStartTime)
        { 
            if(thresholdCache_.count(startingBlocksForPeriods.back())) return;
            
            thresholdCache_[startingBlocksForPeriods.back()] = ThresholdState::DEFINED;
            return;
        }
        return getStartingBlocksForPeriodsPreceedingBlockIndex(startingBlocksForPeriods);
    }
}

ThresholdState CachedBIP9ActivationStateTracker::getLastCachedStatePriorToBlockIndex(const CBlockIndex* shallowBlockIndex) const
{
    if(bip_.nStartTime==BIP9Deployment::ALWAYS_ACTIVE) return ThresholdState::ACTIVE;
    if(!bipIsViable_)
    {
        return ThresholdState::FAILED;
    }
    else
    {   if(!shallowBlockIndex)
        {
            return ThresholdState::DEFINED;
        }
        
        shallowBlockIndex = getMostRecentStartingBlock(shallowBlockIndex);

        std::vector<const CBlockIndex*> startingBlocksForPeriods = {shallowBlockIndex};
        getStartingBlocksForPeriodsPreceedingBlockIndex(startingBlocksForPeriods);
        return thresholdCache_[startingBlocksForPeriods.back()];
    }
}
