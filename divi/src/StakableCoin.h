#ifndef STAKABLE_COIN_H
#define STAKABLE_COIN_H
#include <merkletx.h>
#include <uint256.h>
struct StakableCoin
{
    const CMerkleTx* tx;
    unsigned outputIndex;
    uint256 blockHashOfFirstConfirmation;
    COutPoint utxo;

    StakableCoin(
        ): tx(nullptr)
        , outputIndex(0u)
        , blockHashOfFirstConfirmation(0u)
        , utxo(uint256(0),outputIndex)
    {
    }

    StakableCoin(
        const CMerkleTx* txIn,
        unsigned outputIndexIn,
        uint256 blockHashIn
        ): tx(txIn)
        , outputIndex(outputIndexIn)
        , blockHashOfFirstConfirmation(blockHashIn)
        , utxo( tx?tx->GetHash():uint256(0), outputIndex)
    {
    }
    bool operator<(const StakableCoin& other) const
    {
        return utxo < other.utxo;
    }
};
#endif//STAKABLE_COIN_H
