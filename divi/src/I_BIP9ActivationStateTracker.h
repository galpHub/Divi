#ifndef I_BIP9_ACTIVATION_STATE_TRACKER_H
#define I_BIP9_ACTIVATION_STATE_TRACKER_H


class CBlockIndex;
enum class ThresholdState;

class I_BIP9ActivationStateTracker
{
public:
    virtual bool bipIsSignaledFor(const CBlockIndex* shallowBlockIndex) const = 0;
    virtual bool update(const CBlockIndex* shallowBlockIndex) = 0;
    virtual ThresholdState getStateAtBlockIndex(const CBlockIndex* shallowBlockIndex) const = 0;
};

#endif // I_BIP9_ACTIVATION_STATE_TRACKER_H