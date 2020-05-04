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
    return false;
}
bool CachedBIP9ActivationStateTracker::update(const CBlockIndex* shallowBlockIndex)
{
    return false;
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
        if(predecesor->GetMedianTimePast() < bip_.nStartTime) return;
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
