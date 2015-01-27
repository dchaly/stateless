/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 P.G. Demidov Yaroslavl State University
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
 * Author: Dmitry Chalyy <chaly@uniyar.ac.ru>
 */

#include <stdint.h>
#include <iostream>
#include "ns3/trickles-shieh-header.h"
#include "ns3/buffer.h"
#include "ns3/address-utils.h"

namespace ns3 {
    
    NS_OBJECT_ENSURE_REGISTERED (TricklesShiehHeader);
    
    uint8_t TricklesShiehHeader::m_magic = 139;
    
    TricklesShiehHeader::TricklesShiehHeader ()
    : m_tcpbase(0), m_startCwnd(0), m_ssthresh(0)
    {
    }
    
    TricklesShiehHeader::~TricklesShiehHeader ()
    {
    }
 
    SequenceNumber32 TricklesShiehHeader::GetTcpBase() const {
        return(m_tcpbase);
    }
    
    void TricklesShiehHeader::SetTcpBase(SequenceNumber32 base) {
        m_tcpbase = base;
    }
    
    uint16_t TricklesShiehHeader::GetStartCwnd() const {
        return(m_startCwnd);
    }
    
    void TricklesShiehHeader::SetStartCwnd(uint32_t cwnd) {
        m_startCwnd = cwnd;
    }
    
    uint16_t TricklesShiehHeader::GetSsthresh() const {
        return(m_ssthresh);
    }
    
    void TricklesShiehHeader::SetSsthresh(uint16_t ssthresh) {
        m_ssthresh = ssthresh;
    }
    
    uint32_t TricklesShiehHeader::GetSerializedSize (void)  const
    {
        return (8+1);
    }
    void TricklesShiehHeader::Serialize (Buffer::Iterator start)  const
    {
        Buffer::Iterator i = start;
        i.WriteU8(m_magic);
        i.WriteHtonU32 (m_tcpbase.GetValue());
        i.WriteHtonU16 (m_startCwnd);
        i.WriteHtonU16 (m_ssthresh);
    }
    uint32_t TricklesShiehHeader::Deserialize (Buffer::Iterator start)
    {
        Buffer::Iterator i = start;
        if (i.ReadU8() != m_magic) {
            return 0;
        }
        m_tcpbase = i.ReadNtohU32();
        m_startCwnd = i.ReadNtohU16();
        m_ssthresh = i.ReadNtohU16();
        return GetSerializedSize ();
    }
    
    TypeId TricklesShiehHeader::GetTypeId (void) {
        static TypeId tid = TypeId ("ns3::TricklesShiehHeader")
        .SetParent<Header> ()
        .AddConstructor<TricklesShiehHeader> ()
        ;
        return tid;
    }
    
    TypeId TricklesShiehHeader::GetInstanceTypeId (void) const {
        return GetTypeId();
    }
    
    void TricklesShiehHeader::Print (std::ostream &os)  const
    {
        os << "TCPBase=" << m_tcpbase << " startCwnd=" << m_startCwnd << " ssthresh=" << m_ssthresh;
    }

} // namespace ns3
