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
#include "trickles-sack.h"

NS_LOG_COMPONENT_DEFINE ("TricklesSack");

#define MIN(a,b) ((a)<(b))?(a):(b)
#define MAX(a,b) ((a)>(b))?(a):(b)

namespace ns3 {
    
    TypeId
    TricklesSack::GetTypeId (void)
    {
        static TypeId tid = TypeId ("ns3::TricklesSack")
        .SetParent<Object> ()
        .AddConstructor<TricklesSack> ()
        ;
        return tid;
    }
    
    TricklesSack::TricklesSack ()
    {
        NS_LOG_FUNCTION (this);
    }
    
    TricklesSack::~TricklesSack ()
    {
        NS_LOG_FUNCTION (this << m_blocks.size());
    }
    
    SackConstIterator TricklesSack::firstBlock() const {
        return m_blocks.begin();
    }
    
    bool TricklesSack::isEnd(SackConstIterator i) const {
        return (i==m_blocks.end());
    }
    
    uint32_t TricklesSack::DataSize() const {
        SackConstIterator i = m_blocks.begin();
        uint32_t result = 0;
        while (i != m_blocks.end()) {
            result += i->second-i->first;
            i++;
        }
        return(result);
    }
    
    uint32_t TricklesSack::numBlocks() const {
        return(m_blocks.size());
    }
    
    bool TricklesSack::AddBlock(SequenceNumber32 from, SequenceNumber32 to) {
        if (from>=to) return(0);
        if (m_blocks.size() == 0) {
            m_blocks[from] = to;
            return(1);
        }
        SackIterator i = m_blocks.begin();
        SequenceNumber32 bFrom = from, bTo = to;
//        std::cout << "*[" << bFrom << ":" << bTo << "]\n";
        while (i != m_blocks.end()) {
//            std::cout << "current[" << i->first.GetValue() << ":" << i->second.GetValue() << "]\n";
            if (bTo<i->first) {
                m_blocks[bFrom] = bTo;
//                std::cout << "inserted[" << bFrom << ":" << bTo << "]\n";
                bFrom = bTo;
                break;
            }
            if (bFrom>i->second) { /* std::cout<<"next\n"; */ ++i; continue; }
            if ((bFrom<=i->second) && (bTo>=i->first)) {
                bFrom = (bFrom<i->first)?bFrom:i->first;
                bTo = (bTo>i->second)?bTo:i->second;
//                std::cout << "^[" << bFrom << ":" << bTo << "]\n";
                m_blocks.erase(i++);
                continue;
            }
            ++i;
        }
//        std::cout << "-[" << bFrom << ":" << bTo << "]\n";
        if (bFrom != bTo) m_blocks[bFrom] = bTo;
        return(1);
    }
    
    bool TricklesSack::AckBlock(SequenceNumber32 from, SequenceNumber32 to) {
        if (from>=to) return(0);
        if (m_blocks.size() == 0) return(0);
        SackIterator i = m_blocks.begin();
//        std::cout << "-[" << from << ":" << to << "]\n";
        while (i != m_blocks.end()) {
            SequenceNumber32 fi = i->first;
            SequenceNumber32 ti = i->second;
//            std::cout << "?[" << fi << ":" << ti << "]\n";
            if ((from<ti) && (to>fi)) {
                // nbmin = min(max(from, first), first))
                SequenceNumber32 nbmin = MIN(MAX(from, fi), MIN(to, ti));
                // nbmax = min(max(from, first), min(to, second))
                SequenceNumber32 nbmax = MIN(MAX(from, ti), MIN(to, ti));//MIN(MAX(from, fi), MIN(to, ti));//(maxfromfi<mintoti)?maxfromfi:mintoti;
//                std::cout << "*[" << nbmin << ":" << nbmax << "]\n";
                m_blocks.erase(i++);
/*                if ((nbmin==from) && (nbmax==to)) {
                    m_blocks[fi] = nbmin;
                    m_blocks[nbmax] = ti;
                    break;
                }
                if (nbmin != nbmax) {
                    m_blocks[nbmin] = nbmax;
                    i = m_blocks.find(nbmin);
                } */
                if (fi<nbmin) m_blocks[fi] = nbmin;
                if (nbmax<ti) m_blocks[nbmax] = ti;
                i = m_blocks.begin(); // This is not a good decision
            } else ++i;
        }
        return(1);
    }
    
    void TricklesSack::Print(std::ostream &os) const {
        if (m_blocks.size()==0) {
            os << "[0]";
            return;
        }
        SackConstIterator i = m_blocks.begin();
        while (i != m_blocks.end()) {
            os << "[" << i->first.GetValue() << ":" << i->second.GetValue() << "]";
            i++;
        }
    }
    
    void TricklesSack::Clear() {
        m_blocks.clear();
    }
    
} //namepsace ns3
