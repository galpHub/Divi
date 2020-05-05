#include <test_only.h>

#include <CachedBIP9ActivationStateTracker.h>
#include <chain.h>
#include <test/FakeBlockIndexChain.h>

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

BOOST_AUTO_TEST_CASE(willFindStateInCacheIfPresent)
{
    BIP9Deployment bip = createViableBipDeployment();
    ThresholdConditionCache cache;
    std::shared_ptr<CBlockIndex> blockIndexPtr = std::make_shared<CBlockIndex>();
    cache[blockIndexPtr.get()] = ThresholdState::ACTIVE;

    CachedBIP9ActivationStateTracker activationStateTracker(bip,cache);

    BOOST_CHECK(activationStateTracker.getStateAtBlockIndex(blockIndexPtr.get())==ThresholdState::ACTIVE);
}

BOOST_AUTO_TEST_CASE(willDeferToCachedStateAtApropriateHeight)
{
    
    BIP9Deployment bip = createViableBipDeployment();
    ThresholdConditionCache cache;
    FakeBlockIndexChain fakeChain;
    int fakeChainSize = 2*bip.nPeriod;
    fakeChain.extend(fakeChainSize, 0, 0);
    cache[fakeChain.at(bip.nPeriod)] = ThresholdState::ACTIVE;

    CachedBIP9ActivationStateTracker activationStateTracker(bip,cache);

    for(int height = 0; height < bip.nPeriod; height++)
    {
        BOOST_CHECK_MESSAGE(
            activationStateTracker
                .getStateAtBlockIndex(fakeChain.at(height))==ThresholdState::DEFINED,
            "The height is" << height);
    }
    for(int height = bip.nPeriod; height < fakeChainSize; height++)
    {
        BOOST_CHECK_MESSAGE(
            activationStateTracker
                .getStateAtBlockIndex(fakeChain.at(height))==ThresholdState::ACTIVE,
            "The height is" << height);
    }
}

BOOST_AUTO_TEST_CASE(willGetEarliestCachedState)
{
    BIP9Deployment bip = createViableBipDeployment();
    ThresholdConditionCache cache;
    FakeBlockIndexChain fakeChain;
    int fakeChainSize = 2*bip.nPeriod;
    fakeChain.extend(fakeChainSize, 0, 0);

    CachedBIP9ActivationStateTracker activationStateTracker(bip,cache);
    BOOST_CHECK(
        activationStateTracker
            .getStateAtBlockIndex(fakeChain.at(fakeChainSize-1))==ThresholdState::DEFINED);
}

BOOST_AUTO_TEST_CASE(willDetectBlockSignalsForBip)
{
    BIP9Deployment bip = createViableBipDeployment();
    ThresholdConditionCache cache;
    FakeBlockIndexChain fakeChain;
    CachedBIP9ActivationStateTracker activationStateTracker(bip,cache);

    int32_t bipMask = ( (int32_t)1 << bip.bit);


    fakeChain.extend(1, 0, VERSIONBITS_TOP_BITS );
    fakeChain.extend(1, 0,  VERSIONBITS_TOP_BITS | bipMask );
    fakeChain.extend(1, 0, VERSIONBITS_TOP_BITS | ( bipMask << 1 ) );
    
    BOOST_CHECK(!activationStateTracker.bipIsSignaledFor(fakeChain.at(0)));
    BOOST_CHECK(activationStateTracker.bipIsSignaledFor(fakeChain.at(1)));
    BOOST_CHECK(!activationStateTracker.bipIsSignaledFor(fakeChain.at(2)));
}


BOOST_AUTO_TEST_CASE(willClaimToUpdateIfBlocksMeetThresholdAndPriorStateIsStarted)
{
    {
        BIP9Deployment bip = createViableBipDeployment();
        ThresholdConditionCache cache;
        FakeBlockIndexChain fakeChain;
        fakeChain.extend(bip.threshold, 0, VERSIONBITS_TOP_BITS | (1 << bip.bit) );
        fakeChain.extend(bip.nPeriod+1 - bip.threshold, 0, 0);
        cache[fakeChain.at(0)] = ThresholdState::STARTED;

        CachedBIP9ActivationStateTracker activationStateTracker(bip,cache);

        BOOST_CHECK(activationStateTracker.update(fakeChain.at(bip.nPeriod)));
        BOOST_CHECK(activationStateTracker.getStateAtBlockIndex(fakeChain.at(bip.nPeriod))==ThresholdState::LOCKED_IN);
    }
    {
        BIP9Deployment bip = createViableBipDeployment();
        ThresholdConditionCache cache;
        FakeBlockIndexChain fakeChain;
        fakeChain.extend(bip.threshold-1, 0, VERSIONBITS_TOP_BITS | (1 << bip.bit) );
        fakeChain.extend(bip.nPeriod+2 - bip.threshold, 0, 0);
        cache[fakeChain.at(0)] = ThresholdState::STARTED;

        CachedBIP9ActivationStateTracker activationStateTracker(bip,cache);

        BOOST_CHECK(!activationStateTracker.update(fakeChain.at(bip.nPeriod)));
        BOOST_CHECK(activationStateTracker.getStateAtBlockIndex(fakeChain.at(bip.nPeriod))!=ThresholdState::STARTED);
    }
    {
        BIP9Deployment bip = createViableBipDeployment();
        ThresholdConditionCache cache;
        FakeBlockIndexChain fakeChain;
        fakeChain.extend(bip.threshold, 0, VERSIONBITS_TOP_BITS | (1 << bip.bit) );
        fakeChain.extend(bip.nPeriod+1 - bip.threshold, 0, 0);
        cache[fakeChain.at(0)] = ThresholdState::DEFINED;

        CachedBIP9ActivationStateTracker activationStateTracker(bip,cache);

        BOOST_CHECK(!activationStateTracker.update(fakeChain.at(bip.nPeriod)));
        BOOST_CHECK(activationStateTracker.getStateAtBlockIndex(fakeChain.at(bip.nPeriod))!=ThresholdState::LOCKED_IN);
    }
}

BOOST_AUTO_TEST_CASE(willTransitionFromDefinedStateToStartedStateOnlyIfBlockTimeIsAtLeastStartTime)
{
    {
        BIP9Deployment bip = createViableBipDeployment();
        ThresholdConditionCache cache;
        FakeBlockIndexChain fakeChain;
        fakeChain.extend(bip.nPeriod+1, bip.nStartTime - 1, 1 );
        cache[fakeChain.at(0)] = ThresholdState::DEFINED;

        CachedBIP9ActivationStateTracker activationStateTracker(bip,cache);

        BOOST_CHECK(!activationStateTracker.update(fakeChain.at(bip.nPeriod)));
        BOOST_CHECK(activationStateTracker.getStateAtBlockIndex(fakeChain.at(bip.nPeriod))!=ThresholdState::STARTED);
    }
    {
        BIP9Deployment bip = createViableBipDeployment();
        ThresholdConditionCache cache;
        FakeBlockIndexChain fakeChain;
        fakeChain.extend(bip.nPeriod, bip.nStartTime - 1, 1 );
        fakeChain.extend(1, bip.nStartTime, 1 );
        cache[fakeChain.at(0)] = ThresholdState::DEFINED;

        CachedBIP9ActivationStateTracker activationStateTracker(bip,cache);

        BOOST_CHECK(activationStateTracker.update(fakeChain.at(bip.nPeriod)));
        BOOST_CHECK(activationStateTracker.getStateAtBlockIndex(fakeChain.at(bip.nPeriod))==ThresholdState::STARTED);
    }
}

BOOST_AUTO_TEST_CASE(willTransitionFromDefinedOrStartedStateToFailedStateIfBlockTimeIsAtLeastTimeout)
{
    {
        BIP9Deployment bip = createViableBipDeployment();
        ThresholdConditionCache cache;
        FakeBlockIndexChain fakeChain;
        fakeChain.extend(bip.nPeriod, bip.nStartTime - 1, 1 );
        fakeChain.extend(1, bip.nTimeout, 1 );
        cache[fakeChain.at(0)] = ThresholdState::DEFINED;

        CachedBIP9ActivationStateTracker activationStateTracker(bip,cache);

        BOOST_CHECK(activationStateTracker.update(fakeChain.at(bip.nPeriod)));
        BOOST_CHECK(activationStateTracker.getStateAtBlockIndex(fakeChain.at(bip.nPeriod))==ThresholdState::FAILED);
    }    
    {
        BIP9Deployment bip = createViableBipDeployment();
        ThresholdConditionCache cache;
        FakeBlockIndexChain fakeChain;
        fakeChain.extend(bip.nPeriod, bip.nStartTime - 1, 1 );
        fakeChain.extend(1, bip.nTimeout, 1 );
        cache[fakeChain.at(0)] = ThresholdState::STARTED;

        CachedBIP9ActivationStateTracker activationStateTracker(bip,cache);

        BOOST_CHECK(activationStateTracker.update(fakeChain.at(bip.nPeriod)));
        BOOST_CHECK(activationStateTracker.getStateAtBlockIndex(fakeChain.at(bip.nPeriod))==ThresholdState::FAILED);
    }
}

BOOST_AUTO_TEST_SUITE_END();
