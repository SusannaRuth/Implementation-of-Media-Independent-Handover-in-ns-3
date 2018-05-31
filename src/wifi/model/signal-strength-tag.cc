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

#include "signal-strength-tag.h"
#include "ns3/double.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SignalStrengthTag);

TypeId
SignalStrengthTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SignalStrengthTag")
    .SetParent<Tag> ()
    .SetGroupName ("Wifi")
    .AddConstructor<SignalStrengthTag> ()
    .AddAttribute ("SignalStrength", "The signal strength of the last packet received",
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&SignalStrengthTag::Get),
                   MakeDoubleChecker<double> ())
  ;
  return tid;
}

TypeId
SignalStrengthTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

SignalStrengthTag::SignalStrengthTag ()
  : m_signalStrength (0)
{
}

SignalStrengthTag::SignalStrengthTag (double signalStrength)
  : m_signalStrength (signalStrength)
{
}

uint32_t
SignalStrengthTag::GetSerializedSize (void) const
{
  return sizeof (double);
}

void
SignalStrengthTag::Serialize (TagBuffer i) const
{
  i.WriteDouble (m_signalStrength);
}

void
SignalStrengthTag::Deserialize (TagBuffer i)
{
  m_signalStrength = i.ReadDouble ();
}

void
SignalStrengthTag::Print (std::ostream &os) const
{
  os << "SignalStrength=" << m_signalStrength;
}

void
SignalStrengthTag::Set (double signalStrength)
{
  m_signalStrength = signalStrength;
}

double
SignalStrengthTag::Get (void) const
{
  return m_signalStrength;
}

}
