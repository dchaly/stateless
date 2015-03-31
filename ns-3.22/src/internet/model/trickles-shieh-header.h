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

#ifndef TRICKLES_SHIEH_HEADER_H
#define TRICKLES_SHIEH_HEADER_H

#include <stdint.h>
#include "ns3/header.h"
#include "ns3/buffer.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/sequence-number.h"

namespace ns3 {
    /**
     * \ingroup tricklestp
     * \class TricklesShiehHeader
     * \brief Класс, моделирующий поля из оригинального протокола Trickles в пакете
     *
     * Класс содержит поля, которые находятся в передаваемых пакетах и необходимых для функционирования протокола Trickles, предложенного A.Shieh et al.
     *
     * В совокупности классы ns3::TricklesHeader и этот класс реализуют все поля, необходимые для функционирования протокола Trickles, предложенного A. Shieh et al.
     */
    
    class TricklesShiehHeader : public Header
    {
    private:
        static uint8_t m_magic;
    public:
        TricklesShiehHeader ();
        virtual ~TricklesShiehHeader ();
        
        static TypeId GetTypeId (void);
        virtual TypeId GetInstanceTypeId (void) const;
        virtual void Print (std::ostream &os) const;
        virtual uint32_t GetSerializedSize (void) const;
        virtual void Serialize (Buffer::Iterator start) const;
        virtual uint32_t Deserialize (Buffer::Iterator start);
        SequenceNumber32 GetTcpBase() const;
        void SetTcpBase(SequenceNumber32 base);
        uint16_t GetStartCwnd() const;
        void SetStartCwnd(uint32_t cwnd);
        uint16_t GetSsthresh() const;
        void SetSsthresh(uint16_t ssthresh);
    private:
        /**
         * \brief Значение параметра TCPBase
         */
        SequenceNumber32 m_tcpbase;
        /**
         * \brief Значение параметра startCwnd - cwnd при отправке пакета с номером TCPBase
         */
        uint16_t m_startCwnd;
        /**
         * \brief Значение параметра ssthresh
         */
        uint16_t m_ssthresh;
    };
    
#undef LOG_TRICKLES_SHIEH_PACKET
#define LOG_TRICKLES_SHIEH_PACKET(p) \
if (p) { \
TricklesShiehHeader th; \
if (p->PeekHeader(th)) th.Print(std::clog); else std::clog << " No TricklesShiehHeader "; \
} else { \
std::clog << " Packet is NULL "; }

#undef LOG_TRICKLES_SHIEH_PACKET_
#define LOG_TRICKLES_SHIEH_PACKET_(p) \
if (p) { \
TricklesHeader th; \
if (p->RemoveHeader(th)) { LOG_TRICKLES_SHIEH_PACKET(p); p->AddHeader(th); } \
else LOG_TRICKLES_SHIEH_PACKET(p); \
}
    
#undef LOG_TRICKLES_SHIEH_HEADER
#define LOG_TRICKLES_SHIEH_HEADER(t) \
t.Print(std::clog);
    
} // namespace ns3

#endif /* TRICKLES_SHIEH_HEADER */
