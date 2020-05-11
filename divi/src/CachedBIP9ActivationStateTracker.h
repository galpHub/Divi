#ifndef CACHED_BIP9_ACTIVATION_STATE_TRACKER_H
#define CACHED_BIP9_ACTIVATION_STATE_TRACKER_H

#include <map>
#include <BIP9Deployment.h>
#include <I_BIP9ActivationStateTracker.h>
#include <vector>

class CBlockIndex;
typedef std::map<const CBlockIndex*, ThresholdState> ThresholdConditionCache;

class CachedBIP9ActivationStateTracker: public I_BIP9ActivationStateTracker
{
private:
    const BIP9Deployment& bip_;
    ThresholdConditionCache& thresholdCache_;
    const bool bipIsViable_;

    const CBlockIndex* getMostRecentStartingBlock(const CBlockIndex* shallowBlockIndex) const;
    void getStartingBlocksForPeriodsPreceedingBlockIndex(
        std::vector<const CBlockIndex*>& startingBlocksForPeriods
        ) const;
    bool enoughBipSignalsToLockIn(const CBlockIndex* uncachedStartingBlockIndex) const;
    void computeStateTransition(
        ThresholdState& lastKnownState,
        const CBlockIndex* previousBlockIndex) const;
public:
    CachedBIP9ActivationStateTracker(
        const BIP9Deployment& bip,
        ThresholdConditionCache& thresholdCache
        );
    virtual bool bipIsSignaledFor(const CBlockIndex* shallowBlockIndex) const;
    virtual bool update(const CBlockIndex* shallowBlockIndex);
    virtual ThresholdState getLastCachedStatePriorToBlockIndex(const CBlockIndex* shallowBlockIndex) const;
};

#endif // CACHED_BIP9_ACTIVATION_STATE_TRACKER_H