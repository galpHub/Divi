#ifndef NODE_H
#define NODE_H
#include <deque>
#include <string>
#include <compat.h>
#include <streams.h>
#include <sync.h>
#include <allocators.h>
#include <protocol.h>
#include <netbase.h>
#include <uint256.h>
#include <mruset.h>
#include <stdint.h>

#include <boost/signals2/signal.hpp>
#include <boost/thread/condition_variable.hpp>

class CBloomFilter;
typedef int NodeId;

/** The maximum number of entries in an 'inv' protocol message */
constexpr unsigned int MAX_INV_SZ = 50000;

enum NodeBufferStatus
{
    HAS_SPACE,
    IS_FULL,
    IS_OVERFLOWED,
};

// Signals for message handling
class CNode;
struct CNodeSignals {
    boost::signals2::signal<int()> GetHeight;
    boost::signals2::signal<bool(CNode*)> ProcessMessages;
    boost::signals2::signal<bool(CNode*, bool)> SendMessages;
    boost::signals2::signal<void(NodeId, const CNode*)> InitializeNode;
    boost::signals2::signal<void(NodeId)> FinalizeNode;
};

class CNodeStats
{
public:
    NodeId nodeid;
    uint64_t nServices;
    int64_t nLastSend;
    int64_t nLastRecv;
    int64_t nTimeConnected;
    std::string addrName;
    int nVersion;
    std::string cleanSubVer;
    bool fInbound;
    int nStartingHeight;
    uint64_t nSendBytes;
    uint64_t nRecvBytes;
    bool fWhitelisted;
    double dPingTime;
    double dPingWait;
    std::string addrLocal;
};

class CNetMessage
{
public:
    bool in_data; // parsing header (false) or data (true)

    CDataStream hdrbuf; // partially received header
    CMessageHeader hdr; // complete header
    unsigned int nHdrPos;
    CDataStream vRecv; // received message data
    unsigned int nDataPos;
    int64_t nTime; // time (in microseconds) of message receipt.

    CNetMessage(int nTypeIn, int nVersionIn);
    bool complete() const;
    void SetVersion(int nVersionIn);
    int readHeader(const char* pch, unsigned int nBytes);
    int readData(const char* pch, unsigned int nBytes);
};

class CNode
{
public:
    // socket
    uint64_t nServices;
    SOCKET hSocket;
    CDataStream ssSend;
    size_t nSendSize;   // total size of all vSendMsg entries
    size_t nSendOffset; // offset inside the first vSendMsg already sent
    uint64_t nSendBytes;
    std::deque<CSerializeData> vSendMsg;
    CCriticalSection cs_vSend;

    std::deque<CInv> vRecvGetData;
    std::deque<CNetMessage> vRecvMsg;
    CCriticalSection cs_vRecvMsg;
    uint64_t nRecvBytes;
    int nRecvVersion;

    int64_t nLastSend;
    int64_t nLastRecv;
    int64_t nTimeConnected;
    CAddress addr;
    std::string addrName;
    CService addrLocal;
    int nVersion;
    // strSubVer is whatever byte array we read from the wire. However, this field is intended
    // to be printed out, displayed to humans in various forms and so on. So we sanitize it and
    // store the sanitized version in cleanSubVer. The original should be used when dealing with
    // the network or wire types and the cleaned string used when displayed or logged.
    std::string strSubVer, cleanSubVer;
    bool fWhitelisted; // This peer can bypass DoS banning.
    bool fOneShot;
    bool fClient;
    bool fInbound;
    bool fNetworkNode;
    bool fSuccessfullyConnected;
    bool fDisconnect;
    // We use fRelayTxes for two purposes -
    // a) it allows us to not relay tx invs before receiving the peer's version message
    // b) the peer may tell us in their version message that we should not relay tx invs
    //    until they have initialized their bloom filter.
    bool fRelayTxes;
    // Should be 'true' only if we connected to this node to actually mix funds.
    // In this case node will be released automatically via CMasternodeMan::ProcessMasternodeConnections().
    // Connecting to verify connectability/status or connecting for sending/relaying single message
    // (even if it's relative to mixing e.g. for blinding) should NOT set this to 'true'.
    // For such cases node should be released manually (preferably right after corresponding code).
    bool fObfuScationMaster;
    CSemaphoreGrant grantOutbound;
    CCriticalSection cs_filter;
    CBloomFilter* pfilter;
    int nRefCount;
    NodeId id;

    int nSporksCount = -1;

protected:
    // Denial-of-service detection/prevention
    // Key is IP address, value is banned-until-time
    static std::map<CNetAddr, int64_t> setBanned;
    static CCriticalSection cs_setBanned;

    // Whitelisted ranges. Any node connecting from these is automatically
    // whitelisted (as well as those connecting to whitelisted binds).
    static std::vector<CSubNet> vWhitelistedRange;
    static CCriticalSection cs_vWhitelistedRange;

    // Basic fuzz-testing
    void Fuzz(int nChance); // modifies ssSend

    CNodeSignals* nodeSignals_;
public:
    uint256 hashContinue;
    int nStartingHeight;

    // flood relay
    std::vector<CAddress> vAddrToSend;
    mruset<CAddress> setAddrKnown;
    bool fGetAddr;
    std::set<uint256> setKnown;

    // inventory based relay
    mruset<CInv> setInventoryKnown;
    std::vector<CInv> vInventoryToSend;
    CCriticalSection cs_inventory;
    std::multimap<int64_t, CInv> mapAskFor;
    std::vector<uint256> vBlockRequested;

    // Ping time measurement:
    // The pong reply we're expecting, or 0 if no pong expected.
    uint64_t nPingNonceSent;
    // Time (in usec) the last ping was sent, or 0 if no ping was ever sent.
    int64_t nPingUsecStart;
    // Last measured round-trip time.
    int64_t nPingUsecTime;
    // Whether a ping is requested.
    bool fPingQueued;

    int nSporksSynced = 0;

    CNode(CNodeSignals* nodeSignals, SOCKET hSocketIn, CAddress addrIn, std::string addrNameIn = "", bool fInboundIn = false);
    ~CNode();

private:
    // Network usage totals
    static CCriticalSection cs_totalBytesRecv;
    static CCriticalSection cs_totalBytesSent;
    static uint64_t nTotalBytesRecv;
    static uint64_t nTotalBytesSent;


    // TODO: Document the postcondition of this function.  Is cs_vSend locked?
    void BeginMessage(const char* pszCommand) EXCLUSIVE_LOCK_FUNCTION(cs_vSend);
    // TODO: Document the precondition of this function.  Is cs_vSend locked?
    void AbortMessage() UNLOCK_FUNCTION(cs_vSend);
    // TODO: Document the precondition of this function.  Is cs_vSend locked?
    void EndMessage() UNLOCK_FUNCTION(cs_vSend);

    void PushMessageInternal()
    {
    }
    template <typename T1, typename ...Args>
    void PushMessageInternal(const T1& nextArgument, Args&&... args)
    {
        ssSend << nextArgument;
        PushMessageInternal(std::forward<Args>(args)...);
    }
public:
    template <typename ...Args>
    void PushMessage(const char* pszCommand, Args&&... args)
    {
        try {
            BeginMessage(pszCommand);
            PushMessageInternal(std::forward<Args>(args)...);
            EndMessage();
        } catch (...) {
            AbortMessage();
            throw;
        }
    }

    NodeBufferStatus GetSendBufferStatus() const;
    bool IsSelfConnection(uint64_t otherNonce) const;
    NodeId GetId() const;
    int GetRefCount();
    unsigned int GetTotalRecvSize();

    // requires LOCK(cs_vRecvMsg)
    bool ReceiveMsgBytes(const char* pch, unsigned int nBytes,boost::condition_variable& messageHandlerCondition);
    // requires LOCK(cs_vRecvMsg)
    void SetRecvVersion(int nVersionIn);
    CNode* AddRef();
    void Release();
    void AddAddressKnown(const CAddress& addr);
    void PushAddress(const CAddress& addr);
    void AddInventoryKnown(const CInv& inv);
    void PushInventory(const CInv& inv);
    void AskFor(const CInv& inv);
    void SocketSendData();

    void PushVersion();
    void SetSporkCount(int nSporkCountIn);
    bool AreSporksSynced() const;

    bool IsSubscribed(unsigned int nChannel);
    void Subscribe(unsigned int nChannel, unsigned int nHops = 0);
    void CancelSubscribe(unsigned int nChannel);
    void CloseSocketDisconnect();
    bool DisconnectOldProtocol(int nVersionRequired, std::string strLastCommand = "");

    /** Checks whether we want to send messages to this peer.  We do not
     *  send messages until we receive their version and get sporks.  */
    bool CanSendMessagesToPeer() const;

    /** Checks if we should send a ping message to this peer, and does it
     *  if we should.  */
    void MaybeSendPing();

    // Denial-of-service detection/prevention
    // The idea is to detect peers that are behaving
    // badly and disconnect/ban them, but do it in a
    // one-coding-mistake-won't-shatter-the-entire-network
    // way.
    // IMPORTANT:  There should be nothing I can give a
    // node that it will forward on that will make that
    // node's peers drop it. If there is, an attacker
    // can isolate a node and/or try to split the network.
    // Dropping a node for sending stuff that is invalid
    // now but might be valid in a later version is also
    // dangerous, because it can cause a network split
    // between nodes running old code and nodes running
    // new code.
    static void ClearBanned(); // needed for unit testing
    static std::string ListBanned();
    static bool IsBanned(CNetAddr ip);
    static bool Ban(const CNetAddr& ip);
    static bool Ban(const CNetAddr& addr, int64_t banTime);
    void copyStats(CNodeStats& stats);

    static bool IsWhitelistedRange(const CNetAddr& ip);
    static void AddWhitelistedRange(const CSubNet& subnet);

    // Network stats
    static void RecordBytesRecv(uint64_t bytes);
    static void RecordBytesSent(uint64_t bytes);

    static uint64_t GetTotalBytesRecv();
    static uint64_t GetTotalBytesSent();
};
#endif// NODE_H