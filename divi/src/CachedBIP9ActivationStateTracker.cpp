#include <CachedBIP9ActivationStateTracker.h>

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
        return ThresholdState::DEFINED;
    }
}
