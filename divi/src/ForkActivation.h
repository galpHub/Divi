// Copyright (c) 2020 The DIVI Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef FORK_ACTIVATION_H
#define FORK_ACTIVATION_H

#include <cstdint>

class CBlockHeader;
class CBlockIndex;

/**
 * The list of consensus changes ("forks") that have been introduced
 * on the network since its launch and may need to be activated
 * (with conditional logic in other parts of the codebase depending
 * on whether or not a fork is active).
 */
enum Fork
{
  HardenedStakeModifier,
  UniformLotteryWinners,

  /**
   * Start of "segwit light":  The UTXOs created by transactions from after
   * the fork will be indexed by a "bare txid", which does not include
   * any signature data.  This fixes transaction malleability.
   */
  SegwitLight,

  /* Test forks not actually deployed / active but used for unit tests.  */
  TestByTimestamp,
};

/**
 * Activation state of forks.  Each instance corresponds to a particular block,
 * and can be used to query the state of each supported fork as it should be
 * for validating this block.
 */
class ActivationState
{

private:

  /** The timestamp of the block this is associated to.  */
  const int64_t nTime;

public:

  explicit ActivationState(const CBlockIndex* pi);
  explicit ActivationState(const CBlockHeader& block);

  ActivationState(ActivationState&&) = default;

  ActivationState() = delete;
  ActivationState(const ActivationState&) = delete;
  void operator=(const ActivationState&) = delete;

  /**
   * Returns true if the indicated fork should be considered active
   * for processing the associated block.
   */
  bool IsActive(Fork f) const;

  /**
   * Returns true if the current time is "close" to the activation of
   * segwit-light (before or after), with close being within the given
   * number of seconds.  This is used for a temporary measure to disallow
   * (by mempool and wallet policy) spending of unconfirmed change
   * around the fork.
   */
  static bool CloseToSegwitLight(int maxSeconds);

};

#endif // FORK_ACTIVATION_H
