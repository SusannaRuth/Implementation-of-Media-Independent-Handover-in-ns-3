/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 University of Washington
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
 */

#ifndef WIFI_MIH_LINK_SAP_HELPER_H
#define WIFI_MIH_LINK_SAP_HELPER_H

#include "ns3/ptr.h"
#include "ns3/node.h"
#include "ns3/wifi-mih-link-sap.h"

namespace ns3 {

class Node;

/**
 * \brief Helper class that adds ns3::mih::WifiMihLinkSap objects
 */
class WifiMihLinkSapHelper
{
public:
  //virtual ~WifiMihLinkSapHelper ();
  /**
   * \brief Constructor.
   */
  WifiMihLinkSapHelper ();

  /**
   * \brief Construct an WifiMihLinkSapHelper from another previously 
   * initialized instance (Copy Constructor).
   */
  WifiMihLinkSapHelper (const WifiMihLinkSapHelper &);

  /**
   * \param first lower bound on the set of nodes on which a WifiMihLinkSap must be created
   * \param last upper bound on the set of nodes on which a WifiMihLinkSap must be created
   * \returns a newly-created WifiMihLinkSap, aggregated to the set of nodes
   */
  //virtual Ptr<WifiMihLinkSap> Install (NodeContainer::Iterator first,
  //                                     NodeContainer::Iterator last) const;
  /**
   * \param c the set of nodes on which a WifiMihLinkSap must be created
   * \returns a newly-created WifiMihLinkSap, aggregated to the set of nodes
   */
  //virtual Ptr<WifiMihLinkSap> Install (NodeContainer c) const;

  /**
   * \param node the node on which a WifiMihLinkSap must be created
   * \returns a newly-created WifiMihLinkSap, aggregated to the node
   */
  virtual Ptr<mih::WifiMihLinkSap> Install (Ptr<Node> node) const;

};

} // namespace ns3

#endif /* WIFI_MIH_LINK_SAP_HELPER_H */

