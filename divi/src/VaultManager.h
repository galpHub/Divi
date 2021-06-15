#ifndef VAULT_MANAGER_H
#define VAULT_MANAGER_H
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <sync.h>

class CScript;
using ManagedScripts = std::map<CScript,unsigned>;
class COutPoint;
using UnspentOutputs = std::set<COutPoint>;
class CTransaction;
class CBlock;
class CWalletTx;
class TransactionUtxoHasher;
class uint256;

class I_VaultManagerDatabase;
class WalletTransactionRecord;
class SpentOutputTracker;
class CChain;
class BlockMap;
class VaultManager
{
private:
    const CChain& activeChain_;
    const BlockMap& blockIndicesByHash_;
    mutable CCriticalSection cs_vaultManager_;
    uint64_t transactionOrderingIndex_;
    const TransactionUtxoHasher& utxoHasher_;
    std::unique_ptr<WalletTransactionRecord> walletTxRecord_;
    std::unique_ptr<SpentOutputTracker> outputTracker_;
    ManagedScripts managedScriptsLimits_;
public:
    VaultManager(
        const CChain& activeChain,
        const BlockMap& blockIndicesByHash,
        const TransactionUtxoHasher& utxoHasher);
    VaultManager(
        const CChain& activeChain,
        const BlockMap& blockIndicesByHash,
        const TransactionUtxoHasher& utxoHasher,
        I_VaultManagerDatabase& vaultManagerDB);
    ~VaultManager();
    void SyncTransaction(const CTransaction& tx, const CBlock *pblock);
    void addManagedScript(const CScript& script, unsigned limit);
    UnspentOutputs getUTXOs() const;
    const CWalletTx& GetTransaction(const uint256&) const;
    const ManagedScripts& GetManagedScriptLimits() const;
};
#endif// VAULT_MANAGER_H
