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

#include "ns3/packet.h"
#include "ns3/fatal-error.h"
#include "ns3/log.h"
#include "trickles-buffer.h"

NS_LOG_COMPONENT_DEFINE ("TricklesBuffer");

namespace ns3 {
    
    TypeId
    TricklesBuffer::GetTypeId (void)
    {
        static TypeId tid = TypeId ("ns3::TricklesBuffer")
        .SetParent<Object> ()
        .AddConstructor<TricklesBuffer> ()
        ;
        return tid;
    }
    
    TricklesBuffer::TricklesBuffer (uint32_t maxSize)
    : m_freeBytes(maxSize), m_availBytes(0), m_NextRqSeq(0), m_maxSize(maxSize)
    {
    }
    
    TricklesBuffer::~TricklesRxBuffer ()
    {
    }
    
    uint32_t
    TricklesBuffer::MaxBufferSize (void) const
    {
        return m_maxBuffer;
    }
    
    void
    TricklesBuffer::SetMaxBufferSize (uint32_t s)
    {
        m_maxBuffer = s;
    }
    
    uint32_t
    TricklesBuffer::Available () const
    {
        return m_availBytes;
    }
    
    uint32_t
    TricklesBuffer::FreeBytes () const
    {
        return m_freeBytes;
    }
    
    uint32_t
    TricklesBuffer::SetNextRqSeq(uint32_t s) const {
        m_nextRxSeq = s;
    }
    
    Ptr<Packet> TricklesBuffer::NextRequest(uint16_t reqSize) {
        if (!reqSize) return 0;
        if (!m_freeBytes) return 0;
        
        uint16_t maxReqSize = std::min(m_freeBytes, reqSize);
        Ptr<Packet> p = Create<Packet>;
        TricklesHeader trh;
        trh.SetPacketType(REQUEST);
        trh.SetRequestSize(maxReqSize);
        p->AddHeader(trh);
        // Let the last request be the large one
        BufIterator i = m_data.begin();
        BufIterator prev = m_data.end();
        while (i != m_data.end()) {
            prev = i;
            ++i;
        }
        if (prev != m_data.end()) {
            TricklesHeader t;
            prev->second->PeekHeader(t);
            if (t.GetPacketType == REQUEST) {
                prev->second->RemoveHeader(t);
                t.SetRequestSize(t.GetRequestSize()+maxReqSize);
                prev->second->AddHeader(t);
            } else {
                Ptr<Packet> dp = Create<Packet>(p);
                m_data[m_nextRqSeq] = dp;
            }
        } else {
            Ptr<Packet> dp = Create<Packet>(p);
            m_data[m_nextRqSeq] = dp;
        }
        m_nextRqSeq += maxReqSize;
        m_freeBytes -= maxReqSize;
        return(p);
    }
    
    uint16_t TricklesBuffer::ProcessContinuation(Ptr<Packet> p, TcpHeader const& tcph) {
        TricklesHeader trh;
        if (!p->RemoveHeader(trh)) return 0;
        if (trh->GetPacketType() != CONTINUATION) return 0;
        SequenceNumber32 pFirst = tcph.GetSequenceNumber();
        SequenceNumber32 pLast = packetFirst+SequenceNumber32(p->GetSize());
        BufIterator i = m_data.begin();
        SequenceNumber32 iFirst = SequenceNumber32(0);
        SequenceNumber32 iLast = SequenceNumber32(0);
        while ((i != m_data.end()) && (p->GetSize())) {
            TricklesHeader ih;
            i->second->PeekHeader(ih);
            SequenceNumber32 iFirst = i->first;
            SequenceNumber32 iLast = iFirst+SequenceNumber32(ih.GetRequestSize());
            if ((ih.GetPacketType() == CONTINUATION) && (pFirst<Last)) {
                
            }
            if ((pFirst<iLast) || (pLast>iFirst)) {
                if (ih.GetPacketType() == CONTINUATION) {
                    // Incoming packet overlaps with continuation
                    p = p->CreateFragment(iLast-pFirst, p->GetSize()-(iLast-pFirst));
                    pFirst += iLast-pFirst;
                } else {
                    // Incoming packet overlaps with request
                    i->second->RemoveHeader(ih);
                    Ptr<Packet> lp = 0, rp = 0;
                    SequenceNumber32 lseq, rseq;
                    // Left non-overlap
                    if (iFirst<pFirst) {
                        TricklesHeader lh;
                        lp = Create<Packet>;
                        lseq = iFirst;
                        lh.SetRequestSize(pFirst-iFirst);
                        lh.SetPacketType(REQUEST);
                        lp->AddHeader(lh);
                    }
                    // Right non-overlap
                    if (pLast<iLast) {
                        TricklesHeader rh;
                        rp = Create<Packet>;
                        rseq = pLast;
                        rh.SetRequestSize(iLast-pLast);
                        rh.SetPacketType(REQUEST);
                        rp->AddHeader(rh);
                    }
                    BufIterator j = i; ++j;
                    m_data.erase(i);
                    if (lp) m_data[lseq] = lp;
                    if (rp) {
                        m_data[rseq] = rp;
                        m_data[pFirst] = p;
                    }
                    i = j;
                }
            }
            ++i;
        }
    }
    
    bool
    TricklesBuffer::Add (Ptr<Packet> p)
    {
        TcpHeader tcph;
        p->RemoveHeader(tcph);
        TricklesHeader trh;
        if (!p->RemoveHeader(trh)) return(0);
        
        NS_LOG_FUNCTION (this << p << tcph << trh);
        
        uint32_t pktSize = p->GetSize ();
        SequenceNumber32 headSeq = tcph.GetSequenceNumber ();
        SequenceNumber32 tailSeq = headSeq + SequenceNumber32 ((trh.GetPacketType()==REQUEST)?trh.GetRequestSize():pktSize);
        NS_LOG_LOGIC ("Add pkt " << p << " len=" << pktSize << " seq=" << headSeq);
        
        if (trh.GetPacketType()==REQUEST) {
            // REQUEST packet
            // Trim request to fit Rx window
            SequenceNumber32 from = m_data.size()?m_data.begin()->first:m_nextRxSeq;
            SequenceNumber32 to = from+m_maxSize;
            if (headSeq<from) headSeq = from;
            if (tailSeq>to) tailSeq = to;
            if (headSeq>=tailSeq) return(0); // Request is out of the window
            
            BufIterator i = m_data.begin();
            while (i!=m_data.end()) {
            }
        } else {
            // CONTINUATION packet
        }
        // Trim packet to fit Rx window specification
        if (headSeq < m_nextRxSeq) headSeq = m_nextRxSeq;
        if (m_data.size ())
        {
            SequenceNumber32 maxSeq = m_data.begin ()->first + SequenceNumber32 (m_maxBuffer);
            if (maxSeq < tailSeq) tailSeq = maxSeq;
            if (tailSeq < headSeq) headSeq = tailSeq;
        }
        // Remove overlapped bytes from packet
        BufIterator i = m_data.begin ();
        while (i != m_data.end () && i->first <= tailSeq)
        {
            SequenceNumber32 lastByteSeq = i->first + SequenceNumber32 (i->second->GetSize ());
            if (lastByteSeq > headSeq)
            {
                if (i->first > headSeq && lastByteSeq < tailSeq)
                { // Rare case: Existing packet is embedded fully in the new packet
                    m_size -= i->second->GetSize ();
                    m_data.erase (i++);
                    continue;
                }
                if (i->first <= headSeq)
                { // Incoming head is overlapped
                    headSeq = lastByteSeq;
                }
                if (lastByteSeq >= tailSeq)
                { // Incoming tail is overlapped
                    tailSeq = i->first;
                }
            }
            ++i;
        }
        // We now know how much we are going to store, trim the packet
        if (headSeq >= tailSeq)
        {
            NS_LOG_LOGIC ("Nothing to buffer");
            return false; // Nothing to buffer anyway
        }
        else
        {
            uint32_t start = headSeq - tcph.GetSequenceNumber ();
            uint32_t length = tailSeq - headSeq;
            p = p->CreateFragment (start, length);
            NS_ASSERT (length == p->GetSize ());
        }
        // Insert packet into buffer
        NS_ASSERT (m_data.find (headSeq) == m_data.end ()); // Shouldn't be there yet
        m_data [ headSeq ] = p;
        NS_LOG_LOGIC ("Buffered packet of seqno=" << headSeq << " len=" << p->GetSize ());
        // Update variables
        m_size += p->GetSize ();      // Occupancy
        for (BufIterator i = m_data.begin (); i != m_data.end (); ++i)
        {
            if (i->first < m_nextRxSeq)
            {
                continue;
            }
            else if (i->first > m_nextRxSeq)
            {
                break;
            };
            m_nextRxSeq = i->first + SequenceNumber32 (i->second->GetSize ());
            m_availBytes += i->second->GetSize ();
        }
        NS_LOG_LOGIC ("Updated buffer occupancy=" << m_size << " nextRxSeq=" << m_nextRxSeq);
        if (m_gotFin && m_nextRxSeq == m_finSeq)
        { // Account for the FIN packet
            ++m_nextRxSeq;
        };
        return true;
    }
    
    Ptr<Packet>
    TcpRxBuffer::Extract (uint32_t maxSize)
    {
        NS_LOG_FUNCTION (this << maxSize);
        
        uint32_t extractSize = std::min (maxSize, m_availBytes);
        NS_LOG_LOGIC ("Requested to extract " << extractSize << " bytes from TcpRxBuffer of size=" << m_size);
        if (extractSize == 0) return 0;  // No contiguous block to return
        NS_ASSERT (m_data.size ()); // At least we have something to extract
        Ptr<Packet> outPkt = Create<Packet> (); // The packet that contains all the data to return
        BufIterator i;
        while (extractSize)
        { // Check the buffered data for delivery
            i = m_data.begin ();
            NS_ASSERT (i->first <= m_nextRxSeq); // in-sequence data expected
            // Check if we send the whole pkt or just a partial
            uint32_t pktSize = i->second->GetSize ();
            if (pktSize <= extractSize)
            { // Whole packet is extracted
                outPkt->AddAtEnd (i->second);
                m_data.erase (i);
                m_size -= pktSize;
                m_availBytes -= pktSize;
                extractSize -= pktSize;
            }
            else
            { // Partial is extracted and done
                outPkt->AddAtEnd (i->second->CreateFragment (0, extractSize));
                m_data[i->first + SequenceNumber32 (extractSize)] = i->second->CreateFragment (extractSize, pktSize - extractSize);
                m_data.erase (i);
                m_size -= extractSize;
                m_availBytes -= extractSize;
                extractSize = 0;
            }
        }
        if (outPkt->GetSize () == 0)
        {
            NS_LOG_LOGIC ("Nothing extracted.");
            return 0;
        }
        NS_LOG_LOGIC ("Extracted " << outPkt->GetSize ( ) << " bytes, bufsize=" << m_size
                      << ", num pkts in buffer=" << m_data.size ());
        return outPkt;
    }
    
} //namepsace ns3
