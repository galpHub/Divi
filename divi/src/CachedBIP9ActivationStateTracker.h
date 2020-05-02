#ifndef CACHED_BIP9_ACTIVATION_STATE_TRACKER_H
#define CACHED_BIP9_ACTIVATION_STATE_TRACKER_H

#include <map>
#include <BIP9Deployment.h>
#include <I_BIP9ActivationStateTracker.h>

class CBlockIndex;
typedef std::map<const CBlockIndex*, ThresholdState> ThresholdConditionCache;

class CachedBIP9ActivationStateTracker: public I_BIP9ActivationStateTracker
{
private:
    const BIP9Deployment& bip_;
    ThresholdConditionCache& thresholdCache_;
protected:
    virtual bool bipIsSignaledFor(const CBlockIndex* shallowBlockIndex) const;
public:
    CachedBIP9ActivationStateTracker(
        const BIP9Deployment& bip,
        ThresholdConditionCache& thresholdCache
        );
    virtual bool update(const CBlockIndex* shallowBlockIndex);
    virtual ThresholdState getStateAtBlockIndex(const CBlockIndex* shallowBlockIndex) const;
};

#endif // CACHED_BIP9_ACTIVATION_STATE_TRACKER_H