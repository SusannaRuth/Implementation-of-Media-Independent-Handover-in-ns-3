/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/mih-module.h"

// Default Network Topology
//
//   Wifi 10.1.3.0
//         AP
//    *    *
//    |    |    
//   n1   n0   
//                   
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiMihLinkDownExample");

static void
SetPosition (Ptr<Node> node, double x)
{
  Ptr<MobilityModel> mobility = node ->GetObject<MobilityModel> ();
  Vector pos = mobility ->GetPosition ();
  pos.x = x;
  mobility ->SetPosition (pos);
}
int 
main (int argc, char *argv[])
{
  bool verbose = true;
  uint32_t nWifi = 1;
  bool tracing = false;

  CommandLine cmd;
  //cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);

  cmd.Parse (argc,argv);

  // The underlying restriction of 18 is due to the grid position
  // allocator's configuration; the grid layout will exceed the
  // bounding box if more than 18 nodes are provided.
  /*if (nWifi > 18)
    {
      std::cout << "nWifi should be 18 or less; otherwise grid layout exceeds the bounding box" << std::endl;
      return 1;
    }*/

  if (verbose)
    {
      //LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      //LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("MihLinkSap", LOG_LEVEL_INFO);
    }

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);

  WifiMihLinkSapHelper wifiMifLinkSapHelper;
  Ptr<mih::WifiMihLinkSap> mihLinkSap1 = wifiMifLinkSapHelper.Install (wifiStaNodes.Get (0));

  //Ptr<mih::WifiMihLinkSap> mihLinkSap1 = CreateObject<mih::WifiMihLinkSap> ();
  //wifiStaNodes.Get (0)->AggregateObject(mihLinkSap1);
  //Ptr<mih::WifiMihLinkSap> mihLinkSap2 = CreateObject<mih::WifiMihLinkSap> ();
  //wifiStaNodes.Get (1)->AggregateObject (mihLinkSap2);
  //Ptr<mih::WifiMihLinkSap> mihLinkSap3 = CreateObject<mih::WifiMihLinkSap> ();
  //wifiStaNodes.Get (2)->AggregateObject (mihLinkSap3);

  NodeContainer wifiApNode;
  wifiApNode.Create (1);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

  MobilityHelper mobility;

  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc ->Add (Vector (0, 0, 0));
  positionAlloc ->Add (Vector (20, 0, 0));
  mobility.SetPositionAllocator (positionAlloc);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiStaNodes);
  mobility.Install (wifiApNode);

  Simulator::Schedule (Seconds (5.0), &SetPosition, wifiStaNodes.Get(0), 300);
  InternetStackHelper stack;
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;

  address.SetBase ("10.1.3.0", "255.255.255.0");
  address.Assign (staDevices);
  address.Assign (apDevices);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop (Seconds (10.0));

  if (tracing == true)
    {
      phy.EnablePcap ("wifimihlinkdown_node", staDevices.Get (0));
      phy.EnablePcap ("wifimihlinkdown_ap", apDevices.Get (0));
    }

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
