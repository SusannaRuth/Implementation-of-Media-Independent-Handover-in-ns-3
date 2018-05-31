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

#ifndef STATION_COUNT_TAG_H
#define STATION_COUNT_TAG_H

#include "ns3/tag.h"

namespace ns3 {

class Tag;

class StationCountTag : public Tag
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  TypeId GetInstanceTypeId (void) const;

  /**
   * Create a StationCountTag with the default station count 0
   */
  StationCountTag ();

  /**
   * Create a StationCountTag with the given station count value
   * \param stationCount the given station count value
   */
  StationCountTag (uint32_t stationCount);

  uint32_t GetSerializedSize (void) const;
  void Serialize (TagBuffer i) const;
  void Deserialize (TagBuffer i);
  void Print (std::ostream &os) const;

  /**
   * Set the station count to the given value.
   *
   * \param stationCount the value of the station count to set
   */
  void Set (uint32_t stationCount);
  /**
   * Return the station count value.
   *
   * \return the station count value
   */
  uint32_t Get (void) const;


private:
  uint32_t m_stationCount;  //!< Station count value
};

}

#endif /* STATION_COUNT_TAG_H */
