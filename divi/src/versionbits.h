// Copyright (c) 2016-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
// Copyright (c) 2020 The Divi Core developers 
#ifndef VERSIONBITS_H
#define VERSIONBITS_H

#include <map>
#include <limits>

class CBlockIndex;

/** What block version to use for new blocks (pre versionbits) */
static const int32_t VERSIONBITS_LAST_OLD_BLOCK_VERSION = 4;
/** What bits to set in version for versionbits blocks */
static const int32_t VERSIONBITS_TOP_BITS = 0x20000000UL;
/** What bitmask determines whether versionbits is in use */
static const int32_t VERSIONBITS_TOP_MASK = 0xE0000000UL;
/** Total bits available for versionbits */
static const int32_t VERSIONBITS_NUM_BITS = 29;

/** BIP 9 defines a finite-state-machine to deploy a softfork in multiple stages.
 *  State transitions happen during retarget period if conditions are met
 *  In case of reorg, transitions can go backward. Without transition, state is
 *  inherited between periods. All blocks of a period share the same state.
 */
enum class ThresholdState {
    DEFINED,   // First state that each softfork starts out as. The genesis block is by definition in this state for each deployment.
    STARTED,   // For blocks past the starttime.
    LOCKED_IN, // For one retarget period after the first retarget period with STARTED blocks of which at least threshold have the associated bit set in nVersion.
    ACTIVE,    // For all blocks after the LOCKED_IN retarget period (final state)
    FAILED,    // For all blocks once the first retarget period after the timeout time is hit, if LOCKED_IN wasn't already reached (final state)
};

struct BIP9Deployment 
{
    /** Name of deployment for later caching*/
    const std::string deploymentName;
    /** Bit position to select the particular bit in nVersion. */
    const int bit;
    /** Start MedianTime for version bits miner confirmation. Can be a date in the past */
    const int64_t nStartTime;
    /** Timeout/expiry MedianTime for the deployment attempt. */
    const int64_t nTimeout;
    // Number of blocks to look back & number of blocks needed
    const int nPeriod;
    const int threshold;

    mutable ThresholdState state = ThresholdState::DEFINED;

    BIP9Deployment(
        ): deploymentName("invalid")
        , bit(0)
        , nStartTime(1)
        , nTimeout(0)
        , nPeriod(0)
        , threshold(1)
    {
    }

    BIP9Deployment(
        std::string name,
        unsigned bitIndex, 
        int64_t startTime, 
        int64_t timeout,
        int blockPeriod,
        int blockThreshold
        ): deploymentName(name)
        , bit(static_cast<int>(bitIndex))
        , nStartTime(startTime)
        , nTimeout(timeout)
        , nPeriod(blockPeriod)
        , threshold(blockThreshold)
    {
    }

    BIP9Deployment& operator=(const BIP9Deployment& other)
    {
        *const_cast<std::string*>(&deploymentName)=other.deploymentName;
        *const_cast<int*>(&bit)=other.bit;
        *const_cast<int64_t*>(&nStartTime)=other.nStartTime;
        *const_cast<int64_t*>(&nTimeout)=other.nTimeout;
        *const_cast<int*>(&nPeriod)=other.nPeriod;
        *const_cast<int*>(&threshold)=other.threshold;
        return *this;
    }

    void setState(ThresholdState updatedState) const
    {
        state = updatedState;
    }

    /** Constant for nTimeout very far in the future. */
    static constexpr int64_t NO_TIMEOUT = std::numeric_limits<int64_t>::max();

    /** Special value for nStartTime indicating that the deployment is always active.
     *  This is useful for testing, as it means tests don't need to deal with the activation
     *  process (which takes at least 3 BIP9 intervals). Only tests that specifically test the
     *  behaviour during activation cannot use this. */
    static constexpr int64_t ALWAYS_ACTIVE = -1;
    static constexpr int MAX_VERSION_BITS_DEPLOYMENTS = 1;
};



// A map that gives the state for blocks whose height is a multiple of Period().
// The map is indexed by the block's parent, however, so all keys in the map
// will either be nullptr or a block with (height + 1) % Period() == 0.
typedef std::map<const CBlockIndex*, ThresholdState> ThresholdConditionCache;

/** Display status of an in-progress BIP9 softfork */
struct BIP9Stats {
    /** Length of blocks of the BIP9 signalling period */
    int period;
    /** Number of blocks with the version bit set required to activate the softfork */
    int threshold;
    /** Number of blocks elapsed since the beginning of the current period */
    int elapsed;
    /** Number of blocks with the version bit set since the beginning of the current period */
    int count;
    /** False if there are not enough blocks left in this period to pass activation threshold */
    bool possible;
};

/**
 * Abstract class that implements BIP9-style threshold logic, and caches results.
 */
class AbstractThresholdConditionChecker {
protected:
    const BIP9Deployment& bip_;
    virtual bool Condition(const CBlockIndex* pindex) const =0;

public:
    explicit AbstractThresholdConditionChecker(const BIP9Deployment& bip): bip_(bip){}
    AbstractThresholdConditionChecker& operator=(AbstractThresholdConditionChecker&& other)
    {
        const_cast<BIP9Deployment&>(bip_) = other.bip_;
        return *this;
    }
    /** Returns the numerical statistics of an in-progress BIP9 softfork in the current period */
    BIP9Stats GetStateStatisticsFor(const CBlockIndex* pindex) const;
    /** Returns the state for pindex A based on parent pindexPrev B. Applies any state transition if conditions are present.
     *  Caches state from first block of period. */
    ThresholdState UpdateCacheState(const CBlockIndex* pindexPrev, ThresholdConditionCache& cache) const;
    /** Returns the height since when the ThresholdState has started for pindex A based on parent pindexPrev B, all blocks of a period share the same */
    int GetStateSinceHeightFor(const CBlockIndex* pindexPrev, ThresholdConditionCache& cache) const;
};

/** BIP 9 allows multiple softforks to be deployed in parallel. We cache per-period state for every one of them
 *  keyed by the bit position used to signal support. */
struct VersionBitsCache
{
    ThresholdConditionCache caches[BIP9Deployment::MAX_VERSION_BITS_DEPLOYMENTS];
    void Clear();
};

ThresholdState VersionBitsState(const CBlockIndex* pindexPrev, const BIP9Deployment& bip9Deployment, VersionBitsCache& cache);
BIP9Stats VersionBitsStatistics(const CBlockIndex* pindexPrev, const BIP9Deployment& bip9Deployment);
int VersionBitsStateSinceHeight(const CBlockIndex* pindexPrev, const BIP9Deployment& bip9Deployment, VersionBitsCache& cache);
uint32_t VersionBitsMask(const BIP9Deployment& bip9Deployment);

#endif // VERSIONBITS_H
