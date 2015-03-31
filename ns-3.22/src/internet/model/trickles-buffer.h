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

#ifndef TRICKLES_BUFFER_H
#define TRICKLES_BUFFER_H

#include <map>
#include "ns3/traced-value.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/sequence-number.h"
#include "ns3/ptr.h"
#include "tcp-header.h"
#include "trickles-header.h"

namespace ns3 {
    class Packet;
    
    /**
     * \ingroup trickles
     *
     * \brief class for the buffer that keeps the data from lower layer, i.e.
     *        TricklesL4Protocol, sent to the application
     */
    class TricklesBuffer : public Object
    {
    public:
        static TypeId GetTypeId (void);
        TricklesBuffer (uint32_t maxSize);
        virtual ~TricklesBuffer ();

        uint32_t MaxBufferSize (void) const;
        void SetMaxBufferSize (uint32_t s);
        uint32_t Available () const;
        uint32_t FreeBytes() const;
        void SetNextRqSeq(uint32_t s) const;
        /**
         * Insert a packet into the buffer and update the availBytes counter to
         * reflect the number of bytes ready to send to the application. This
         * function handles overlap by triming the head of the inputted packet and
         * removing data from the buffer that overlaps the tail of the inputted
         * packet
         *
         * \return True when success, false otherwise.
         */
        bool Add (Ptr<Packet> p);
        
        /**
         * Extract data from the head of the buffer as indicated by nextRxSeq.
         * The extracted data is going to be forwarded to the application.
         */
        Ptr<Packet> Extract (uint32_t maxSize);
        Ptr<Packet> NextRequest(uint16_t reqSize);
    public:
        typedef std::map<SequenceNumber32, Ptr<Packet> >::iterator BufIterator;
    private:
        uint32_t m_availBytes;                     //< Number of bytes available to read, i.e. contiguous block at head
        uint32_t m_maxSize;
        uint32_t m_freeBytes;
        SequenceNumber32 m_nextRqSeq; // Next request sequence number
        std::map<SequenceNumber32, Ptr<Packet> > m_data;
        //< Corresponding data (may be null)
    };
    
} //namepsace ns3

#endif /* TCP_RX_BUFFER_H */
