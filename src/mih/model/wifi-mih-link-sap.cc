/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */            
/*
 * Copyright (c) 2008 IT-SUDPARIS
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
 * Author: Providence SALUMU M. <Providence.Salumu_Munga@it-sudparis.eu>
 */

#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/mac48-address.h"
#include "wifi-mih-link-sap.h"
#include "mih-device-information.h"
#include "ns3/double.h"

NS_LOG_COMPONENT_DEFINE ("WifiMihLinkSap");

namespace ns3 {
  namespace mih {
    NS_OBJECT_ENSURE_REGISTERED (WifiMihLinkSap);
    
    TypeId 
    WifiMihLinkSap::GetTypeId (void)
    {
      static TypeId tid = TypeId ("ns3::mih::WifiMihLinkSap")
	.SetParent<MihLinkSap> ()
	.AddConstructor<WifiMihLinkSap> ()
	;
      return tid;
    }
    WifiMihLinkSap::WifiMihLinkSap (void) :
      m_linkIdentifier ()
    {
      NS_LOG_FUNCTION_NOARGS ();
      m_signalStrength = 0;
      m_stationCount = 0;
      //m_eventTriggerInterval = CreateObject<UniformRandomVariable> ();
      //m_eventTriggerInterval->SetAttribute ("Min", DoubleValue(0));
      //m_eventTriggerInterval->SetAttribute ("Max", DoubleValue(0.5));
      //m_rngChoice = CreateObject<UniformRandomVariable> ();
      //m_rngChoice->SetAttribute ("Min", DoubleValue(0));
      //m_rngChoice->SetAttribute ("Max", DoubleValue(3));
    }
    WifiMihLinkSap::~WifiMihLinkSap (void)
    {
      NS_LOG_FUNCTION_NOARGS ();
    }

    bool
    WifiMihLinkSap::LinkDetected (MihfId sourceMihfId, 
                                  LinkDetectedInformation linkDetectedInfo) 
    {
      NS_LOG_FUNCTION (this);
      NS_LOG_DEBUG ("MIH LinkDetected Event");
      NS_LOG_DEBUG ("Source Mihf Identifier = " << sourceMihfId);
      NS_LOG_DEBUG ("\n from Link Identifier = " << linkDetectedInfo.GetLinkIdentifier () << ", Signal Strength = "
                    << linkDetectedInfo.GetSignalStrength ().GetValue () << ", SNR = " << linkDetectedInfo.GetSinr ()
                    << ", StationCount = " << linkDetectedInfo.GetStationCount ());  
      SupportedRates rates = linkDetectedInfo.GetSupportedRates ();
      NS_LOG_DEBUG ("Supported Rates ");
      for (uint32_t j = 0; j < rates.GetNRates (); j++)
        {
          NS_LOG_DEBUG (rates.GetRate (j)<<", ");
        }
      double signalStrength = linkDetectedInfo.GetSignalStrength ().GetValue ();
      uint32_t stationCount = linkDetectedInfo.GetStationCount ();
      if (signalStrength > m_signalStrength && stationCount <= m_stationCount)
        {
          m_signalStrength = signalStrength;
          m_stationCount = stationCount + 1;
          return true;
        }
      return false;
    }

    void
    WifiMihLinkSap::LinkUp (MihfId sourceMihfId,
                            LinkIdentifier linkIdentifier,
                            Address oldAR, 
                            Address newAR, 
                            bool ipRenewal, 
                            MobilityManagementSupport mobilitySupport) 
    {
      NS_LOG_FUNCTION (this);
      SetLinkIdentifier (linkIdentifier);
      MihLinkSap::LinkUp (sourceMihfId, linkIdentifier, oldAR, newAR, ipRenewal, mobilitySupport);
    }

    void
    WifiMihLinkSap::DoDispose (void)
    {
      NS_LOG_FUNCTION (this);
      MihLinkSap::DoDispose ();
    }
  
    LinkType
    WifiMihLinkSap::GetLinkType (void)
    {
      NS_LOG_FUNCTION (this);
      return m_linkIdentifier.GetType ();
    }
    void
    WifiMihLinkSap::SetLinkType (LinkType linkType)
    {
      NS_LOG_FUNCTION (this);
      m_linkIdentifier.SetType (linkType);
    }
    Address
    WifiMihLinkSap::GetLinkAddress (void)
    {
      NS_LOG_FUNCTION (this);
      return m_linkIdentifier.GetDeviceLinkAddress ();
    }
    void 
    WifiMihLinkSap::SetLinkAddress (Address addr)
    {
      NS_LOG_FUNCTION (this);
      m_linkIdentifier.SetDeviceLinkAddress (addr);
    }
    LinkIdentifier
    WifiMihLinkSap::GetLinkIdentifier (void)
    {
      NS_LOG_FUNCTION (this);
      return m_linkIdentifier;
    }
    void 
    WifiMihLinkSap::SetLinkIdentifier (LinkIdentifier linkIdentifier)
    {
      NS_LOG_FUNCTION (this);
      m_linkIdentifier = linkIdentifier;
    }
    Address 
    WifiMihLinkSap::GetPoAAddress (void)
    {
      NS_LOG_FUNCTION (this);
      return m_linkIdentifier.GetPoALinkAddress ();
    }
    void
    WifiMihLinkSap::SetPoAAddress (Address addr)
    {
      NS_LOG_FUNCTION (this);
      m_linkIdentifier.SetPoALinkAddress (addr);
    }
    double 
    WifiMihLinkSap::GetSignalStrength (void)
    {
      NS_LOG_FUNCTION (this);
      return m_signalStrength;
    }
    void
    WifiMihLinkSap::SetSignalStrength (double signalStrength)
    {
      NS_LOG_FUNCTION (this);
      m_signalStrength = signalStrength;
    }
    uint32_t 
    WifiMihLinkSap::GetStationCount (void)
    {
      NS_LOG_FUNCTION (this);
      return m_stationCount;
    }
    void
    WifiMihLinkSap::SetStationCount (uint32_t stationCount)
    {
      NS_LOG_FUNCTION (this);
      m_stationCount = stationCount;
    }
    MihfId
    WifiMihLinkSap::GetMihfId ()
    {
      NS_LOG_FUNCTION (this);
      return m_mihfId;
    }
    void
    WifiMihLinkSap::SetMihfId (MihfId mihfId)
    {
      NS_LOG_FUNCTION (this);
      m_mihfId = mihfId;
    }

    void 
    WifiMihLinkSap::SetSendAssocCallback (Callback<void> sendAssoc)
    {
      NS_LOG_FUNCTION (this);
      m_sendAssoc = sendAssoc;
    }

    LinkCapabilityDiscoverConfirm
    WifiMihLinkSap::CapabilityDiscover (void) 
    {
      NS_LOG_FUNCTION (this);
      return LinkCapabilityDiscoverConfirm (Status::SUCCESS,
					    (EventList::LINK_DETECTED |
					     EventList::LINK_UP |
					     EventList::LINK_DOWN |
					     EventList::LINK_PARAMETERS_REPORT),
					    (MihCommandList::LINK_GET_PARAMETERS |
					     MihCommandList::LINK_CONFIGURE_THRESHOLDS));
    }
    LinkGetParametersConfirm 
    WifiMihLinkSap::GetParameters (LinkParameterTypeList linkParametersRequest, 
				     LinkStatesRequest linkStatesRequest, 
				     LinkDescriptorsRequest descriptors)
    {
      NS_LOG_FUNCTION (this);
      return LinkGetParametersConfirm ();
    }
    LinkConfigureThresholdsConfirm 
    WifiMihLinkSap::ConfigureThresholds (LinkConfigurationParameterList configureParameters)
    {
      NS_LOG_FUNCTION (this);
      //      NS_ASSERT (m_powerUp);
      return LinkConfigureThresholdsConfirm (Status::UNSPECIFIED_FAILURE);
    }
    EventId
    WifiMihLinkSap::Action (LinkAction action, 
			      uint64_t executionDelay, 
			      Address poaLinkAddress,
			      LinkActionConfirmCallback actionConfirmCb)
    {
      return EventId ();
    }
    Ptr<DeviceStatesResponse> 
    WifiMihLinkSap::GetDeviceStates (void)
    {
      NS_LOG_FUNCTION (this);
      return Create<DeviceInformation> ("OEM = ns-3Team");
    }
  } // namespace mih
} // namespace ns3
