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

BOOST_AUTO_TEST_SUITE_END()