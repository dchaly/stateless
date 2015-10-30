/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 P.G. Demidov Yaroslavl State University
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
 * Author: Dmitry Ju. Chalyy (chaly@uniyar.ac.ru)
 */

#include <stdint.h>
#include <iostream>
#include <iomanip>
#include "ns3/log.h"
#include "trickles-header.h"
#include "ns3/buffer.h"
#include "ns3/address-utils.h"
#include "ns3/sequence-number.h"

NS_LOG_COMPONENT_DEFINE ("TricklesHeader");

namespace ns3 {
    
    NS_OBJECT_ENSURE_REGISTERED (TricklesHeader);
    
    uint8_t TricklesHeader::m_magic = 145;
    
    TricklesHeader::TricklesHeader ()
    : m_packetType(REQUEST), m_requestSize(0), m_trickleNo(SequenceNumber32(1)), m_parentNo(SequenceNumber32(0)), m_recovery(NO_RECOVERY), m_firstLoss(SequenceNumber32(0))
    {
        NS_LOG_FUNCTION (this);
    }
    
    TricklesHeader::~TricklesHeader ()
    {
        NS_LOG_FUNCTION (this);
    }
    
    Trickle_t TricklesHeader::GetPacketType() const {
        return(m_packetType);
    }
    
    void TricklesHeader::SetPacketType(Trickle_t t) {
        m_packetType = t;
    }
    
    uint16_t TricklesHeader::GetRequestSize() const {
        return(m_requestSize);
    }
    
    void TricklesHeader::SetRequestSize(uint16_t reqSize) {
        NS_LOG_FUNCTION(this << reqSize);
        m_requestSize = reqSize;
    }
    
    SequenceNumber32 TricklesHeader::GetTrickleNumber() const {
        return(m_trickleNo);
    }

    void TricklesHeader::SetTrickleNumber(SequenceNumber32 i) {
        NS_LOG_FUNCTION (this << i);
        m_trickleNo = i;
    }
    
    void TricklesHeader::SetTrickleNumber(uint32_t i) {
        m_trickleNo = SequenceNumber32(i);
    }
    
    SequenceNumber32 TricklesHeader::GetParentNumber() const {
        return(m_parentNo);
    }
    
    void TricklesHeader::SetParentNumber(SequenceNumber32 i) {
        m_parentNo = i;
    }
    
    void TricklesHeader::SetParentNumber(uint32_t i) {
        m_parentNo = SequenceNumber32(i);
    }

    
    Recovery_t TricklesHeader::IsRecovery() const {
        return(m_recovery);
    }
    
    void TricklesHeader::SetRecovery(Recovery_t rec) {
        m_recovery = rec;
    }
    
    SequenceNumber32 TricklesHeader::GetTSVal() const {
        return(m_tsval);
    }
    
    void TricklesHeader::SetTSVal(SequenceNumber32 tsval) {
        m_tsval = tsval;
    }
    
    SequenceNumber32 TricklesHeader::GetTSEcr() const {
        return(m_tsecr);
    }
    
    void TricklesHeader::SetTSEcr(SequenceNumber32 tsecr) {
        m_tsecr = tsecr;
    }
    
    Time TricklesHeader::GetRTT() const {
        return(m_rtt);
    }
    
    void TricklesHeader::SetRTT(Time tval) {
        m_rtt = tval;
    }
    
    TricklesSack TricklesHeader::GetSacks() const {
        return(m_sacks);
    }
    
    void TricklesHeader::SetSacks(TricklesSack newsack) {
        m_sacks = newsack;
    }
    
    SequenceNumber32 TricklesHeader::GetFirstLoss() const {
        return(m_firstLoss);
    }
    
    void TricklesHeader::SetFirstLoss(SequenceNumber32 firstLoss) {
        m_firstLoss = firstLoss;
    }

    
    uint32_t TricklesHeader::GetSerializedSize (void)  const
    {
        return 1+24+m_sacks.numBlocks()*8;
    }
    void TricklesHeader::Serialize (Buffer::Iterator start)  const
    {
        Buffer::Iterator i = start;
        i.WriteU8(m_magic);
        i.WriteHtonU16 (m_requestSize);
        i.WriteHtonU32 (m_trickleNo.GetValue());
        i.WriteHtonU32 (m_parentNo.GetValue());
        uint16_t t = 0; // The variable contains m_packetType and m_Recovery;
        t = ((m_packetType + (m_recovery << 1)) << 8)+m_sacks.numBlocks();
        i.WriteHtonU16(t);
        i.WriteHtonU32(m_tsval.GetValue());
        i.WriteHtonU32(m_tsecr.GetValue());
        uint32_t tv = m_rtt.GetMilliSeconds();
        i.WriteHtonU32(tv);
        //NS_LOG_FUNCTION(this << tv);
        SackConstIterator s = m_sacks.firstBlock();
        for (;!m_sacks.isEnd(s); s++) {
            i.WriteHtonU32(s->first.GetValue());
            i.WriteHtonU32(s->second.GetValue());
        }
    }
    uint32_t TricklesHeader::Deserialize (Buffer::Iterator start)
    {
        Buffer::Iterator i = start;
        if (i.ReadU8() != m_magic) {
            return 0;
        }
        m_requestSize = i.ReadNtohU16();
        m_trickleNo = i.ReadNtohU32();
        m_parentNo = i.ReadNtohU32();
        uint16_t t = i.ReadNtohU16();
        m_packetType = static_cast<Trickle_t>((t >> 8) & 0x1);
        m_recovery = static_cast<Recovery_t>(t >> 9);
        uint32_t ts;
        ts = i.ReadNtohU32();
        m_tsval = SequenceNumber32(ts);
        ts = i.ReadNtohU32();
        m_tsecr = SequenceNumber32(ts);
        ts = i.ReadNtohU32();
        m_rtt = MilliSeconds(ts);
        //NS_LOG_FUNCTION(this << ts);
        t = t & 0x00FF;
        for (;t;t--) {
            uint32_t f, s;
            f = i.ReadNtohU32();
            s = i.ReadNtohU32();
            m_sacks.AddBlock(SequenceNumber32(f), SequenceNumber32(s));
        }
        return GetSerializedSize ();
    }
    
    TypeId TricklesHeader::GetTypeId (void) {
        static TypeId tid = TypeId ("ns3::TricklesHeader")
        .SetParent<Header> ()
        .AddConstructor<TricklesHeader> ()
        ;
        return tid;
    }
    TypeId TricklesHeader::GetInstanceTypeId (void) const {
        return GetTypeId();
    }
    
    void TricklesHeader::Print (std::ostream &os)  const
    {
//        os << "Trickles:";
        if (m_packetType == REQUEST) {
            os << "REQUEST";
        } else
            if (m_packetType == CONTINUATION) {
                os << "CONTINUATION";
            } else os << "UNKNOWN";
        os << " Trickle=" << m_trickleNo;
        os << " Parent=" << m_parentNo;
        os << " FirstLoss=" << m_firstLoss;
        os << " SACKS="; m_sacks.Print(os);
        os << " Request=" << m_requestSize;
        os << " Recovery=";
        switch (m_recovery) {
            case NO_RECOVERY:
                os << "n";
                break;
            case FAST_RETRANSMIT:
                os << "f";
                break;
            case RTO_TIMEOUT:
                os << "r";
                break;
            default:
                break;
        }
        os << " tsval=" << m_tsval << " tsecr=" << m_tsecr;
        os << " RTT=" << m_rtt.GetMilliSeconds() << " ";
    }

} // namespace ns3
