#!/usr/bin/env python3
# Copyright (c) 2020-2021 The DIVI developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Tests that nodes with support for custom reward addresses are compatible
# to nodes without it before the change actually activates.  For this,
# we run two masternodes (one with and one without support) and check
# that they can both see each other's masternode.
#
# We use five nodes:
# - node 0 is a masternode with default protocol (i.e. "new")
# - node 1 is a masternode with previous protocol (i.e. "old")
# - nodes 2-4 are just used to get above the "three full nodes" threshold

from test_framework import BitcoinTestFramework
from util import *
from masternode import *

import collections
import time

OLD_PROTOCOL = 70915
NEW_PROTOCOL = 71000


def args_for (n):
  base = ["-debug", "-nolistenonion", "-activeversion=%d" % OLD_PROTOCOL]
  if n == 1:
    base.extend (["-protocolversion=%d" % OLD_PROTOCOL])
  return base


class CustomAddressUpdateTest (BitcoinTestFramework):

  def setup_chain (self):
    for i in range (5):
      initialize_datadir (self.options.tmpdir, i)

  def setup_network (self, config_line=None, extra_args=[]):
    self.nodes = [
      start_node (i, self.options.tmpdir, extra_args=args_for (i))
      for i in range (5)
    ]

    # We want to work with mock times that are beyond the genesis
    # block timestamp but before current time (so that nodes being
    # started up and before they get on mocktime aren't rejecting
    # the on-disk blockchain).
    self.time = 1580000000
    assert self.time < time.time ()
    set_node_times (self.nodes, self.time)

    connect_nodes (self.nodes[0], 2)
    connect_nodes (self.nodes[1], 2)
    connect_nodes (self.nodes[2], 3)
    connect_nodes (self.nodes[2], 4)
    connect_nodes (self.nodes[3], 4)

    self.is_network_split = False

  def start_node (self, n):
    """Starts node n (0 or 1) with the proper arguments
    and masternode config for it."""

    assert n in [0, 1]
    configs = [[c.getLine ()] for c in self.cfg]

    args = args_for (n)
    args.append ("-masternode")
    args.append ("-masternodeprivkey=%s" % self.cfg[n].privkey)
    args.append ("-masternodeaddr=127.0.0.1:%d" % p2p_port (n))

    self.nodes[n] = start_node (n, self.options.tmpdir,
                                extra_args=args, mn_config_lines=configs[n])
    self.nodes[n].setmocktime (self.time)

    for i in [2, 3, 4]:
      connect_nodes (self.nodes[n], i)
    sync_blocks (self.nodes)

  def stop_node (self, n):
    """Stops node n."""

    stop_node (self.nodes[n], n)
    self.nodes[n] = None

  def advance_time (self, dt=1):
    """Advances mocktime by the given number of seconds."""

    self.time += dt
    set_node_times (self.nodes, self.time)

  def mine_blocks (self, n):
    """Mines blocks with node 2."""

    self.nodes[2].setgenerate(True, n)
    sync_blocks (self.nodes)

  def run_test (self):
    assert_equal (self.nodes[0].getnetworkinfo ()["protocolversion"], NEW_PROTOCOL)
    assert_equal (self.nodes[1].getnetworkinfo ()["protocolversion"], OLD_PROTOCOL)

    self.fund_masternodes ()
    self.start_masternodes ()

  def fund_masternodes (self):
    print ("Funding masternodes...")

    # The collateral needs 15 confirmations, and the masternode broadcast
    # signature must be later than that block's timestamp.  Thus we start
    # with a very early timestamp.
    genesis = self.nodes[0].getblockhash (0)
    genesisTime = self.nodes[0].getblockheader (genesis)["time"]
    assert genesisTime < self.time
    set_node_times (self.nodes, genesisTime)

    self.nodes[0].setgenerate (True, 1)
    sync_blocks (self.nodes)
    self.nodes[1].setgenerate (True, 1)
    sync_blocks (self.nodes)
    self.mine_blocks (25)
    assert_equal (self.nodes[0].getbalance (), 1250)
    assert_equal (self.nodes[1].getbalance (), 1250)

    id1 = self.nodes[0].allocatefunds ("masternode", "mn1", "copper")["txhash"]
    id2 = self.nodes[1].allocatefunds ("masternode", "mn2", "copper")["txhash"]
    sync_mempools (self.nodes)
    self.mine_blocks (15)
    set_node_times (self.nodes, self.time)
    self.mine_blocks (1)

    self.cfg = [
      fund_masternode (self.nodes[0], "mn1", "copper", id1, "localhost:%d" % p2p_port (0)),
      fund_masternode (self.nodes[1], "mn2", "copper", id2, "localhost:%d" % p2p_port (1)),
    ]

  def start_masternodes (self):
    print ("Starting masternodes...")

    for i in range (2):
      self.stop_node (i)
      self.start_node (i)

    # Start the masternodes after the nodes are back up and connected
    # (so they will receive each other's broadcast).
    for i in range (2):
      res = self.nodes[i].startmasternode ("mn%d" % (i + 1))
      assert_equal (res, {"status": "success"})

    # Check status of the masternodes themselves.
    for i in [0, 1]:
      data = self.nodes[i].getmasternodestatus ()
      assert_equal (data["status"], 4)
      assert_equal (data["txhash"], self.cfg[i].txid)
      assert_equal (data["outputidx"], self.cfg[i].vout)
      assert_equal (data["message"], "Masternode successfully started")

    # Both masternodes should see each other, independent of the
    # protocol version used.
    lists = [{}] * 2
    for i in range (2):
      cur = self.nodes[i].listmasternodes ()
      while len (cur) < 2:
        time.sleep (0.1)
        cur = self.nodes[i].listmasternodes ()
      for c in cur:
        lists[i][c["txhash"]] = c
    assert_equal (lists[0], lists[1])

    lst = lists[0]
    assert_equal (len (lst), 2)
    for val in lst.values ():
      assert_equal (val["tier"], "COPPER")
      assert_equal (val["status"], "ENABLED")


if __name__ == '__main__':
  CustomAddressUpdateTest ().main ()
