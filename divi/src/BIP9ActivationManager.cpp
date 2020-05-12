#include <BIP9ActivationManager.h>

#include <BIP9Deployment.h>
#include <algorithm>

BIP9ActivationManager::BIP9ActivationManager(
    ): thresholdCaches_()
    , bip9ActivationTrackers_()
    , knownBIPs_()
    , bitfieldOfBipsInUse_(0u)
{
    knownBIPs_.reserve(BIP9ActivationManager::MAXIMUM_SIMULTANEOUS_DEPLOYMENTS);
}

bool BIP9ActivationManager::networkEnabledBIP(std::string bipName) const
{
    return false;
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
        knownBIPs_.push_back(std::make_shared<BIP9Deployment>(bip));
        bitfieldOfBipsInUse_ |= bipMask;
    }
}