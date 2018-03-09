/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
 *               2010      NICTA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: 
 */

#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/wifi-net-device.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/yans-error-rate-model.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/test.h"
#include "ns3/pointer.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/config.h"
#include "ns3/string.h"
#include "ns3/packet-socket-server.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"

using namespace ns3;

class WifiMihLinkSapTest : public TestCase
{
public:
  WifiMihLinkSapTest ();

  virtual void DoRun (void);
};

WifiMihLinkSapTest::WifiMihLinkSapTest ()
  : TestCase ("WifiMihLinkSapTest")
{
}


void
WifiMihLinkSapTest::DoRun ()
{
  Ptr<WifiNetDevice> dev = CreateObject<WifiNetDevice> ();
  ObjectFactory m_mac;
  Ptr<StaWifiMac> mac = m_mac.Create<StaWifiMac> ();
  mac->ConfigureStandard (WIFI_PHY_STANDARD_80211a);
  mac->SetAddress (Mac48Address::Allocate ());
  dev->SetMac (mac);
  
  mac->SetState (StaWifiMac::WAIT_ASSOC_RESP);

  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ASSOCIATION_RESPONSE);
  hdr.SetAddr1 (MAC->GetAddress ());
  hdr.SetAddr2 (Mac48Address ("00:00:00:00:00:92"));

  hdr.SetDsFrom ();
  hdr.SetDsTo ();
  // Fill QoS fields:
  hdr.SetQosAckPolicy (WifiMacHeader::NORMAL_ACK);
  hdr.SetQosNoEosp ();
  hdr.SetQosNoAmsdu ();
  hdr.SetQosTxopLimit (0);

  Ptr<Packet> pckt = Create<Packet> (500);
  MgtAssocResponseHeader assocResp;
  StatusCode status = StatusCode ();
  status.SetSuccess ();
  assocResp.SetStatusCode (status);
  pckt->AddHeader (assocResp);

  mac->Receive (pckt,&hdr);
}


class WifiMihLinkSapTestSuite : public TestSuite
{
public:
  WifiMihLinkSapTestSuite ();
};

WifiMihLinkSapTestSuite::WifiMihLinkSapTestSuite ()
  : TestSuite ("wifi-mih-limk-sap", UNIT)
{
  AddTestCase (new WifiMihLinkSapTest, TestCase::QUICK);
}

static WifiMihLinkSapTestSuite g_WifiMihLinkSapTestSuite; ///< the test suite
