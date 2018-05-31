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
#include "ns3/flow-monitor-module.h"

// Default Network Topology
//
//   Wifi 10.1.3.0
//    AP        AP    
//    *    *    *    *
//    |    |    |    |
//   n0   n1   n2   n3
//                   
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiMihApp");

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
  uint32_t nWifi = 2;
  bool tracing = false;

  CommandLine cmd;
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);

  cmd.Parse (argc,argv);

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("MihLinkSap", LOG_LEVEL_INFO);
      LogComponentEnable ("WifiMihLinkSap", LOG_LEVEL_INFO);
    }

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);

  WifiMihLinkSapHelper wifiMifLinkSapHelper;
  Ptr<mih::WifiMihLinkSap> mihLinkSap1 = wifiMifLinkSapHelper.Install (wifiStaNodes.Get (0));
  Ptr<mih::WifiMihLinkSap> mihLinkSap2 = wifiMifLinkSapHelper.Install (wifiStaNodes.Get (1));

  NodeContainer wifiApNode;
  wifiApNode.Create (2);

  Ptr<mih::WifiMihLinkSap> mihLinkSap3 = wifiMifLinkSapHelper.Install (wifiApNode.Get (0));
  Ptr<mih::WifiMihLinkSap> mihLinkSap4 = wifiMifLinkSapHelper.Install (wifiApNode.Get (1));

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
  positionAlloc ->Add (Vector (10, 0, 0));
  positionAlloc ->Add (Vector (80, 0, 0));
  positionAlloc ->Add (Vector (0, 0, 0));
  positionAlloc ->Add (Vector (25, 0 , 0));
  mobility.SetPositionAllocator (positionAlloc);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiStaNodes);
  mobility.Install (wifiApNode);

  Simulator::Schedule (Seconds (4.0), &SetPosition, wifiStaNodes.Get(0), 20);
  InternetStackHelper stack;
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;

  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer staInterfaces;
  staInterfaces = address.Assign (staDevices);
  address.Assign (apDevices);

  //UdpEchoServerHelper echoServer (9);

  //ApplicationContainer serverApps = echoServer.Install (wifiStaNodes.Get (0));
  //serverApps.Start (Seconds (1.0));
  //serverApps.Stop (Seconds (10.0));

  //UdpEchoClientHelper echoClient (staInterfaces.GetAddress (0), 9);
  //echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  //echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  //echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  //ApplicationContainer clientApps = 
    //echoClient.Install (wifiStaNodes.Get (1));
  //clientApps.Start (Seconds (2.0));
  //clientApps.Stop (Seconds (10.0));

//
// Create one udpServer applications on node one.
//
  uint16_t port = 4000;
  UdpServerHelper server (port);
  ApplicationContainer apps = server.Install (wifiStaNodes.Get (0));
  apps.Start (Seconds (4.0));
  apps.Stop (Seconds (10.0));

//
// Create one UdpTraceClient application to send UDP datagrams from node zero to
// node one.
//
  uint32_t MaxPacketSize = 1472;  // Back off 20 (IP) + 8 (UDP) bytes from MTU
  UdpTraceClientHelper client (staInterfaces.GetAddress (0), port,"");
  client.SetAttribute ("MaxPacketSize", UintegerValue (MaxPacketSize));
  apps = client.Install (wifiStaNodes.Get (1));
  apps.Start (Seconds (5.0));
  apps.Stop (Seconds (10.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  
  // Install FlowMonitor on all nodes
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  Simulator::Stop (Seconds (10.0));

  if (tracing == true)
    {
      phy.EnablePcap ("node1", staDevices.Get (0));
      phy.EnablePcap ("node2", staDevices.Get (1));
      phy.EnablePcap ("ap1", apDevices.Get (0));
      phy.EnablePcap ("ap2", apDevices.Get (1));
    }

  Simulator::Run ();

  // Print per flow statistics
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      // Duration for throughput measurement is 9.0 seconds, since
      //   StartTime of the OnOffApplication is at about "second 1"
      // and
      //   Simulator::Stops at "second 10".
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
      std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
      std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
      std::cout << "  TxOffered:  " << i->second.txBytes * 8.0 / 5.0 / 1000 / 1000  << " Mbps\n";
      std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
      std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
      std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / 5.0 / 1000 / 1000  << " Mbps\n";
      std::cout << "  Mean delay:   " << i->second.delaySum.GetSeconds () / i->second.rxPackets << "\n";
    }

  flowmon.SerializeToXmlFile ("mih-ap.flowmon", false, false);
  Simulator::Destroy ();
  return 0;
}
