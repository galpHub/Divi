#ifndef BIP9_ACTIVATION_MANAGER_H
#define BIP9_ACTIVATION_MANAGER_H

#include <vector>
#include <memory>

class I_BIP9ActivationStateTracker;
struct ThresholdConditionCache;
struct BIP9Deployment;
class CBlockIndex;


class BIP9ActivationManager
{
private:
    static constexpr unsigned MAXIMUM_SIMULTANEOUS_DEPLOYMENTS = 29;
    std::vector<std::shared_ptr<ThresholdConditionCache>> thresholdCaches_;
    std::vector<std::shared_ptr<I_BIP9ActivationStateTracker>> bip9ActivationTrackers_;

public:
    enum BIPStatus
    {
        UNKNOWN_BIP,
        IN_PROGRESS
    };

    BIP9ActivationManager();
    bool networkEnabledBIP(std::string bipName) const;
    BIPStatus getBIPStatus(std::string bipName) const;
};

#endif// BIP9_ACTIVATION_MANAGER_H