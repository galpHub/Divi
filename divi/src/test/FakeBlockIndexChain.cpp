#include <test/FakeBlockIndexChain.h>
#include <chain.h>
void FakeBlockIndexChain::resetFakeChain()
{
    for(CBlockIndex* ptr: fakeChain)
    {
        if(ptr) delete ptr;
    }
    fakeChain.clear();
}


FakeBlockIndexChain::FakeBlockIndexChain(): fakeChain()
{
}
FakeBlockIndexChain::~FakeBlockIndexChain()
{
    resetFakeChain();
}
void FakeBlockIndexChain::extend(
    unsigned maxHeight,
    int32_t time,
    int32_t version)
{
    fakeChain.reserve(maxHeight);
    extendFakeBlockIndexChain(maxHeight,time,version,fakeChain);
}
void FakeBlockIndexChain::extendFakeBlockIndexChain(
    unsigned height,
    int32_t time,
    int32_t version,
    std::vector<CBlockIndex*>& currentChain
    )
{
    while(currentChain.size() < height)
    {
        CBlockIndex* pindex = new CBlockIndex();
        pindex->nHeight = currentChain.size();
        pindex->pprev = currentChain.size() > 0 ? currentChain.back() : nullptr;
        pindex->nTime = time;
        pindex->nVersion = version;
        pindex->BuildSkip();
        currentChain.push_back(pindex);
    }
}

CBlockIndex* FakeBlockIndexChain::at(unsigned height) const
{
    return fakeChain[height];
}