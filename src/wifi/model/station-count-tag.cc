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

#include "station-count-tag.h"
#include "ns3/uinteger.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (StationCountTag);

TypeId
StationCountTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::StationCountTag")
    .SetParent<Tag> ()
    .SetGroupName ("Wifi")
    .AddConstructor<StationCountTag> ()
    .AddAttribute ("StationCount", "The number of stations associated to an AP",
                   UintegerValue (0),
                   MakeUintegerAccessor (&StationCountTag::Get),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

TypeId
StationCountTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

StationCountTag::StationCountTag ()
  : m_stationCount (0)
{
}

StationCountTag::StationCountTag (uint32_t stationCount)
  : m_stationCount (stationCount)
{
}

uint32_t
StationCountTag::GetSerializedSize (void) const
{
  return sizeof (uint32_t);
}

void
StationCountTag::Serialize (TagBuffer i) const
{
  i.WriteU32 (m_stationCount);
}

void
StationCountTag::Deserialize (TagBuffer i)
{
  m_stationCount = i.ReadU32 ();
}

void
StationCountTag::Print (std::ostream &os) const
{
  os << "StationCount=" << m_stationCount;
}

void
StationCountTag::Set (uint32_t stationCount)
{
  m_stationCount = stationCount;
}

uint32_t
StationCountTag::Get (void) const
{
  return m_stationCount;
}

}
