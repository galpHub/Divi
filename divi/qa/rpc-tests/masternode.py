# Copyright (c) 2020 The DIVI developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Utilities for testing masternodes.

from util import *


class MnConfigLine (object):
  """Wrapper class for working with masternode.conf lines."""

  def __init__ (self, line):
    parts = line.split (" ")
    assert len (parts) in [5, 6]

    self.line = line

    self.alias = parts[0]
    self.ip = parts[1]
    self.privkey = parts[2]
    self.txid = parts[3]
    self.vout = int (parts[4])

    if len (parts) >= 6:
      self.rewardAddr = parts[5]
    else:
      self.rewardAddr = None

  def getLine (self):
    """Returns the config line as string, to put into masternode.conf."""

    res = "%s %s %s %s %d" % (self.alias, self.ip, self.privkey,
                              self.txid, self.vout)
    if self.rewardAddr is not None:
      res += " %s" % self.rewardAddr

    return res


def fund_masternode (node, alias, tier, txid, ip):
  """Calls fundmasternode with the given data and returns the
  MnConfigLine instance."""

  data = node.fundmasternode (alias, tier, txid, ip)
  assert "config line" in data
  return MnConfigLine (data["config line"])

class MnSetupData (object):
  """Wrapper class for working with setting up remote masternodes"""
  def __init__(self, setup_data):
    assert_equal(len(setup_data), 4)

    self.protocol_version = setup_data["protocol_version"]
    self.message_to_sign = setup_data["message_to_sign"]
    self.cfg = MnConfigLine(setup_data["config_line"])
    self.broadcast_data = setup_data["broadcast_data"]

def setup_masternode (node, conf_data,pubkey):
  """Calls fundmasternode with the given data and returns the
  MnConfigLine instance."""
  data = node.setupmasternode (
    conf_data.alias,
    conf_data.txid,
    str(conf_data.vout),
    pubkey,
    conf_data.ip)
  return MnSetupData (data)
