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

#ifndef   	WIFI_MIH_LINK_SAP_H
#define   	WIFI_MIH_LINK_SAP_H

#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include "mihf-id.h"
#include "mih-link-sap.h"

namespace ns3 {
  namespace mih {
    class WifiMihLinkSap : public MihLinkSap {
    public:
      static TypeId GetTypeId (void);
      WifiMihLinkSap (void);
      virtual ~WifiMihLinkSap (void);
      bool LinkDetected (MihfId sourceMihfId, 
                         LinkDetectedInformation linkDetectedInfo);
      virtual void LinkUp (MihfId sourceMihfId, 
                           LinkIdentifier linkIdentifier, 
                           Address oldAR, 
                           Address newAR,
                           bool ipRenewal,
                           MobilityManagementSupport mobilitySupport);
      virtual LinkType GetLinkType (void);
      void SetLinkType (LinkType linkType);
      virtual Address GetLinkAddress (void);
      void SetLinkAddress (Address addr);
      virtual LinkIdentifier GetLinkIdentifier (void);
      virtual void SetLinkIdentifier (LinkIdentifier linkIdentifier);
      virtual Address GetPoAAddress (void);
      virtual void SetPoAAddress (Address addr);
      virtual double GetSignalStrength (void);
      virtual void SetSignalStrength (double signalStrength);
      virtual uint32_t GetStationCount (void);
      virtual void SetStationCount (uint32_t stationCount);
      
      virtual LinkCapabilityDiscoverConfirm CapabilityDiscover (void);
      virtual LinkGetParametersConfirm GetParameters (LinkParameterTypeList linkParametersRequest, 
					      LinkStatesRequest linkStatesRequest, 
					      LinkDescriptorsRequest descriptors);
      virtual LinkConfigureThresholdsConfirm ConfigureThresholds (LinkConfigurationParameterList configureParameters);
      virtual EventId Action (LinkAction action, 
                              uint64_t executionDelay, 
                              Address poaLinkAddress,
                              LinkActionConfirmCallback actionConfirmCb);
      MihfId GetMihfId ();
      void SetMihfId (MihfId mihfId);
      void SetSendAssocCallback (Callback<void> sendAssoc);

      //void Run (void);
      
      //void Stop (void);

    protected:
      //typedef void (SimpleMihLinkSap::*ActionTrigger) (void);
      //void ScheduleNextTrigger (void);
      //void TriggerLinkDetected (void);
      //void TriggerLinkUp (void);
      //void TriggerLinkDown (void);

      //void DoAction (LinkAction action,
      //               Address poaLinkAddress,
      //               LinkActionConfirmCallback actionConfirmCb);
        
      virtual void DoDispose (void);
      virtual Ptr<DeviceStatesResponse> GetDeviceStates (void);
      LinkIdentifier m_linkIdentifier;
      //EventId m_nextEventId;
      //Ptr<UniformRandomVariable> m_eventTriggerInterval;// rng for next Tx
						        // Time
      //Ptr<UniformRandomVariable> m_rngChoice;   // rng for next action to perform
				                // in trigger
    
      MihfId m_mihfId;
      Callback<void> m_sendAssoc;
      double m_signalStrength;
      uint32_t m_stationCount;
      //bool m_powerUp;

    };
  } // namespace mih
} // namespace ns3

#endif 	    /* !SIMPLE_MIH_LINK_SAP_H */

