#ifndef INDEX_DATABASE_UPDATES_H
#define INDEX_DATABASE_UPDATES_H
#include <vector>
#include <utility>
#include <addressindex.h>
#include <spentindex.h>
#include <uint256.h>

class CTransaction;
class TransactionUtxoHasher;

struct IndexDatabaseUpdates
{
    std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > addressUnspentIndex;
    std::vector<std::pair<CSpentIndexKey, CSpentIndexValue> > spentIndex;
    std::vector<std::pair<uint256, CDiskTxPos> > txLocationData;

    IndexDatabaseUpdates();
};

struct TransactionLocationReference
{
    uint256 hash;
    uint256 utxoHash;
    unsigned blockHeight;
    int transactionIndex;

    TransactionLocationReference(
        const TransactionUtxoHasher& utxoHasher,
        const CTransaction& tx,
        unsigned blockheightValue,
        int transactionIndexValue);
};
#endif// INDEX_DATABASE_UPDATES_H
