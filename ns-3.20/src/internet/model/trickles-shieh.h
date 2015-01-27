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

#ifndef TRICKLES_SHIEH_H
#define TRICKLES_SHIEH_H

#include <stdint.h>
#include <queue>
#include "ns3/callback.h"
#include "ns3/traced-callback.h"
#include "ns3/socket.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-interface.h"
#include "icmpv4.h"
#include "trickles-socket.h"
#include "tcp-tx-buffer.h"
#include "tcp-rx-buffer.h"
#include "rtt-estimator.h"
#include "trickles-socket-base.h"

namespace ns3 {
    
    class Ipv4EndPoint;
    class Ipv6EndPoint;
    class Node;
    class Packet;
    class TricklesL4Protocol;
    class TricklesSocketBase;
    
    /**
     * \ingroup trickles
     * \brief A sockets interface to the original Trickles protocol model
     *
     * This class subclasses ns3::TricklesSocketBase, and provides a socket interface
     * to ns3's implementation of Trickles.
     */
    
    class TricklesShieh : public TricklesSocketBase
    {
    public:
        static TypeId GetTypeId (void);
        /**
         * Create an unbound trickles socket.
         */
        TricklesShieh ();
        TricklesShieh(const TricklesShieh &sock);
        virtual ~TricklesShieh ();
        
    protected:
        virtual void ProcessTricklesPacket(Ptr<Packet> packet, TricklesHeader &th);
        virtual void NewRequest();
    private:
        void TrySendDelayed();
        void ProcessShiehContinuation(Ptr<Packet> packet, TricklesHeader th, TricklesShiehHeader trh);
        void ProcessShiehRequest(Ptr<Packet> packet, TricklesHeader th, TricklesShiehHeader trh);
        uint16_t tcpCwnd(SequenceNumber32 tcpBase, uint16_t cwnd, uint16_t ssthresh, SequenceNumber32 k) const;
        void DelayPacket(Ptr<Packet> packet);
        void SendDelayedPackets();
        void ReTxTimeout();
        uint32_t GetStartCwnd() const { return m_cwnd; };
        void SetStartCwnd(uint32_t i) { m_cwnd = i; };
        uint32_t GetStartSsthresh() const { return m_ssthresh; };
        void SetStartSsthresh(uint32_t i) { m_ssthresh = i; };
        SequenceNumber32 m_tcpBase;
        uint32_t m_cwnd;
        uint32_t m_ssthresh;
        std::list<Ptr<Packet> > m_delayed;
        EventId m_retxEvent;
        bool m_started;
    };
    
} // namespace ns3

#endif /* TRICKLES_SHIEH_H */
