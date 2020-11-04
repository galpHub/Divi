#include <LotteryWinnersCalculator.h>

#include <SuperblockHelpers.h>
#include <hash.h>
#include <uint256.h>
#include <primitives/transaction.h>
#include <chain.h>
#include <timedata.h>
#include <numeric>
#include <spork.h>
#include <BlockDiskAccessor.h>
#include <I_SuperblockHeightValidator.h>


LotteryWinnersCalculator::LotteryWinnersCalculator(
    int startOfLotteryBlocks,
    CChain& activeChain,
    CSporkManager& sporkManager,
    const I_SuperblockHeightValidator& superblockHeightValidator
    ): startOfLotteryBlocks_(startOfLotteryBlocks)
    , activeChain_(activeChain)
    , sporkManager_(sporkManager)
    , superblockHeightValidator_(superblockHeightValidator)
{
}

int LotteryWinnersCalculator::minimumCoinstakeForTicket(int nHeight) const
{
    int nMinStakeValue = 10000; // default is 10k

    if(sporkManager_.IsSporkActive(SPORK_16_LOTTERY_TICKET_MIN_VALUE)) {
        MultiValueSporkList<LotteryTicketMinValueSporkValue> vValues;
        CSporkManager::ConvertMultiValueSporkVector(sporkManager_.GetMultiValueSpork(SPORK_16_LOTTERY_TICKET_MIN_VALUE), vValues);
        auto nBlockTime = activeChain_[nHeight] ? activeChain_[nHeight]->nTime : GetAdjustedTime();
        LotteryTicketMinValueSporkValue activeSpork = CSporkManager::GetActiveMultiValueSpork(vValues, nHeight, nBlockTime);

        if(activeSpork.IsValid()) {
            // we expect that this value is in coins, not in satoshis
            nMinStakeValue = activeSpork.nEntryTicketValue;
        }
    }

    return nMinStakeValue;
}

uint256 LotteryWinnersCalculator::CalculateLotteryScore(const uint256 &hashCoinbaseTx, const uint256 &hashLastLotteryBlock) const
{
    // Deterministically calculate a "score" for a Masternode based on any given (block)hash
    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << hashCoinbaseTx << hashLastLotteryBlock;
    return ss.GetHash();
}

bool LotteryWinnersCalculator::IsCoinstakeValidForLottery(const CTransaction &tx, int nHeight) const
{
    CAmount nAmount = 0;
    if(tx.IsCoinBase()) {
        nAmount = tx.vout[0].nValue;
    }
    else {
        auto payee = tx.vout[1].scriptPubKey;
        nAmount = std::accumulate(std::begin(tx.vout), std::end(tx.vout), CAmount(0), [payee](CAmount accum, const CTxOut &out) {
                return out.scriptPubKey == payee ? accum + out.nValue : accum;
    });
    }

    return nAmount > minimumCoinstakeForTicket(nHeight) * COIN; // only if stake is more than 10k
}

uint256 LotteryWinnersCalculator::GetLastLotteryBlockHashBeforeHeight(int blockHeight) const
{
    const int lotteryBlockPaymentCycle = superblockHeightValidator_.GetLotteryBlockPaymentCycle(blockHeight);
    const int nLastLotteryHeight = std::max(startOfLotteryBlocks_,  lotteryBlockPaymentCycle* ((blockHeight - 1) / lotteryBlockPaymentCycle) );
    return activeChain_[nLastLotteryHeight]->GetBlockHash();
}

LotteryCoinstakeData LotteryWinnersCalculator::CalculateUpdatedLotteryWinners(
    const CTransaction& coinMintTransaction,
    const LotteryCoinstakeData& previousBlockLotteryCoinstakeData,
    int nHeight) const
{
    if(nHeight <= 0) return LotteryCoinstakeData();
    if(superblockHeightValidator_.IsValidLotteryBlockHeight(nHeight)) return LotteryCoinstakeData(nHeight);
    if(nHeight <= startOfLotteryBlocks_) return previousBlockLotteryCoinstakeData.getShallowCopy();
    if(!IsCoinstakeValidForLottery(coinMintTransaction, nHeight)) return previousBlockLotteryCoinstakeData.getShallowCopy();

    auto hashLastLotteryBlock = GetLastLotteryBlockHashBeforeHeight(nHeight);

    // lotteryWinnersCoinstakes has hashes of coinstakes, let calculate old scores + new score
    using LotteryScore = uint256;
    std::vector<LotteryScore> scores;
    LotteryCoinstakes coinstakes = previousBlockLotteryCoinstakeData.getLotteryCoinstakes();
    scores.reserve(coinstakes.size()+1);
    int startingWinnerIndex = 0;
    std::map<uint256,int> initialLotteryRanksByTransactionHash;
    for(auto&& lotteryCoinstake : coinstakes) {
        initialLotteryRanksByTransactionHash[lotteryCoinstake.first] = startingWinnerIndex++;
        scores.emplace_back(CalculateLotteryScore(lotteryCoinstake.first, hashLastLotteryBlock));
    }

    auto newScore = CalculateLotteryScore(coinMintTransaction.GetHash(), hashLastLotteryBlock);
    scores.emplace_back(newScore);
    initialLotteryRanksByTransactionHash[coinMintTransaction.GetHash()] = startingWinnerIndex++;

    coinstakes.reserve(coinstakes.size()+1);
    coinstakes.emplace_back(coinMintTransaction.GetHash(), coinMintTransaction.IsCoinBase()? coinMintTransaction.vout[0].scriptPubKey:coinMintTransaction.vout[1].scriptPubKey);

    // biggest entry at the begining
    if(scores.size() > 1)
    {
        std::stable_sort(std::begin(coinstakes), std::end(coinstakes),
            [&scores,&initialLotteryRanksByTransactionHash](const LotteryCoinstake& lhs, const LotteryCoinstake& rhs)
            {
                return scores[initialLotteryRanksByTransactionHash[lhs.first]] > scores[initialLotteryRanksByTransactionHash[rhs.first]];
            }
        );
    }
    bool shouldUpdateCoinstakeData = (coinstakes.size()>0)? initialLotteryRanksByTransactionHash[coinstakes.back().first] != 11 : false;
    coinstakes.resize( std::min( std::size_t(11), coinstakes.size()) );
    if(shouldUpdateCoinstakeData)
    {
        return LotteryCoinstakeData(nHeight,coinstakes);
    }
    else
    {
        return previousBlockLotteryCoinstakeData.getShallowCopy();
    }
}