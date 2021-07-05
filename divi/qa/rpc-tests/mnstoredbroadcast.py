#!/usr/bin/env python3
# Copyright (c) 2021 The DIVI developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Tests handling of pre-signed, stored masternode broadcasts.
#
# This test uses two nodes:  Node 0 is used for mining and funding
# the masternode (as the cold node) and node 1 is the actual masternode
# with empty wallet and imported broadcast (the hot node).

from test_framework import BitcoinTestFramework
from util import *
from masternode import *


class MnStoredBroadcastTest (BitcoinTestFramework):

  def setup_network (self, config_line=None):
    args = [["-spendzeroconfchange"]] * 2
    config_lines = [[]] * 2

    if config_line:
      config_lines = [[config_line.line]] * 2
      args[1].append ("-masternode")
      args[1].append ("-masternodeprivkey=%s" % config_line.privkey)

    self.nodes = start_nodes(2, self.options.tmpdir, extra_args=args, mn_config_lines=config_lines)
    connect_nodes_bi (self.nodes, 0, 1)
    self.is_network_split = False
    self.sync_all ()

  def run_test (self):
    print ("Funding masternode...")
    self.nodes[0].setgenerate (True, 30)
    txid = self.nodes[0].allocatefunds ("masternode", "mn", "copper")["txhash"]
    self.nodes[0].setgenerate (True, 1)
    cfg = fund_masternode (self.nodes[0], "mn", "copper", txid, "1.2.3.4")

    print ("Updating masternode.conf...")
    stop_nodes (self.nodes)
    wait_bitcoinds ()
    self.setup_network (config_line=cfg)

    print ("Preparing the masternode broadcast...")
    mnb = self.nodes[0].startmasternode ("mn", True)
    assert_equal (mnb["status"], "success")
    mnb = mnb["broadcastData"]

    print ("Importing broadcast data...")
    assert_raises (JSONRPCException,
                   self.nodes[1].importmnbroadcast, "invalid")
    assert_equal (self.nodes[1].importmnbroadcast (mnb), True)
    assert_equal (self.nodes[1].importmnbroadcast (mnb), True)
    assert_equal (self.nodes[0].listmnbroadcasts (), [])
    assert_equal (self.nodes[1].listmnbroadcasts (), [
      {
        "txhash": cfg.txid,
        "outidx": cfg.vout,
        "broadcast": mnb,
      }
    ])

    print ("Restarting node...")
    stop_nodes (self.nodes)
    wait_bitcoinds ()
    self.setup_network (config_line=cfg)

    print ("Testing imported data...")
    assert_equal (self.nodes[1].listmnbroadcasts (), [
      {
        "txhash": cfg.txid,
        "outidx": cfg.vout,
        "broadcast": mnb,
      }
    ])


if __name__ == '__main__':
  MnStoredBroadcastTest ().main ()
