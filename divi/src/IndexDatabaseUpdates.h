#ifndef INDEX_DATABASE_UPDATES_H
#define INDEX_DATABASE_UPDATES_H
#include <vector>
#include <utility>
#include <addressindex.h>
#include <OutputHash.h>
#include <spentindex.h>
#include <uint256.h>

class CTransaction;
class TransactionUtxoHasher;

/** One entry in the tx index, which locates transactions on disk by their txid
 *  or bare txid (both keys are possible).  */
struct TxIndexEntry
{
    uint256 txid;
    uint256 bareTxid;
    CDiskTxPos diskPos;

    explicit TxIndexEntry(const uint256& t, const uint256& b, const CDiskTxPos& p)
      : txid(t), bareTxid(b), diskPos(p)
    {}
};

struct IndexDatabaseUpdates
{
    std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > addressUnspentIndex;
    std::vector<std::pair<CSpentIndexKey, CSpentIndexValue> > spentIndex;
    std::vector<TxIndexEntry> txLocationData;

    IndexDatabaseUpdates();
};

struct TransactionLocationReference
{
    uint256 hash;
    OutputHash utxoHash;
    unsigned blockHeight;
    int transactionIndex;

    TransactionLocationReference(
        const TransactionUtxoHasher& utxoHasher,
        const CTransaction& tx,
        unsigned blockheightValue,
        int transactionIndexValue);
};
#endif// INDEX_DATABASE_UPDATES_H
