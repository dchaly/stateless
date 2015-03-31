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
#include <list>
#include <limits>
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/node.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv6-route.h"
#include "ns3/ipv4.h"
#include "ns3/ipv6.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv6-routing-protocol.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/ipv4-packet-info-tag.h"
#include "ns3/packet.h"
#include "ns3/trickles-header.h"
#include "ns3/trickles-shieh-header.h"
#include "ns3/trickles-socket-factory.h"
#include "ns3/trickles-socket-base.h"
#include "ns3/trickles-l4-protocol.h"
#include "ns3/tcp-rx-buffer.h"
#include "ns3/rtt-estimator.h"
#include "ns3/trickles-shieh.h"

NS_LOG_COMPONENT_DEFINE ("TricklesShieh");

static uint16_t ShiehInitialCwnd = 2;
static uint16_t ShiehInitialSsthresh = 66;
static uint16_t ShiehDupTrickles = 3;

namespace ns3 {
    
    NS_OBJECT_ENSURE_REGISTERED (TricklesShieh);
    
#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT                                   \
if (m_node) { std::clog << Simulator::Now ().GetSeconds () << " [node " << m_node->GetId () << "] "; }
    
#define MY_LOG_TRICKLES_PACKET(p) \
NS_LOG_DEBUG("Logging packet: "); \
LOG_TRICKLES_PACKET(p); \
std::clog << " "; \
LOG_TRICKLES_SHIEH_PACKET_(p); \
std::clog << "\n";

    // Add attributes generic to all TricklesSockets to base class TricklesSocket
    TypeId
    TricklesShieh::GetTypeId (void)
    {
        static TypeId tid = TypeId ("ns3::TricklesShieh")
        .SetParent<TricklesSocketBase> ()
        .AddConstructor<TricklesShieh> ()
        .AddAttribute("StartCwnd",
                      "TricklesShieh start cwnd value",
                      UintegerValue(2),
                      MakeUintegerAccessor(&TricklesShieh::GetStartCwnd,
                                           &TricklesShieh::SetStartCwnd),
                      MakeUintegerChecker<uint32_t> ())
        .AddAttribute("StartSsthresh",
                      "TricklesShieh start ssthresh value",
                      UintegerValue(0xffff),
                      MakeUintegerAccessor(&TricklesShieh::GetStartSsthresh,
                                           &TricklesShieh::SetStartSsthresh),
                      MakeUintegerChecker<uint32_t>())
        ;
        return tid;
    }
    
    TricklesShieh::TricklesShieh ():TricklesSocketBase(), m_tcpBase(SequenceNumber32(0)), m_cwnd(ShiehInitialCwnd), m_ssthresh(ShiehInitialSsthresh)
    {
        NS_LOG_FUNCTION_NOARGS ();
    }
    
    TricklesShieh::TricklesShieh(const TricklesShieh &sock)
    : TricklesSocketBase(sock), m_tcpBase(SequenceNumber32(0)), m_cwnd(ShiehInitialCwnd), m_ssthresh(ShiehInitialSsthresh) {
        NS_LOG_FUNCTION (this);
   }
    
    TricklesShieh::~TricklesShieh ()
    {
        NS_LOG_FUNCTION (this);
        if (m_retxEvent.IsRunning()) m_retxEvent.Cancel();
    }
    
    void TricklesShieh::NewRequest() {
        // NS_LOG_FUNCTION (this);
        m_retries = 0;
        if (m_RcvdRequests.numBlocks()==0) {
            m_tcpBase = SequenceNumber32(0);
            uint32_t i = m_tcpBase.GetValue();
            while (m_delayed.size()<m_cwnd) {
                TricklesHeader trh;
                trh.SetPacketType(REQUEST);
                trh.SetParentNumber(m_tcpBase);
                trh.SetTrickleNumber(++i);
                trh.SetRecovery(NO_RECOVERY);
                trh.SetTSVal(GetCurTSVal());
                trh.SetTSEcr(SequenceNumber32(0));
                trh.SetRTT(GetMinRto());
                trh.SetFirstLoss(SequenceNumber32(0));
                TricklesShiehHeader tsh;
                tsh.SetTcpBase(m_tcpBase);
                tsh.SetStartCwnd(m_cwnd);
                tsh.SetSsthresh(m_ssthresh);
                m_RcvdRequests.AddBlock(SequenceNumber32(i),SequenceNumber32(i+1));
                Ptr<Packet> p = Create<Packet>();
                p->AddHeader(tsh);
                p->AddHeader(trh);
                DelayPacket(p);
            }
        }
        TrySendDelayed();
    }
    
    void TricklesShieh::ProcessTricklesPacket(Ptr<Packet> packet, TricklesHeader &th) {
        TricklesShiehHeader tsh;
        NS_LOG_FUNCTION (this);
        
//        LOG_TRICKLES_HEADER(th); std::clog << "\n";
//        MY_LOG_TRICKLES_PACKET(packet);
        ResetMultiplier();

        if (packet->RemoveHeader(tsh)) {
            TricklesSocketBase::ProcessTricklesPacket(packet, th);
            //std::clog << "Shieh processing: "; LOG_TRICKLES_HEADER(th); std::clog << "\n";

            if (th.GetPacketType()==CONTINUATION) ProcessShiehRequest(packet, th, tsh);
            else ProcessShiehContinuation(packet, th, tsh);
            //MY_LOG_TRICKLES_PACKET(packet);
        }
    }
    
    void TricklesShieh::ProcessShiehContinuation(Ptr<Packet> packet, TricklesHeader th, TricklesShiehHeader trh) {
        // Класс-предок обновляет TricklesSack и добавляет данные в очередь приложению
        // он же отлавливает дубликаты, соответственно если пакет до сюда добрался, то он не был дубликатом/некорректным пакетом
        NS_LOG_FUNCTION (this);
        
        // Нормальная отправка пакета
        if (th.IsRecovery() == NO_RECOVERY) {
            m_tcpBase = trh.GetTcpBase();
            m_cwnd = trh.GetStartCwnd();
            m_ssthresh = trh.GetSsthresh();
        }
        th.SetRequestSize(m_segSize);
        if (m_RcvdRequests.numBlocks()>1) {
            // Срабатывание повторной передачи
            packet->AddHeader(trh);
            packet->AddHeader(th);
            //std::clog << "Fast retransmit triggered by: "; MY_LOG_TRICKLES_PACKET(packet); std::clog << "\n";
            
            DelayPacket(packet);
            SackConstIterator i = m_RcvdRequests.firstBlock();
            i++;
            uint32_t t = 0;
            while (!m_RcvdRequests.isEnd(i)) {
                t += i->second - i->first;
                i++;
            }
            if (t==ShiehDupTrickles) {
                TrySendDelayed(true);
            }
        } else {
            ResetMultiplier();
            packet->AddHeader(trh);
            packet->AddHeader(th);
            DelayPacket(packet);
            TrySendDelayed();
        }
    }
    
    void TricklesShieh::ProcessShiehRequest(Ptr<Packet> packet, TricklesHeader th, TricklesShiehHeader trh) {
        NS_LOG_FUNCTION (this);
        SocketAddressTag tag;
        NS_ASSERT(packet->PeekPacketTag(tag));
        TricklesSack s = th.GetSacks();
        SequenceNumber32 parent_trickle = th.GetTrickleNumber();
        SackConstIterator i = s.firstBlock();
        
        if (s.numBlocks()>1) {
            // Обнаружены потери
            SequenceNumber32 firstLoss = i->second;
            NS_LOG_DEBUG("Losses!");
            if (th.IsRecovery() == NO_RECOVERY) {
                // Быстрая ретрансляция
                i++;
                uint16_t cwndatloss = tcpCwnd(trh.GetTcpBase(), trh.GetStartCwnd(), trh.GetSsthresh(), SequenceNumber32(firstLoss - 1));
                NS_LOG_DEBUG("Fast recovery with cwnd at loss=" << cwndatloss);
                if (th.GetTrickleNumber()==i->first) {
                    // Первый пакет после серии потерь
                    th.SetParentNumber(parent_trickle);
                    th.SetTrickleNumber(firstLoss);
                    th.SetFirstLoss(firstLoss);
                    th.SetRecovery(FAST_RETRANSMIT);
                    th.SetRequestSize(0);
                    trh.SetStartCwnd(cwndatloss/2);
                    trh.SetSsthresh(cwndatloss/2);
                    packet->AddHeader(trh);
                    packet->AddHeader(th);
                    // Добавить этот пакет в очередь к приложению
                    QueueToServerApp(packet);
                } else {
                    uint32_t lossOffset = th.GetTrickleNumber()-firstLoss;
                    // uint16_t numInFlight = cwndatloss-1;
                    NS_LOG_DEBUG("Loss offset: " << lossOffset);
                    if ((cwndatloss-lossOffset+1)<=(cwndatloss/2)) {
                        th.SetParentNumber(parent_trickle);
                        th.SetTrickleNumber(lossOffset+cwndatloss);
                        th.SetRecovery(FAST_RETRANSMIT);
                        trh.SetStartCwnd(cwndatloss/2);
                        trh.SetSsthresh(cwndatloss/2);
                        packet->AddHeader(trh);
                        packet->AddHeader(th);
                        // Добавить этот пакет в очередь к приложению
                        QueueToServerApp(packet);
                    } else NS_LOG_DEBUG("Killed trickle!");
                }
            }
            if (th.IsRecovery() == RTO_TIMEOUT) {
                uint16_t l;
                SequenceNumber32 f = SequenceNumber32(1);
                SequenceNumber32 t = SequenceNumber32(1);
                NS_LOG_DEBUG("RTO Timeout recovery");
                for (l=1; l<ShiehInitialCwnd; l++) {
                    if (f>=t) {
                        if (i != s.firstBlock()) i++;
                        if (s.isEnd(i)) break;
                        f = i->second;
                        i++;
                        if (s.isEnd(i)) break;
                        t = i->first;
                    }
                    TricklesHeader newth = th;
                    TricklesShiehHeader newtrh = trh;
                    th.SetParentNumber(parent_trickle);
                    th.SetTrickleNumber(f);
                    th.SetFirstLoss(firstLoss);
                    //th.SetRequestSize(1000);
                    trh.SetStartCwnd(ShiehInitialCwnd);
                    trh.SetSsthresh(tcpCwnd(trh.GetTcpBase(), trh.GetStartCwnd(), trh.GetSsthresh(), firstLoss-1)/2);
                    Ptr<Packet> p = Create<Packet>();
                    p->AddHeader(trh);
                    p->AddHeader(th);
                    p->AddPacketTag(tag);
                    // Добавить этот пакет в очередь к приложению
                    QueueToServerApp(p);
                }
            }
        } else {
            if (th.IsRecovery() == NO_RECOVERY) {
                // Нормальное функционирование/выход из режима восстановления по тайм-ауту
                uint16_t curcwnd = tcpCwnd(trh.GetTcpBase(), trh.GetStartCwnd(), trh.GetSsthresh(), th.GetTrickleNumber());
                uint16_t prevcwnd = tcpCwnd(trh.GetTcpBase(), trh.GetStartCwnd(), trh.GetSsthresh(), th.GetTrickleNumber()-1);
                int16_t cwnddelta = curcwnd-prevcwnd;
                NS_LOG_DEBUG("Normal/RTO recovery exit. TCPCwnd(k=seq)=" << curcwnd << " TCPCwnd(k=seq-1)=" << prevcwnd << " CwndDelta=" << cwnddelta);
                if (cwnddelta>=0) {
                    th.SetRecovery(NO_RECOVERY);
                    th.SetTrickleNumber(th.GetTrickleNumber()+prevcwnd);
                    th.SetParentNumber(parent_trickle);
                    packet->AddHeader(trh);
                    packet->AddHeader(th);
                    NS_LOG_DEBUG("Queuing to server app");
                    //MY_LOG_TRICKLES_PACKET(packet);
                    // Добавить пакет packet в очередь к приложению
                    QueueToServerApp(packet);
                    for (uint16_t i=1; i<=cwnddelta; i++) {
                        Ptr<Packet> p = Create<Packet>();
                        TricklesHeader pth = th;
                        TricklesShiehHeader ptrh = trh;
                        pth.SetTrickleNumber(th.GetTrickleNumber()+i);
                        pth.SetParentNumber(parent_trickle);
                        pth.SetRequestSize(0);
                        p->AddHeader(ptrh);
                        p->AddHeader(pth);
                        //MY_LOG_TRICKLES_PACKET(p);
                        // Добавить этот пакет в очередь к приложению
                        p->AddPacketTag(tag);
                        QueueToServerApp(p);
                    }
                }
            }
            // Нет потерь
            if (th.IsRecovery() == RTO_TIMEOUT) {
//                SequenceNumber32 firstLoss = th.GetFirstLoss();
                th.SetRecovery(NO_RECOVERY);
                trh.SetTcpBase(i->second-1);
                trh.SetStartCwnd(ShiehInitialCwnd);
                trh.SetSsthresh(trh.GetSsthresh()/2);
                packet->AddHeader(trh);
                packet->AddHeader(th);
                QueueToServerApp(packet);
            }
            if (th.IsRecovery() == FAST_RETRANSMIT) {
                SequenceNumber32 firstLoss = th.GetFirstLoss();
                uint16_t cwndatloss = tcpCwnd(trh.GetTcpBase(), trh.GetStartCwnd(), trh.GetSsthresh(), SequenceNumber32(firstLoss-1));
                NS_LOG_DEBUG("Fast retransmit recovery exit");
                th.SetRecovery(NO_RECOVERY);
                th.SetTrickleNumber(firstLoss+SequenceNumber32(cwndatloss)+SequenceNumber32(1));
                th.SetParentNumber(parent_trickle);
                trh.SetStartCwnd(cwndatloss/2);
                trh.SetSsthresh(cwndatloss/2);
                trh.SetTcpBase(firstLoss+SequenceNumber32(cwndatloss));
                packet->AddHeader(trh);
                packet->AddHeader(th);
                // Добавить пакет packet в очередь к приложению
                QueueToServerApp(packet);
            }
        }
    }
    
    uint16_t TricklesShieh::tcpCwnd(SequenceNumber32 tcpBase, uint16_t cwnd, uint16_t ssthresh, SequenceNumber32 k) const {
        
        SequenceNumber32 A = SequenceNumber32(tcpBase.GetValue()-cwnd+ssthresh);
        //NS_LOG_FUNCTION (this << "tcpBase=" << tcpBase << " startCwnd=" << cwnd << " ssthresh=" << ssthresh << " k=" << k << " A=" << A);
        int16_t result = 0;
        if (k<A) {
            result = cwnd+(k-tcpBase);
        } else
        if (k<=(A+SequenceNumber32(ssthresh))) {
            result = ssthresh;
        } else {
            double zero = 0.5+sqrt(0.25+ssthresh*ssthresh-ssthresh+2.0*(k-A));
            NS_ASSERT(fabs(((zero-1.0)*zero-(ssthresh-1.0)*ssthresh)/2.0-(k-A))<0.001);
            result = ceil(zero);
        }

        NS_ASSERT(result>0);
        
        return(result);
    }
    
    void TricklesShieh::DelayPacket(Ptr<Packet> packet) {
        NS_LOG_FUNCTION (this);
        TricklesHeader trh;
        NS_ASSERT(packet->PeekHeader(trh));
        TricklesShiehHeader tsh;
        packet->RemoveHeader(trh);
        NS_ASSERT(packet->PeekHeader(tsh));
        packet->AddHeader(trh);
        NS_ASSERT(packet->PeekHeader(trh));
        if (m_delayed.find(trh.GetTrickleNumber()) != m_delayed.end()) {
            m_delayed[trh.GetTrickleNumber()]=packet;
        }
    }
    
    void TricklesShieh::TrySendDelayed(bool fastrx) {
        std::map<SequenceNumber32, Ptr<Packet> >::iterator it = m_delayed.begin();
        uint32_t newReqSize = 0;
        // In-order packet transmission
        while (it != m_delayed.end()) {
            if ((m_reqDataSize-newReqSize)<m_segSize) break;
            Ptr<Packet> p = it->second;
            TricklesHeader th;
            p->PeekHeader(th);
            if ((fastrx) || (th.GetTrickleNumber()<m_RcvdRequests.firstLoss())) {
                p->RemoveHeader(th);
                th.SetRequestSize(m_segSize);
                newReqSize += m_segSize;
                th.SetSacks(m_RcvdRequests);
                it = m_delayed.erase(it);
                p->AddHeader(th);
                m_retxEvent.Cancel();
                m_retxEvent = Simulator::Schedule(GetRto(), &TricklesShieh::ReTxTimeout, this);
                TricklesSocketBase::Send(p, 0);
            } else break;
        }
    }
    
    void TricklesShieh::ReTxTimeout() {
        NS_LOG_FUNCTION(this);
        if ((m_retxEvent.IsExpired()) && (m_RcvdRequests.numBlocks()>1)) {
            m_retries++;
            TricklesHeader trh;
            trh.SetPacketType(REQUEST);
            SackConstIterator i = m_RcvdRequests.firstBlock();
            trh.SetTrickleNumber(i->second);
            trh.SetRequestSize(m_segSize);
            trh.SetRecovery(RTO_TIMEOUT);
            trh.SetTSVal(GetCurTSVal());
            trh.SetTSEcr(SequenceNumber32(0));
            trh.SetRTT(m_rtt->GetEstimate());
            trh.SetFirstLoss(i->second);
            trh.SetSacks(m_RcvdRequests);
            TricklesShiehHeader tsh;
            tsh.SetTcpBase(m_tcpBase);
            tsh.SetStartCwnd(m_cwnd);
            tsh.SetSsthresh(m_ssthresh);
            Ptr<Packet> p = Create<Packet> ();
            p->AddHeader(tsh);
            p->AddHeader(trh);
            m_retxEvent.Cancel();
            m_retxEvent = Simulator::Schedule(GetRto(), &TricklesShieh::ReTxTimeout, this);
            TricklesSocketBase::Send(p, 0);
        }
    }

} // namespace ns3
