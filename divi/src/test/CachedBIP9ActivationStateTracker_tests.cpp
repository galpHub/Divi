#include <test_only.h>

#include <CachedBIP9ActivationStateTracker.h>

BOOST_AUTO_TEST_SUITE(CachedBIP9ActivationStateTracker_tests)

BOOST_AUTO_TEST_CASE(initial_test)
{
    BIP9Deployment bip;
    ThresholdConditionCache cache;
    CachedBIP9ActivationStateTracker activationStateTracker(bip,cache);
}

BOOST_AUTO_TEST_SUITE_END();
