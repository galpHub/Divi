#include <test_only.h>

#include <CachedBIP9ActivationStateTracker.h>

static inline BIP9Deployment createViableBipDeployment()
{
    return BIP9Deployment(
            "ViableTestBIP9",
            0u,
            (int64_t)1588600341,
            (int64_t)1598600341,
            1000,
            900
            );
}
static inline BIP9Deployment createTimedOutBipDeployment()
{
    return BIP9Deployment(
        "ViableTestBIP9",
        0u,
        (int64_t)1598600341,
        (int64_t)1588600341,
        1000,
        900
        );
}
static inline BIP9Deployment createBipDeploymentWithPeriodLessThanThreshold()
{
    return BIP9Deployment(
        "ViableTestBIP9",
        0u,
        (int64_t)1598600341,
        (int64_t)1588600341,
        900,
        1000
        );
}

BOOST_AUTO_TEST_SUITE(CachedBIP9ActivationStateTracker_tests)

BOOST_AUTO_TEST_CASE(initial_test)
{
    BIP9Deployment bip;
    ThresholdConditionCache cache;
    CachedBIP9ActivationStateTracker activationStateTracker(bip,cache);
}
BOOST_AUTO_TEST_CASE(willFindStateToBeDefinedByDefaultIfBipIsViable)
{
    {
        BIP9Deployment bip;
        ThresholdConditionCache cache;
        CachedBIP9ActivationStateTracker activationStateTracker(bip,cache);
        BOOST_CHECK(activationStateTracker.getStateAtBlockIndex(NULL)==ThresholdState::FAILED);
    }
    {
        BIP9Deployment bip = createTimedOutBipDeployment();
        ThresholdConditionCache cache;
        CachedBIP9ActivationStateTracker activationStateTracker(bip,cache);
        BOOST_CHECK(activationStateTracker.getStateAtBlockIndex(NULL)==ThresholdState::FAILED);
    }
    {
        BIP9Deployment bip = createBipDeploymentWithPeriodLessThanThreshold();
        ThresholdConditionCache cache;
        CachedBIP9ActivationStateTracker activationStateTracker(bip,cache);
        BOOST_CHECK(activationStateTracker.getStateAtBlockIndex(NULL)==ThresholdState::FAILED);
    }
    {
        BIP9Deployment bip = createViableBipDeployment();
        ThresholdConditionCache cache;
        CachedBIP9ActivationStateTracker activationStateTracker(bip,cache);
        BOOST_CHECK(activationStateTracker.getStateAtBlockIndex(NULL)==ThresholdState::DEFINED);
    }
}
BOOST_AUTO_TEST_SUITE_END();
