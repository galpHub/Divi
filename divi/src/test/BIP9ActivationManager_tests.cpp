#include <test_only.h>
#include <BIP9ActivationManager.h>
#include <FakeBlockIndexChain.h>

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

BOOST_AUTO_TEST_SUITE_END()