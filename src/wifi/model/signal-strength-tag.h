/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#ifndef SIGNAL_STRENGTH_TAG_H
#define SIGNAL_STRENGTH_TAG_H

#include "ns3/tag.h"

namespace ns3 {

class Tag;

class SignalStrengthTag : public Tag
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  TypeId GetInstanceTypeId (void) const;

  /**
   * Create a SignalStrengthTag with the default signal strength 0
   */
  SignalStrengthTag ();

  /**
   * Create a SignalStrengthTag with the given signal strength value
   * \param signalStrength the given signal strength value
   */
  SignalStrengthTag (double signalStrength);

  uint32_t GetSerializedSize (void) const;
  void Serialize (TagBuffer i) const;
  void Deserialize (TagBuffer i);
  void Print (std::ostream &os) const;

  /**
   * Set the signal strength to the given value.
   *
   * \param signalStrength the value of the signal strength to set
   */
  void Set (double signalStrength);
  /**
   * Return the signal strength value.
   *
   * \return the signal strength value
   */
  double Get (void) const;


private:
  double m_signalStrength;  //!< Signal Strength value
};

}

#endif /* SIGNAL_STRENGTH_TAG_H */
