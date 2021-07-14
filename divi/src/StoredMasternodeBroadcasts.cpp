#include "StoredMasternodeBroadcasts.h"

namespace
{

/** Magic string for stored broadcasts.  */
constexpr const char* MAGIC_BROADCAST = "mnBroadcast";

} // anonymous namespace

StoredMasternodeBroadcasts::StoredMasternodeBroadcasts(const std::string& file)
  : AppendOnlyFile(file)
{
  auto reader = Read ();
  if (reader != nullptr)
    {
      while (reader->Next())
        {
          const std::string magic = reader->GetMagic();
          if (magic != MAGIC_BROADCAST)
            {
              LogPrint("masternode", "Ignoring chunk '%s' in datafile", magic);
              continue;
            }

          CMasternodeBroadcast mnb;
          if (reader->Parse(mnb))
            broadcasts[mnb.vin.prevout] = mnb;
        }
    }
}

bool StoredMasternodeBroadcasts::AddBroadcast(const CMasternodeBroadcast& mnb)
{
  if (!Append(MAGIC_BROADCAST, mnb))
    return false;

  broadcasts[mnb.vin.prevout] = mnb;
  return true;
}

bool StoredMasternodeBroadcasts::GetBroadcast(const COutPoint& outp, CMasternodeBroadcast& mnb) const
{
  const auto mit = broadcasts.find(outp);
  if (mit == broadcasts.end())
    return false;

  mnb = mit->second;
  return true;
}
