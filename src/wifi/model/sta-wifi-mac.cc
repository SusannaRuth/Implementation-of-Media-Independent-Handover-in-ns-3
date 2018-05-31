/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 */

#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "sta-wifi-mac.h"
#include "wifi-phy.h"
#include "mac-low.h"
#include "mgt-headers.h"
#include "ns3/wifi-mih-link-sap.h"
#include "snr-tag.h"
#include "signal-strength-tag.h"
#include "station-count-tag.h"

/*
 * The state machine for this STA is:
 --------------                                          -----------
 | Associated |   <--------------------      ------->    | Refused |
 --------------                        \    /            -----------
    \                                   \  /
     \    -----------------     -----------------------------
      \-> | Beacon Missed | --> | Wait Association Response |
          -----------------     -----------------------------
                \                       ^
                 \                      |
                  \    -----------------------
                   \-> | Wait Probe Response |
                       -----------------------
 */

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("StaWifiMac");

NS_OBJECT_ENSURE_REGISTERED (StaWifiMac);

TypeId
StaWifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::StaWifiMac")
    .SetParent<InfrastructureWifiMac> ()
    .SetGroupName ("Wifi")
    .AddConstructor<StaWifiMac> ()
    .AddAttribute ("ProbeRequestTimeout", "The interval between two consecutive probe request attempts.",
                   TimeValue (Seconds (0.05)),
                   MakeTimeAccessor (&StaWifiMac::m_probeRequestTimeout),
                   MakeTimeChecker ())
    .AddAttribute ("AssocRequestTimeout", "The interval between two consecutive association request attempts.",
                   TimeValue (Seconds (0.5)),
                   MakeTimeAccessor (&StaWifiMac::m_assocRequestTimeout),
                   MakeTimeChecker ())
    .AddAttribute ("MaxMissedBeacons",
                   "Number of beacons which much be consecutively missed before "
                   "we attempt to restart association.",
                   UintegerValue (10),
                   MakeUintegerAccessor (&StaWifiMac::m_maxMissedBeacons),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("ActiveProbing",
                   "If true, we send probe requests. If false, we don't."
                   "NOTE: if more than one STA in your simulation is using active probing, "
                   "you should enable it at a different simulation time for each STA, "
                   "otherwise all the STAs will start sending probes at the same time resulting in collisions. "
                   "See bug 1060 for more info.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&StaWifiMac::SetActiveProbing, &StaWifiMac::GetActiveProbing),
                   MakeBooleanChecker ())
    .AddTraceSource ("Assoc", "Associated with an access point.",
                     MakeTraceSourceAccessor (&StaWifiMac::m_assocLogger),
                     "ns3::Mac48Address::TracedCallback")
    .AddTraceSource ("DeAssoc", "Association with an access point lost.",
                     MakeTraceSourceAccessor (&StaWifiMac::m_deAssocLogger),
                     "ns3::Mac48Address::TracedCallback")
  ;
  return tid;
}

StaWifiMac::StaWifiMac ()
  : m_state (BEACON_MISSED),
    m_probeRequestEvent (),
    m_assocRequestEvent (),
    m_beaconWatchdogEnd (Seconds (0))
{
  NS_LOG_FUNCTION (this);

  //Let the lower layers know that we are acting as a non-AP STA in
  //an infrastructure BSS.
  SetTypeOfStation (STA);
}

StaWifiMac::~StaWifiMac ()
{
  NS_LOG_FUNCTION (this);
}

void
StaWifiMac::SetActiveProbing (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  if (enable)
    {
      Simulator::ScheduleNow (&StaWifiMac::TryToEnsureAssociated, this);
    }
  else
    {
      m_probeRequestEvent.Cancel ();
    }
  m_activeProbing = enable;
}

bool
StaWifiMac::GetActiveProbing (void) const
{
  return m_activeProbing;
}

void
StaWifiMac::SetWifiPhy (const Ptr<WifiPhy> phy)
{
  NS_LOG_FUNCTION (this << phy);
  RegularWifiMac::SetWifiPhy (phy);
  m_phy->SetCapabilitiesChangedCallback (MakeCallback (&StaWifiMac::PhyCapabilitiesChanged, this));
}

void
StaWifiMac::SetWifiRemoteStationManager (const Ptr<WifiRemoteStationManager> stationManager)
{
  NS_LOG_FUNCTION (this << stationManager);
  RegularWifiMac::SetWifiRemoteStationManager (stationManager);
  m_stationManager->SetPcfSupported (GetPcfSupported ());
}

void
StaWifiMac::SendProbeRequest (void)
{
  NS_LOG_FUNCTION (this);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_PROBE_REQUEST);
  hdr.SetAddr1 (Mac48Address::GetBroadcast ());
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (Mac48Address::GetBroadcast ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();
  Ptr<Packet> packet = Create<Packet> ();
  MgtProbeRequestHeader probe;
  probe.SetSsid (GetSsid ());
  probe.SetSupportedRates (GetSupportedRates ());
  if (GetHtSupported () || GetVhtSupported () || GetHeSupported ())
    {
      probe.SetExtendedCapabilities (GetExtendedCapabilities ());
      probe.SetHtCapabilities (GetHtCapabilities ());
    }
  if (GetVhtSupported () || GetHeSupported ())
    {
      probe.SetVhtCapabilities (GetVhtCapabilities ());
    }
  if (GetHeSupported ())
    {
      probe.SetHeCapabilities (GetHeCapabilities ());
    }
  packet->AddHeader (probe);

  //The standard is not clear on the correct queue for management
  //frames if we are a QoS AP. The approach taken here is to always
  //use the non-QoS for these regardless of whether we have a QoS
  //association or not.
  m_txop->Queue (packet, hdr);

  if (m_probeRequestEvent.IsRunning ())
    {
      m_probeRequestEvent.Cancel ();
    }
  m_probeRequestEvent = Simulator::Schedule (m_probeRequestTimeout,
                                             &StaWifiMac::ProbeRequestTimeout, this);
}

void
StaWifiMac::SendAssociationRequest (bool isReassoc)
{
  NS_LOG_FUNCTION (this << GetBssid () << isReassoc);
  WifiMacHeader hdr;
  hdr.SetType (isReassoc ? WIFI_MAC_MGT_REASSOCIATION_REQUEST : WIFI_MAC_MGT_ASSOCIATION_REQUEST);
  hdr.SetAddr1 (GetBssid ());
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetBssid ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();
  Ptr<Packet> packet = Create<Packet> ();
  if (!isReassoc)
    {
      MgtAssocRequestHeader assoc;
      assoc.SetSsid (GetSsid ());
      assoc.SetSupportedRates (GetSupportedRates ());
      assoc.SetCapabilities (GetCapabilities ());
      assoc.SetListenInterval (0);
      if (GetHtSupported () || GetVhtSupported () || GetHeSupported ())
        {
          assoc.SetExtendedCapabilities (GetExtendedCapabilities ());
          assoc.SetHtCapabilities (GetHtCapabilities ());
        }
      if (GetVhtSupported () || GetHeSupported ())
        {
          assoc.SetVhtCapabilities (GetVhtCapabilities ());
        }
      if (GetHeSupported ())
        {
          assoc.SetHeCapabilities (GetHeCapabilities ());
        }
      packet->AddHeader (assoc);
    }
  else
    {
      MgtReassocRequestHeader reassoc;
      reassoc.SetCurrentApAddress (GetBssid ());
      reassoc.SetSsid (GetSsid ());
      reassoc.SetSupportedRates (GetSupportedRates ());
      reassoc.SetCapabilities (GetCapabilities ());
      reassoc.SetListenInterval (0);
      if (GetHtSupported () || GetVhtSupported () || GetHeSupported ())
        {
          reassoc.SetExtendedCapabilities (GetExtendedCapabilities ());
          reassoc.SetHtCapabilities (GetHtCapabilities ());
        }
      if (GetVhtSupported () || GetHeSupported ())
        {
          reassoc.SetVhtCapabilities (GetVhtCapabilities ());
        }
      if (GetHeSupported ())
        {
          reassoc.SetHeCapabilities (GetHeCapabilities ());
        }
      packet->AddHeader (reassoc);
    }

  //The standard is not clear on the correct queue for management
  //frames if we are a QoS AP. The approach taken here is to always
  //use the non-QoS for these regardless of whether we have a QoS
  //association or not.
  m_txop->Queue (packet, hdr);

  if (m_assocRequestEvent.IsRunning ())
    {
      m_assocRequestEvent.Cancel ();
    }
  m_assocRequestEvent = Simulator::Schedule (m_assocRequestTimeout,
                                             &StaWifiMac::AssocRequestTimeout, this);
}

void
StaWifiMac::SendCfPollResponse (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (GetPcfSupported ());
  m_txop->SendCfFrame (WIFI_MAC_DATA_NULL, GetBssid ());
}

void
StaWifiMac::SendDisassociationRequest (void)
{
  NS_LOG_FUNCTION (this << GetBssid ());
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_DISASSOCIATION);
  hdr.SetAddr1 (GetBssid ());
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetBssid ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();
  Ptr<Packet> packet = Create<Packet> ();

  //The standard is not clear on the correct queue for management
  //frames if we are a QoS AP. The approach taken here is to always
  //use the DCF for these regardless of whether we have a QoS
  //association or not.
  m_txop->Queue (packet, hdr);

}

void
StaWifiMac::TryToEnsureAssociated (void)
{
  NS_LOG_FUNCTION (this);
  switch (m_state)
    {
    case ASSOCIATED:
      return;
      break;
    case WAIT_PROBE_RESP:
      /* we have sent a probe request earlier so we
         do not need to re-send a probe request immediately.
         We just need to wait until probe-request-timeout
         or until we get a probe response
       */
      break;
    case BEACON_MISSED:
      /* we were associated but we missed a bunch of beacons
       * so we should assume we are not associated anymore.
       * We try to initiate a probe request now.
       */
      m_linkDown ();
      if (!m_mihLinkDown.IsNull ())
        {
          mih::LinkIdentifier linkId = mih::LinkIdentifier (mih::LinkType (mih::LinkType::WIRELESS_802_11),
                                                            GetAddress (), GetBssid ());
          m_mihLinkDown (linkId, GetBssid (), mih::LinkDownReason(mih::LinkDownReason::NO_BROADCAST));
        }
      if (GetActiveProbing ())
        {
          SetState (WAIT_PROBE_RESP);
          SendProbeRequest ();
        }
      break;
    case WAIT_ASSOC_RESP:
      /* we have sent an association request so we do not need to
         re-send an association request right now. We just need to
         wait until either assoc-request-timeout or until
         we get an association response.
       */
      break;
    case REFUSED:
      /* we have sent an association request and received a negative
         association response. We wait until someone restarts an
         association with a given ssid.
       */
      break;
    }
}

void
StaWifiMac::AssocRequestTimeout (void)
{
  NS_LOG_FUNCTION (this);
  SetState (WAIT_ASSOC_RESP);
  SendAssociationRequest (false);
}

void
StaWifiMac::ProbeRequestTimeout (void)
{
  NS_LOG_FUNCTION (this);
  SetState (WAIT_PROBE_RESP);
  SendProbeRequest ();
}

void
StaWifiMac::MissedBeacons (void)
{
  NS_LOG_FUNCTION (this);
  if (m_beaconWatchdogEnd > Simulator::Now ())
    {
      if (m_beaconWatchdog.IsRunning ())
        {
          m_beaconWatchdog.Cancel ();
        }
      m_beaconWatchdog = Simulator::Schedule (m_beaconWatchdogEnd - Simulator::Now (),
                                              &StaWifiMac::MissedBeacons, this);
      return;
    }
  NS_LOG_DEBUG ("beacon missed");
  SetState (BEACON_MISSED);
  TryToEnsureAssociated ();
}

void
StaWifiMac::RestartBeaconWatchdog (Time delay)
{
  NS_LOG_FUNCTION (this << delay);
  m_beaconWatchdogEnd = std::max (Simulator::Now () + delay, m_beaconWatchdogEnd);
  if (Simulator::GetDelayLeft (m_beaconWatchdog) < delay
      && m_beaconWatchdog.IsExpired ())
    {
      NS_LOG_DEBUG ("really restart watchdog.");
      m_beaconWatchdog = Simulator::Schedule (delay, &StaWifiMac::MissedBeacons, this);
    }
}

bool
StaWifiMac::IsAssociated (void) const
{
  return m_state == ASSOCIATED;
}

bool
StaWifiMac::IsWaitAssocResp (void) const
{
  return m_state == WAIT_ASSOC_RESP;
}

void
StaWifiMac::Enqueue (Ptr<const Packet> packet, Mac48Address to)
{
  NS_LOG_FUNCTION (this << packet << to);
  if (!IsAssociated ())
    {
      NotifyTxDrop (packet);
      TryToEnsureAssociated ();
      return;
    }
  WifiMacHeader hdr;

  //If we are not a QoS AP then we definitely want to use AC_BE to
  //transmit the packet. A TID of zero will map to AC_BE (through \c
  //QosUtilsMapTidToAc()), so we use that as our default here.
  uint8_t tid = 0;

  //For now, an AP that supports QoS does not support non-QoS
  //associations, and vice versa. In future the AP model should
  //support simultaneously associated QoS and non-QoS STAs, at which
  //point there will need to be per-association QoS state maintained
  //by the association state machine, and consulted here.
  if (GetQosSupported ())
    {
      hdr.SetType (WIFI_MAC_QOSDATA);
      hdr.SetQosAckPolicy (WifiMacHeader::NORMAL_ACK);
      hdr.SetQosNoEosp ();
      hdr.SetQosNoAmsdu ();
      //Transmission of multiple frames in the same TXOP is not
      //supported for now
      hdr.SetQosTxopLimit (0);

      //Fill in the QoS control field in the MAC header
      tid = QosUtilsGetTidForPacket (packet);
      //Any value greater than 7 is invalid and likely indicates that
      //the packet had no QoS tag, so we revert to zero, which'll
      //mean that AC_BE is used.
      if (tid > 7)
        {
          tid = 0;
        }
      hdr.SetQosTid (tid);
    }
  else
    {
      hdr.SetType (WIFI_MAC_DATA);
    }
  if (GetQosSupported () || GetHtSupported () || GetVhtSupported () || GetHeSupported ())
    {
      hdr.SetNoOrder ();
    }

  hdr.SetAddr1 (GetBssid ());
  hdr.SetAddr2 (m_low->GetAddress ());
  hdr.SetAddr3 (to);
  hdr.SetDsNotFrom ();
  hdr.SetDsTo ();

  if (GetQosSupported ())
    {
      //Sanity check that the TID is valid
      NS_ASSERT (tid < 8);
      m_edca[QosUtilsMapTidToAc (tid)]->Queue (packet, hdr);
    }
  else
    {
      m_txop->Queue (packet, hdr);
    }
}

void
StaWifiMac::Receive (Ptr<Packet> packet, const WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << packet << hdr);
  NS_ASSERT (!hdr->IsCtl ());
  if (hdr->GetAddr3 () == GetAddress ())
    {
      NS_LOG_LOGIC ("packet sent by us.");
      return;
    }
  else if (hdr->GetAddr1 () != GetAddress ()
           && !hdr->GetAddr1 ().IsGroup ())
    {
      NS_LOG_LOGIC ("packet is not for us");
      NotifyRxDrop (packet);
      return;
    }
  else if ((hdr->GetAddr1 () == GetAddress ()) && (hdr->GetAddr2 () == GetBssid ()) && hdr->IsCfPoll ())
    {
      SendCfPollResponse ();
    }
  if (hdr->IsData ())
    {
      if (!IsAssociated ())
        {
          NS_LOG_LOGIC ("Received data frame while not associated: ignore");
          NotifyRxDrop (packet);
          return;
        }
      if (!(hdr->IsFromDs () && !hdr->IsToDs ()))
        {
          NS_LOG_LOGIC ("Received data frame not from the DS: ignore");
          NotifyRxDrop (packet);
          return;
        }
      if (hdr->GetAddr2 () != GetBssid ())
        {
          NS_LOG_LOGIC ("Received data frame not from the BSS we are associated with: ignore");
          NotifyRxDrop (packet);
          return;
        }
      if (hdr->IsQosData ())
        {
          if (hdr->IsQosAmsdu ())
            {
              NS_ASSERT (hdr->GetAddr3 () == GetBssid ());
              DeaggregateAmsduAndForward (packet, hdr);
              packet = 0;
            }
          else
            {
              ForwardUp (packet, hdr->GetAddr3 (), hdr->GetAddr1 ());
            }
        }
      else if (hdr->HasData ())
        {
          ForwardUp (packet, hdr->GetAddr3 (), hdr->GetAddr1 ());
        }
      return;
    }
  else if (hdr->IsProbeReq ()
           || hdr->IsAssocReq ()
           || hdr->IsReassocReq ())
    {
      //This is a frame aimed at an AP, so we can safely ignore it.
      NotifyRxDrop (packet);
      return;
    }
  else if (hdr->IsBeacon ())
    {
      NS_LOG_DEBUG ("Beacon received");
      MgtBeaconHeader beacon;
      packet->RemoveHeader (beacon);
      CapabilityInformation capabilities = beacon.GetCapabilities ();
      NS_ASSERT (capabilities.IsEss ());
      bool goodBeacon = false;
      if (GetSsid ().IsBroadcast ()
          || beacon.GetSsid ().IsEqual (GetSsid ()))
        {
          NS_LOG_LOGIC ("Beacon is for our SSID");
          goodBeacon = true;
        }
      CfParameterSet cfParameterSet = beacon.GetCfParameterSet ();
      if (cfParameterSet.GetCFPCount () == 0)
        {
          //see section 9.3.2.2 802.11-1999
          if (GetPcfSupported ())
            {
              m_low->DoNavStartNow (MicroSeconds (cfParameterSet.GetCFPMaxDurationUs ()));
            }
          else
            {
              m_low->DoNavStartNow (MicroSeconds (cfParameterSet.GetCFPDurRemainingUs ()));
            }
        }
      SupportedRates rates = beacon.GetSupportedRates ();
      
      bool sendAssoc = false;
      if (!m_mihLinkDetected.IsNull ())
        {
          if (!(IsAssociated () && hdr->GetAddr3 () == GetBssid ()))
            {
              mih::LinkIdentifier linkId = mih::LinkIdentifier (mih::LinkType (mih::LinkType::WIRELESS_802_11),
                                                                GetAddress (),
                                                                hdr->GetAddr2 ());
              mih::NetworkIdentifier networkId = mih::NetworkIdentifier ();
              mih::NetworkAuxiliaryIdentifier networkAuxiliaryId = mih::NetworkAuxiliaryIdentifier ();
              SignalStrengthTag signalStrengthTag;
              packet->RemovePacketTag (signalStrengthTag);
              uint16_t signal = (uint16_t) signalStrengthTag.Get ();
              mih::SignalStrength signalStrength = mih::SignalStrength (signal);
              SnrTag tag;
              packet->PeekPacketTag (tag);
              uint16_t snr = (uint16_t) tag.Get ();
              mih::MihCapabilityFlag mihCapabilityFlag = mih::MihCapabilityFlag ();
              mih::NetworkCapabilities networkCapabilities =  mih::NetworkCapabilities ();
              StationCountTag stationCountTag;
              packet->RemovePacketTag (stationCountTag);
              uint32_t stationCount = stationCountTag.Get ();
              mih::LinkDetectedInformation linkInfo = mih::LinkDetectedInformation (linkId, networkId,
                                                                                    networkAuxiliaryId, signalStrength, snr, 
                                                                                    rates, mihCapabilityFlag,
                                                                                    networkCapabilities, stationCount);
              sendAssoc = m_mihLinkDetected (linkInfo);
            }
        }
      if (IsAssociated () && sendAssoc)
        {
          SendDisassociationRequest ();
        }
      bool bssMembershipSelectorMatch = false;
      for (uint8_t i = 0; i < m_phy->GetNBssMembershipSelectors (); i++)
        {
          uint8_t selector = m_phy->GetBssMembershipSelector (i);
          if (rates.IsBssMembershipSelectorRate (selector))
            {
              NS_LOG_LOGIC ("Beacon is matched to our BSS membership selector");
              bssMembershipSelectorMatch = true;
            }
        }
      if (m_phy->GetNBssMembershipSelectors () > 0 && bssMembershipSelectorMatch == false)
        {
          NS_LOG_LOGIC ("No match for BSS membership selector");
          goodBeacon = false;
          sendAssoc = false;
        }
      if ((IsWaitAssocResp () || IsAssociated ()) && hdr->GetAddr3 () != GetBssid ())
        {
          NS_LOG_LOGIC ("Beacon is not for us");
          goodBeacon = false;
        }
      if (goodBeacon || (!m_mihLinkDetected.IsNull () && sendAssoc))
        {
          Time delay = MicroSeconds (beacon.GetBeaconIntervalUs () * m_maxMissedBeacons);
          RestartBeaconWatchdog (delay);
          SetBssid (hdr->GetAddr3 ());
          rates = beacon.GetSupportedRates ();
          for (uint8_t i = 0; i < m_phy->GetNModes (); i++)
            {
              WifiMode mode = m_phy->GetMode (i);
              if (rates.IsSupportedRate (mode.GetDataRate (m_phy->GetChannelWidth ())))
                {
                  m_stationManager->AddSupportedMode (hdr->GetAddr2 (), mode);
                }
            }
          bool isShortPreambleEnabled = capabilities.IsShortPreamble ();
          if (GetErpSupported ())
            {
              ErpInformation erpInformation = beacon.GetErpInformation ();
              isShortPreambleEnabled &= !erpInformation.GetBarkerPreambleMode ();
              if (erpInformation.GetUseProtection () != 0)
                {
                  m_stationManager->SetUseNonErpProtection (true);
                }
              else
                {
                  m_stationManager->SetUseNonErpProtection (false);
                }
              if (capabilities.IsShortSlotTime () == true)
                {
                  //enable short slot time
                  SetSlot (MicroSeconds (9));
                }
              else
                {
                  //disable short slot time
                  SetSlot (MicroSeconds (20));
                }
            }
          if (GetQosSupported ())
            {
              bool qosSupported = false;
              EdcaParameterSet edcaParameters = beacon.GetEdcaParameterSet ();
              if (edcaParameters.IsQosSupported ())
                {
                  qosSupported = true;
                  //The value of the TXOP Limit field is specified as an unsigned integer, with the least significant octet transmitted first, in units of 32 μs.
                  SetEdcaParameters (AC_BE, edcaParameters.GetBeCWmin (), edcaParameters.GetBeCWmax (), edcaParameters.GetBeAifsn (), 32 * MicroSeconds (edcaParameters.GetBeTXOPLimit ()));
                  SetEdcaParameters (AC_BK, edcaParameters.GetBkCWmin (), edcaParameters.GetBkCWmax (), edcaParameters.GetBkAifsn (), 32 * MicroSeconds (edcaParameters.GetBkTXOPLimit ()));
                  SetEdcaParameters (AC_VI, edcaParameters.GetViCWmin (), edcaParameters.GetViCWmax (), edcaParameters.GetViAifsn (), 32 * MicroSeconds (edcaParameters.GetViTXOPLimit ()));
                  SetEdcaParameters (AC_VO, edcaParameters.GetVoCWmin (), edcaParameters.GetVoCWmax (), edcaParameters.GetVoAifsn (), 32 * MicroSeconds (edcaParameters.GetVoTXOPLimit ()));
                }
              m_stationManager->SetQosSupport (hdr->GetAddr2 (), qosSupported);
            }
          if (GetHtSupported ())
            {
              HtCapabilities htCapabilities = beacon.GetHtCapabilities ();
              if (!htCapabilities.IsSupportedMcs (0))
                {
                  m_stationManager->RemoveAllSupportedMcs (hdr->GetAddr2 ());
                }
              else
                {
                  m_stationManager->AddStationHtCapabilities (hdr->GetAddr2 (), htCapabilities);
                  HtOperation htOperation = beacon.GetHtOperation ();
                  if (htOperation.GetNonGfHtStasPresent ())
                    {
                      m_stationManager->SetUseGreenfieldProtection (true);
                    }
                  else
                    {
                      m_stationManager->SetUseGreenfieldProtection (false);
                    }
                  if (!GetVhtSupported () && GetRifsSupported () && htOperation.GetRifsMode ())
                    {
                      m_stationManager->SetRifsPermitted (true);
                    }
                  else
                    {
                      m_stationManager->SetRifsPermitted (false);
                    }
                }
            }
          if (GetVhtSupported ())
            {
              VhtCapabilities vhtCapabilities = beacon.GetVhtCapabilities ();
              //we will always fill in RxHighestSupportedLgiDataRate field at TX, so this can be used to check whether it supports VHT
              if (vhtCapabilities.GetRxHighestSupportedLgiDataRate () > 0)
                {
                  m_stationManager->AddStationVhtCapabilities (hdr->GetAddr2 (), vhtCapabilities);
                  VhtOperation vhtOperation = beacon.GetVhtOperation ();
                  for (uint8_t i = 0; i < m_phy->GetNMcs (); i++)
                    {
                      WifiMode mcs = m_phy->GetMcs (i);
                      if (mcs.GetModulationClass () == WIFI_MOD_CLASS_VHT && vhtCapabilities.IsSupportedRxMcs (mcs.GetMcsValue ()))
                        {
                          m_stationManager->AddSupportedMcs (hdr->GetAddr2 (), mcs);
                        }
                    }
                }
            }
          if (GetHtSupported () || GetVhtSupported ())
            {
              ExtendedCapabilities extendedCapabilities = beacon.GetExtendedCapabilities ();
              //TODO: to be completed
            }
          if (GetHeSupported ())
            {
              HeCapabilities heCapabilities = beacon.GetHeCapabilities ();
              //todo: once we support non constant rate managers, we should add checks here whether HE is supported by the peer
              m_stationManager->AddStationHeCapabilities (hdr->GetAddr2 (), heCapabilities);
              HeOperation heOperation = beacon.GetHeOperation ();
              for (uint8_t i = 0; i < m_phy->GetNMcs (); i++)
                {
                  WifiMode mcs = m_phy->GetMcs (i);
                  if (mcs.GetModulationClass () == WIFI_MOD_CLASS_HE && heCapabilities.IsSupportedRxMcs (mcs.GetMcsValue ()))
                    {
                      m_stationManager->AddSupportedMcs (hdr->GetAddr2 (), mcs);
                    }
                }
            }
          m_stationManager->SetShortPreambleEnabled (isShortPreambleEnabled);
          m_stationManager->SetShortSlotTimeEnabled (capabilities.IsShortSlotTime ());
        }
      if ((m_mihLinkDetected.IsNull () && goodBeacon && m_state == BEACON_MISSED) || sendAssoc)
        {
          SetState (WAIT_ASSOC_RESP);
          NS_LOG_DEBUG ("Good beacon received: send association request");
          SendAssociationRequest (false);
        }
      return;
    }
  else if (hdr->IsProbeResp ())
    {
      if (m_state == WAIT_PROBE_RESP)
        {
          NS_LOG_DEBUG ("Probe response received");
          MgtProbeResponseHeader probeResp;
          packet->RemoveHeader (probeResp);
          CapabilityInformation capabilities = probeResp.GetCapabilities ();
          if (!probeResp.GetSsid ().IsEqual (GetSsid ()))
            {
              NS_LOG_DEBUG ("Probe response is not for our SSID");
              return;
            }
          SupportedRates rates = probeResp.GetSupportedRates ();
          for (uint8_t i = 0; i < m_phy->GetNBssMembershipSelectors (); i++)
            {
              uint8_t selector = m_phy->GetBssMembershipSelector (i);
              if (!rates.IsBssMembershipSelectorRate (selector))
                {
                  NS_LOG_DEBUG ("Supported rates do not fit with the BSS membership selector");
                  return;
                }
            }
          for (uint32_t i = 0; i < m_phy->GetNModes (); i++)
            {
              WifiMode mode = m_phy->GetMode (i);
              if (rates.IsSupportedRate (mode.GetDataRate (m_phy->GetChannelWidth ())))
                {
                  m_stationManager->AddSupportedMode (hdr->GetAddr2 (), mode);
                  if (rates.IsBasicRate (mode.GetDataRate (m_phy->GetChannelWidth ())))
                    {
                      m_stationManager->AddBasicMode (mode);
                    }
                }
            }

          bool isShortPreambleEnabled = capabilities.IsShortPreamble ();
          if (GetErpSupported ())
            {
              bool isErpAllowed = false;
              for (uint8_t i = 0; i < m_phy->GetNModes (); i++)
                {
                  WifiMode mode = m_phy->GetMode (i);
                  if (mode.GetModulationClass () == WIFI_MOD_CLASS_ERP_OFDM && rates.IsSupportedRate (mode.GetDataRate (m_phy->GetChannelWidth ())))
                    {
                      isErpAllowed = true;
                      break;
                    }
                }
              if (!isErpAllowed)
                {
                  //disable short slot time and set cwMin to 31
                  SetSlot (MicroSeconds (20));
                  ConfigureContentionWindow (31, 1023);
                }
              else
                {
                  ErpInformation erpInformation = probeResp.GetErpInformation ();
                  isShortPreambleEnabled &= !erpInformation.GetBarkerPreambleMode ();
                  if (m_stationManager->GetShortSlotTimeEnabled ())
                    {
                      //enable short slot time
                      SetSlot (MicroSeconds (9));
                    }
                  else
                    {
                      //disable short slot time
                      SetSlot (MicroSeconds (20));
                    }
                  ConfigureContentionWindow (15, 1023);
                }
            }
          m_stationManager->SetShortPreambleEnabled (isShortPreambleEnabled);
          m_stationManager->SetShortSlotTimeEnabled (capabilities.IsShortSlotTime ());
          SetBssid (hdr->GetAddr3 ());
          Time delay = MicroSeconds (probeResp.GetBeaconIntervalUs () * m_maxMissedBeacons);
          RestartBeaconWatchdog (delay);
          if (m_probeRequestEvent.IsRunning ())
            {
              m_probeRequestEvent.Cancel ();
            }
          if (!m_mihLinkDetected.IsNull ())
            {
              mih::LinkIdentifier linkId = mih::LinkIdentifier (mih::LinkType (mih::LinkType::WIRELESS_802_11),
                                                                hdr->GetAddr1 (),
                                                                hdr->GetAddr2 ());
              mih::NetworkIdentifier networkId = mih::NetworkIdentifier ();
              mih::NetworkAuxiliaryIdentifier networkAuxiliaryId = mih::NetworkAuxiliaryIdentifier ();
              SignalStrengthTag signalStrengthTag;
              packet->RemovePacketTag (signalStrengthTag);
              uint16_t signal = (uint16_t) signalStrengthTag.Get ();
              mih::SignalStrength signalStrength = mih::SignalStrength (signal);
              SnrTag tag;
              packet->PeekPacketTag (tag);
              uint16_t snr = (uint16_t) tag.Get ();
              mih::MihCapabilityFlag mihCapabilityFlag = mih::MihCapabilityFlag ();
              mih::NetworkCapabilities networkCapabilities =  mih::NetworkCapabilities ();
              StationCountTag stationCountTag;
              packet->RemovePacketTag (stationCountTag);
              uint32_t stationCount = stationCountTag.Get ();
              mih::LinkDetectedInformation linkInfo = mih::LinkDetectedInformation (linkId, networkId,
                                                                                    networkAuxiliaryId, signalStrength, snr, 
                                                                                    rates, mihCapabilityFlag,
                                                                                    networkCapabilities, stationCount);
              m_mihLinkDetected (linkInfo);
            }
          SetState (WAIT_ASSOC_RESP);
          SendAssociationRequest (false);
	}
      return;
    }
  else if (hdr->IsAssocResp () || hdr->IsReassocResp ())
    {
      if (m_state == WAIT_ASSOC_RESP)
        {
          MgtAssocResponseHeader assocResp;
          packet->RemoveHeader (assocResp);
          if (m_assocRequestEvent.IsRunning ())
            {
              m_assocRequestEvent.Cancel ();
            }
          if (assocResp.GetStatusCode ().IsSuccess ())
            {
              SetState (ASSOCIATED);
              if (hdr->IsReassocResp ())
                {
                  NS_LOG_DEBUG ("reassociation done");
                }
              else
                {
                  NS_LOG_DEBUG ("association completed");
                }
              CapabilityInformation capabilities = assocResp.GetCapabilities ();
              SupportedRates rates = assocResp.GetSupportedRates ();
              bool isShortPreambleEnabled = capabilities.IsShortPreamble ();
              if (GetErpSupported ())
                {
                  bool isErpAllowed = false;
                  for (uint8_t i = 0; i < m_phy->GetNModes (); i++)
                    {
                      WifiMode mode = m_phy->GetMode (i);
                      if (mode.GetModulationClass () == WIFI_MOD_CLASS_ERP_OFDM && rates.IsSupportedRate (mode.GetDataRate (m_phy->GetChannelWidth ())))
                        {
                          isErpAllowed = true;
                          break;
                        }
                    }
                  if (!isErpAllowed)
                    {
                      //disable short slot time and set cwMin to 31
                      SetSlot (MicroSeconds (20));
                      ConfigureContentionWindow (31, 1023);
                    }
                  else
                    {
                      ErpInformation erpInformation = assocResp.GetErpInformation ();
                      isShortPreambleEnabled &= !erpInformation.GetBarkerPreambleMode ();
                      if (m_stationManager->GetShortSlotTimeEnabled ())
                        {
                          //enable short slot time
                          SetSlot (MicroSeconds (9));
                        }
                      else
                        {
                          //disable short slot time
                          SetSlot (MicroSeconds (20));
                        }
                      ConfigureContentionWindow (15, 1023);
                    }
                }
              m_stationManager->SetShortPreambleEnabled (isShortPreambleEnabled);
              m_stationManager->SetShortSlotTimeEnabled (capabilities.IsShortSlotTime ());
              if (GetQosSupported ())
                {
                  bool qosSupported = false;
                  EdcaParameterSet edcaParameters = assocResp.GetEdcaParameterSet ();
                  if (edcaParameters.IsQosSupported ())
                    {
                      qosSupported = true;
                      //The value of the TXOP Limit field is specified as an unsigned integer, with the least significant octet transmitted first, in units of 32 μs.
                      SetEdcaParameters (AC_BE, edcaParameters.GetBeCWmin (), edcaParameters.GetBeCWmax (), edcaParameters.GetBeAifsn (), 32 * MicroSeconds (edcaParameters.GetBeTXOPLimit ()));
                      SetEdcaParameters (AC_BK, edcaParameters.GetBkCWmin (), edcaParameters.GetBkCWmax (), edcaParameters.GetBkAifsn (), 32 * MicroSeconds (edcaParameters.GetBkTXOPLimit ()));
                      SetEdcaParameters (AC_VI, edcaParameters.GetViCWmin (), edcaParameters.GetViCWmax (), edcaParameters.GetViAifsn (), 32 * MicroSeconds (edcaParameters.GetViTXOPLimit ()));
                      SetEdcaParameters (AC_VO, edcaParameters.GetVoCWmin (), edcaParameters.GetVoCWmax (), edcaParameters.GetVoAifsn (), 32 * MicroSeconds (edcaParameters.GetVoTXOPLimit ()));
                    }
                  m_stationManager->SetQosSupport (hdr->GetAddr2 (), qosSupported);
                }
              if (GetHtSupported ())
                {
                  HtCapabilities htCapabilities = assocResp.GetHtCapabilities ();
                  if (!htCapabilities.IsSupportedMcs (0))
                    {
                      m_stationManager->RemoveAllSupportedMcs (hdr->GetAddr2 ());
                    }
                  else
                    {
                      m_stationManager->AddStationHtCapabilities (hdr->GetAddr2 (), htCapabilities);
                      HtOperation htOperation = assocResp.GetHtOperation ();
                      if (htOperation.GetNonGfHtStasPresent ())
                        {
                          m_stationManager->SetUseGreenfieldProtection (true);
                        }
                      else
                        {
                          m_stationManager->SetUseGreenfieldProtection (false);
                        }
                      if (!GetVhtSupported () && GetRifsSupported () && htOperation.GetRifsMode ())
                        {
                          m_stationManager->SetRifsPermitted (true);
                        }
                      else
                        {
                          m_stationManager->SetRifsPermitted (false);
                        }
                    }
                }
              if (GetVhtSupported ())
                {
                  VhtCapabilities vhtCapabilities = assocResp.GetVhtCapabilities ();
                  //we will always fill in RxHighestSupportedLgiDataRate field at TX, so this can be used to check whether it supports VHT
                  if (vhtCapabilities.GetRxHighestSupportedLgiDataRate () > 0)
                    {
                      m_stationManager->AddStationVhtCapabilities (hdr->GetAddr2 (), vhtCapabilities);
                      VhtOperation vhtOperation = assocResp.GetVhtOperation ();
                    }
                }
              if (GetHeSupported ())
                {
                  HeCapabilities hecapabilities = assocResp.GetHeCapabilities ();
                  //todo: once we support non constant rate managers, we should add checks here whether HE is supported by the peer
                  m_stationManager->AddStationHeCapabilities (hdr->GetAddr2 (), hecapabilities);
                  HeOperation heOperation = assocResp.GetHeOperation ();
                }
              for (uint8_t i = 0; i < m_phy->GetNModes (); i++)
                {
                  WifiMode mode = m_phy->GetMode (i);
                  if (rates.IsSupportedRate (mode.GetDataRate (m_phy->GetChannelWidth ())))
                    {
                      m_stationManager->AddSupportedMode (hdr->GetAddr2 (), mode);
                      if (rates.IsBasicRate (mode.GetDataRate (m_phy->GetChannelWidth ())))
                        {
                          m_stationManager->AddBasicMode (mode);
                        }
                    }
                }
              if (GetHtSupported ())
                {
                  HtCapabilities htCapabilities = assocResp.GetHtCapabilities ();
                  for (uint8_t i = 0; i < m_phy->GetNMcs (); i++)
                    {
                      WifiMode mcs = m_phy->GetMcs (i);
                      if (mcs.GetModulationClass () == WIFI_MOD_CLASS_HT && htCapabilities.IsSupportedMcs (mcs.GetMcsValue ()))
                        {
                          m_stationManager->AddSupportedMcs (hdr->GetAddr2 (), mcs);
                          //here should add a control to add basic MCS when it is implemented
                        }
                    }
                }
              if (GetVhtSupported ())
                {
                  VhtCapabilities vhtcapabilities = assocResp.GetVhtCapabilities ();
                  for (uint8_t i = 0; i < m_phy->GetNMcs (); i++)
                    {
                      WifiMode mcs = m_phy->GetMcs (i);
                      if (mcs.GetModulationClass () == WIFI_MOD_CLASS_VHT && vhtcapabilities.IsSupportedRxMcs (mcs.GetMcsValue ()))
                        {
                          m_stationManager->AddSupportedMcs (hdr->GetAddr2 (), mcs);
                          //here should add a control to add basic MCS when it is implemented
                        }
                    }
                }
              if (GetHtSupported () || GetVhtSupported ())
                {
                  ExtendedCapabilities extendedCapabilities = assocResp.GetExtendedCapabilities ();
                  //TODO: to be completed
                }
              if (GetHeSupported ())
                {
                  HeCapabilities heCapabilities = assocResp.GetHeCapabilities ();
                  for (uint8_t i = 0; i < m_phy->GetNMcs (); i++)
                    {
                      WifiMode mcs = m_phy->GetMcs (i);
                      if (mcs.GetModulationClass () == WIFI_MOD_CLASS_HE && heCapabilities.IsSupportedRxMcs (mcs.GetMcsValue ()))
                        {
                          m_stationManager->AddSupportedMcs (hdr->GetAddr2 (), mcs);
                          //here should add a control to add basic MCS when it is implemented
                        }
                    }
                }
              if (!m_linkUp.IsNull ())
                {
                  m_linkUp ();
                }
              if (!m_mihLinkUp.IsNull ())
                {
                  mih::LinkIdentifier linkId = mih::LinkIdentifier (mih::LinkType (mih::LinkType::WIRELESS_802_11),
                                                                    hdr->GetAddr1 (),
                                                                    hdr->GetAddr2 ());
                  m_mihLinkUp (linkId, Address (), hdr->GetAddr2 (), true, 
                               mih::MobilityManagementSupport (mih::MobilityManagementSupport::MOBILE_IPV4_RFC3344));
                }
            }
          else
            {
              NS_LOG_DEBUG ("association refused");
              SetState (REFUSED);
            }
        }
      return;
    }

  //Invoke the receive handler of our parent class to deal with any
  //other frames. Specifically, this will handle Block Ack-related
  //Management Action frames.
  RegularWifiMac::Receive (packet, hdr);
}

SupportedRates
StaWifiMac::GetSupportedRates (void) const
{
  SupportedRates rates;
  if (GetHtSupported () || GetVhtSupported () || GetHeSupported ())
    {
      for (uint8_t i = 0; i < m_phy->GetNBssMembershipSelectors (); i++)
        {
          rates.AddBssMembershipSelectorRate (m_phy->GetBssMembershipSelector (i));
        }
    }
  for (uint8_t i = 0; i < m_phy->GetNModes (); i++)
    {
      WifiMode mode = m_phy->GetMode (i);
      uint64_t modeDataRate = mode.GetDataRate (m_phy->GetChannelWidth ());
      NS_LOG_DEBUG ("Adding supported rate of " << modeDataRate);
      rates.AddSupportedRate (modeDataRate);
    }
  return rates;
}

CapabilityInformation
StaWifiMac::GetCapabilities (void) const
{
  CapabilityInformation capabilities;
  capabilities.SetShortPreamble (m_phy->GetShortPlcpPreambleSupported () || GetErpSupported ());
  capabilities.SetShortSlotTime (GetShortSlotTimeSupported () && GetErpSupported ());
  if (GetPcfSupported ())
    {
      capabilities.SetCfPollable ();
    }
  return capabilities;
}

void
StaWifiMac::SetState (MacState value)
{
  if (value == ASSOCIATED
      && m_state != ASSOCIATED)
    {
      m_assocLogger (GetBssid ());
    }
  else if (value != ASSOCIATED
           && m_state == ASSOCIATED)
    {
      m_deAssocLogger (GetBssid ());
    }
  m_state = value;
}

void
StaWifiMac::SetEdcaParameters (AcIndex ac, uint32_t cwMin, uint32_t cwMax, uint8_t aifsn, Time txopLimit)
{
  Ptr<QosTxop> edca = m_edca.find (ac)->second;
  edca->SetMinCw (cwMin);
  edca->SetMaxCw (cwMax);
  edca->SetAifsn (aifsn);
  edca->SetTxopLimit (txopLimit);
}

void
StaWifiMac::PhyCapabilitiesChanged (void)
{
  NS_LOG_FUNCTION (this);
  if (IsAssociated ())
    {
      NS_LOG_DEBUG ("PHY capabilities changed: send reassociation request");
      SetState (WAIT_ASSOC_RESP);
      SendAssociationRequest (true);
    }
}

} //namespace ns3
