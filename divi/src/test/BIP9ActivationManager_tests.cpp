#include <test_only.h>
#include <BIP9ActivationManager.h>
#include <FakeBlockIndexChain.h>
#include <BIP9Deployment.h>

BOOST_AUTO_TEST_SUITE(BIP9ActivationManager_tests)

BOOST_AUTO_TEST_CASE(willHaveNoBIPsEnabledByDefaultOnConstruction)
{
    BIP9ActivationManager manager;
    BOOST_CHECK(!manager.networkEnabledBIP("SEGWIT"));
}

BOOST_AUTO_TEST_CASE(willKnowOfNoBIPsByDefault)
{
    BIP9ActivationManager manager;
    BOOST_CHECK(manager.getBIPStatus("SEGWIT") == BIP9ActivationManager::UNKNOWN_BIP);
}

BOOST_AUTO_TEST_CASE(willRecognizeAnAddedBIP)
{
    BIP9Deployment bip("MySegwitVariant", 1, (int64_t)1500000,(int64_t)1600000,1000,900);
    BIP9ActivationManager manager;
    BOOST_CHECK(manager.getBIPStatus(bip.deploymentName) == BIP9ActivationManager::UNKNOWN_BIP);
    manager.addBIP(bip);
    BOOST_CHECK(manager.getBIPStatus(bip.deploymentName) == BIP9ActivationManager::IN_PROGRESS);
}

BOOST_AUTO_TEST_CASE(willNotAllowAddingBIPsWithOverlappingBits)
{
    BIP9Deployment first("MySegwitVariant", 1, (int64_t)1500000,(int64_t)1600000,1000,900);
    BIP9Deployment second("MyOtherSegwitVariant", 1, (int64_t)1500000,(int64_t)1600000,1000,900);
    
    BIP9ActivationManager manager;
    manager.addBIP(first);
    BOOST_CHECK(manager.getBIPStatus(first.deploymentName) == BIP9ActivationManager::IN_PROGRESS);
    
    manager.addBIP(second);
    BOOST_CHECK(manager.getBIPStatus(second.deploymentName) == BIP9ActivationManager::UNKNOWN_BIP);
}

BOOST_AUTO_TEST_CASE(willEnableBIPIfChainMeetsSignalingThreshold)
{
    BIP9Deployment firstBIP("MySegwitVariant", 1, (int64_t)1500000,(int64_t)1600000,1000,900);
    uint32_t bipMask = ( (int32_t)1 << firstBIP.bit);
    FakeBlockIndexChain fakeChain;   
    fakeChain.extendBy(firstBIP.nPeriod, firstBIP.nStartTime - 1, 0); // Stays In Defined
    fakeChain.extendBy(firstBIP.nPeriod, firstBIP.nStartTime, 0); // Moves to started
    fakeChain.extendBy(firstBIP.nPeriod, firstBIP.nStartTime,  VERSIONBITS_TOP_BITS | bipMask ); // Moves To LOCKED_IN
    fakeChain.extendBy(firstBIP.nPeriod+1, firstBIP.nStartTime,  0); // Moves To ACTIVE

    BIP9ActivationManager manager;

    manager.addBIP(firstBIP);
    manager.update(fakeChain.tip());

    BOOST_CHECK(manager.networkEnabledBIP(firstBIP.deploymentName));
}

BOOST_AUTO_TEST_SUITE_END()