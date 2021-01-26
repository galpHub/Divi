#ifndef STAKABLE_COIN_H
#define STAKABLE_COIN_H
#include <merkletx.h>
#include <uint256.h>
struct StakableCoin
{
    const CMerkleTx* tx;
    COutPoint utxo;
    uint256 blockHashOfFirstConfirmation;

    explicit StakableCoin(
        const CMerkleTx& txIn,
        const COutPoint& utxoIn,
        uint256 blockHashIn
        ): tx(&txIn)
        , utxo(utxoIn)
        , blockHashOfFirstConfirmation(blockHashIn)
    {
    }

    bool operator<(const StakableCoin& other) const
    {
        return utxo < other.utxo;
    }

    /** Convenience method to access the staked tx out.  */
    const CTxOut& GetTxOut() const
    {
        return tx->vout[utxo.n];
    }

};
#endif//STAKABLE_COIN_H
