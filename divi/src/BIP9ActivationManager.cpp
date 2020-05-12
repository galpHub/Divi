#include <BIP9ActivationManager.h>

#include <BIP9Deployment.h>

static BIP9Deployment CurrentBIP9Deployments[1] =
{
    BIP9Deployment()
};


BIP9ActivationManager::BIP9ActivationManager(
    ): thresholdCaches_()
    , bip9ActivationTrackers_()
{
}

bool BIP9ActivationManager::networkEnabledBIP(std::string bipName) const
{
    return false;
}