// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txmempool.h"

#include "blockmap.h"
#include "MockUtxoHasher.h"

#include <boost/test/unit_test.hpp>
#include <list>

extern CChain chainActive;
extern BlockMap mapBlockIndex;

class MempoolTestFixture
{

private:

  /** Empty coins view used to back the cached view we actually use.  */
  CCoinsView emptyView;

  /** Tip of our fake chain.  */
  CBlockIndex tip;

protected:

  /** A parent transaction.  */
  CMutableTransaction txParent;

  /** Three children of the parent.  They use the bare txid for their UTXOs
   *  in our UTXO hasher.  */
  CMutableTransaction txChild[3];

  /** Three grand children.  */
  CMutableTransaction txGrandChild[3];

  /** Coins view with the parent inputs.  */
  CCoinsViewCache view;

  /** The test mempool instance.  */
  CTxMemPool testPool;

public:

  MempoolTestFixture()
    : view(&emptyView), testPool(CFeeRate(0))
  {
    std::unique_ptr<MockUtxoHasher> utxoHasher(new MockUtxoHasher());

    CMutableTransaction base;
    base.vout.emplace_back(99000LL, CScript() << OP_11 << OP_EQUAL);
    view.ModifyCoins(base.GetHash())->FromTx(base, 0);

    tip.pprev = nullptr;
    tip.nHeight = 0;
    mapBlockIndex[tip.GetBlockHeader().GetHash()] = &tip;
    view.SetBestBlock(tip.GetBlockHeader().GetHash());
    chainActive.SetTip(&tip);

    txParent.vin.resize(1);
    txParent.vin[0].scriptSig = CScript() << OP_11;
    txParent.vin[0].prevout.hash = base.GetHash();
    txParent.vin[0].prevout.n = 0;
    txParent.vout.resize(3);
    for (int i = 0; i < 3; i++)
    {
        txParent.vout[i].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        txParent.vout[i].nValue = 33000LL;
    }
    assert(txParent.GetHash() != txParent.GetBareTxid());

    for (int i = 0; i < 3; i++)
    {
        txChild[i].vin.resize(1);
        txChild[i].vin[0].scriptSig = CScript() << OP_11;
        txChild[i].vin[0].prevout.hash = txParent.GetHash();
        txChild[i].vin[0].prevout.n = i;
        txChild[i].vout.resize(1);
        txChild[i].vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        txChild[i].vout[0].nValue = 11000LL;
        utxoHasher->UseBareTxid(txChild[i]);
    }

    for (int i = 0; i < 3; i++)
    {
        txGrandChild[i].vin.resize(1);
        txGrandChild[i].vin[0].scriptSig = CScript() << OP_11;
        txGrandChild[i].vin[0].prevout.hash = txChild[i].GetBareTxid();
        txGrandChild[i].vin[0].prevout.n = 0;
        txGrandChild[i].vout.resize(1);
        txGrandChild[i].vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        txGrandChild[i].vout[0].nValue = 11000LL;
    }

    testPool.setSanityCheck(true);
    testPool.SetUtxoHasherForTesting(std::move(utxoHasher));
    testPool.clear();
  }

  ~MempoolTestFixture()
  {
    mapBlockIndex.clear();
  }

  /** Adds the parent, childs and grandchilds to the mempool.  */
  void AddAll()
  {
      testPool.addUnchecked(txParent.GetHash(), CTxMemPoolEntry(txParent, 0, 0, 0.0, 1));
      for (int i = 0; i < 3; i++)
      {
          testPool.addUnchecked(txChild[i].GetHash(), CTxMemPoolEntry(txChild[i], 0, 0, 0.0, 1));
          testPool.addUnchecked(txGrandChild[i].GetHash(), CTxMemPoolEntry(txGrandChild[i], 0, 0, 0.0, 1));
      }
  }

};

BOOST_FIXTURE_TEST_SUITE(mempool_tests, MempoolTestFixture)

BOOST_AUTO_TEST_CASE(MempoolRemoveTest)
{
    // Test CTxMemPool::remove functionality

    std::list<CTransaction> removed;

    // Nothing in pool, remove should do nothing:
    testPool.remove(txParent, removed, true);
    BOOST_CHECK_EQUAL(removed.size(), 0);

    // Just the parent:
    testPool.addUnchecked(txParent.GetHash(), CTxMemPoolEntry(txParent, 0, 0, 0.0, 1));
    testPool.remove(txParent, removed, true);
    BOOST_CHECK_EQUAL(removed.size(), 1);
    removed.clear();
    
    // Parent, children, grandchildren:
    AddAll();

    testPool.check(&view);

    // Remove Child[0], GrandChild[0] should be removed:
    testPool.remove(txChild[0], removed, true);
    BOOST_CHECK_EQUAL(removed.size(), 2);
    removed.clear();
    // ... make sure grandchild and child are gone:
    testPool.remove(txGrandChild[0], removed, true);
    BOOST_CHECK_EQUAL(removed.size(), 0);
    testPool.remove(txChild[0], removed, true);
    BOOST_CHECK_EQUAL(removed.size(), 0);
    // Remove parent, all children/grandchildren should go:
    testPool.remove(txParent, removed, true);
    BOOST_CHECK_EQUAL(removed.size(), 5);
    BOOST_CHECK_EQUAL(testPool.size(), 0);
    removed.clear();

    // Add children and grandchildren, but NOT the parent (simulate the parent being in a block)
    for (int i = 0; i < 3; i++)
    {
        testPool.addUnchecked(txChild[i].GetHash(), CTxMemPoolEntry(txChild[i], 0, 0, 0.0, 1));
        testPool.addUnchecked(txGrandChild[i].GetHash(), CTxMemPoolEntry(txGrandChild[i], 0, 0, 0.0, 1));
    }
    // Now remove the parent, as might happen if a block-re-org occurs but the parent cannot be
    // put into the mempool (maybe because it is non-standard):
    testPool.remove(txParent, removed, true);
    BOOST_CHECK_EQUAL(removed.size(), 6);
    BOOST_CHECK_EQUAL(testPool.size(), 0);
    removed.clear();
}

BOOST_AUTO_TEST_CASE(MempoolIndexByBareTxid)
{
    CTransaction tx;
    std::list<CTransaction> removed;

    AddAll();

    BOOST_CHECK(testPool.lookupBareTxid(txParent.GetBareTxid(), tx));
    BOOST_CHECK(tx.GetHash() == txParent.GetHash());
    BOOST_CHECK(!testPool.lookupBareTxid(txParent.GetHash(), tx));

    testPool.remove(txParent, removed, true);
    BOOST_CHECK(!testPool.lookupBareTxid(txParent.GetBareTxid(), tx));
    BOOST_CHECK(!testPool.lookupBareTxid(txChild[0].GetBareTxid(), tx));
    BOOST_CHECK(!testPool.lookupBareTxid(txGrandChild[0].GetBareTxid(), tx));
}

BOOST_AUTO_TEST_CASE(MempoolOutpointLookup)
{
    CTransaction tx;
    CCoins coins;

    AddAll();
    CCoinsViewMemPool viewPool(&view, testPool);

    BOOST_CHECK(testPool.lookupOutpoint(txParent.GetHash(), tx));
    BOOST_CHECK(!testPool.lookupOutpoint(txParent.GetBareTxid(), tx));
    BOOST_CHECK(!testPool.lookupOutpoint(txChild[0].GetHash(), tx));
    BOOST_CHECK(testPool.lookupOutpoint(txChild[0].GetBareTxid(), tx));

    BOOST_CHECK(viewPool.HaveCoins(txParent.GetHash()));
    BOOST_CHECK(viewPool.GetCoins(txParent.GetHash(), coins));
    BOOST_CHECK(!viewPool.HaveCoins(txParent.GetBareTxid()));
    BOOST_CHECK(!viewPool.GetCoins(txParent.GetBareTxid(), coins));

    BOOST_CHECK(!viewPool.HaveCoins(txChild[0].GetHash()));
    BOOST_CHECK(!viewPool.GetCoins(txChild[0].GetHash(), coins));
    BOOST_CHECK(viewPool.HaveCoins(txChild[0].GetBareTxid()));
    BOOST_CHECK(viewPool.GetCoins(txChild[0].GetBareTxid(), coins));
}

BOOST_AUTO_TEST_SUITE_END()
