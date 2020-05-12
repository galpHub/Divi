#include <BIP9ActivationManager.h>

#include <BIP9Deployment.h>
#include <algorithm>
#include <CachedBIP9ActivationStateTracker.h>
#include <ThresholdConditionCache.h>

BIP9ActivationManager::BIP9ActivationManager(
    ): thresholdCaches_()
    , bip9ActivationTrackers_()
    , knownBIPs_()
    , bitfieldOfBipsInUse_(0u)
    , bipIndexByName_()
    , chainTip_(NULL)
{
    knownBIPs_.reserve(BIP9ActivationManager::MAXIMUM_SIMULTANEOUS_DEPLOYMENTS);
}

bool BIP9ActivationManager::networkEnabledBIP(std::string bipName) const
{
    auto it = bipIndexByName_.find(bipName);
    if(it == bipIndexByName_.end()) return false;

    auto result = bip9ActivationTrackers_[it->second]->getLastCachedStatePriorToBlockIndex(chainTip_);
    return result == ThresholdState::ACTIVE;
}

BIP9ActivationManager::BIPStatus BIP9ActivationManager::getBIPStatus(std::string bipName) const
{
    auto it = std::find_if(knownBIPs_.begin(), knownBIPs_.end(), [&bipName](const std::shared_ptr<BIP9Deployment>& bip) { return bip->deploymentName == bipName;} );
    return (it==knownBIPs_.end())?BIP9ActivationManager::UNKNOWN_BIP : BIP9ActivationManager::IN_PROGRESS;
}

void BIP9ActivationManager::addBIP(const BIP9Deployment& bip)
{
    uint32_t bipMask = ((uint32_t)1 << bip.bit);
    if( (bipMask & bitfieldOfBipsInUse_) == 0)
    {
        bipIndexByName_.insert({bip.deploymentName, knownBIPs_.size()});

        knownBIPs_.push_back(std::make_shared<BIP9Deployment>(bip));
        bitfieldOfBipsInUse_ |= bipMask;

        thresholdCaches_.push_back(std::make_shared<ThresholdConditionCache>());
        bip9ActivationTrackers_.push_back(
            std::make_shared<CachedBIP9ActivationStateTracker>(
                *knownBIPs_.back(), 
                *thresholdCaches_.back()) );

    }
}

void BIP9ActivationManager::update(const CBlockIndex* nextBlockIndex)
{
    chainTip_ = nextBlockIndex;
    for(auto& trackerPtr: bip9ActivationTrackers_)
    {
        trackerPtr->update(nextBlockIndex);
    }
}