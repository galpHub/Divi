#include <test_only.h>
#include <BIP9ActivationManager.h>


BOOST_AUTO_TEST_SUITE(BIP9ActivationManager_tests)

BOOST_AUTO_TEST_CASE(willHaveNoBIPsEnabledByDefaultOnConstruction)
{
    BIP9ActivationManager manager;
    BOOST_CHECK(!manager.networkEnabledBIP("SEGWIT"));
}

BOOST_AUTO_TEST_SUITE_END()