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

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/stats-module.h"
#include "trickles-appcont.h"
#include "trickles-sink.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TricklesSink");
NS_OBJECT_ENSURE_REGISTERED (TricklesSink);

TypeId
TricklesSink::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::TricklesSink")
    .SetParent<Application> ()
    .AddConstructor<TricklesSink> ()
    .AddAttribute ("Remote", "The Address on which to Bind the rx socket.",
                   AddressValue (),
                   MakeAddressAccessor (&TricklesSink::m_peer),
                   MakeAddressChecker ())
    .AddAttribute ("PacketSize", "The request packet size",
                   UintegerValue(512),
                   MakeUintegerAccessor (&TricklesSink::m_packetSize),
                   MakeUintegerChecker<uint32_t> (1))
    .AddAttribute ("DataRate", "The rate with which requests are generated",
                   DataRateValue(DataRate("100kb/s")),
                   MakeDataRateAccessor (&TricklesSink::m_dataRate),
                   MakeDataRateChecker ())
    //    .AddAttribute ("Protocol", "The type id of the protocol to use for the rx socket.",
    //                   TypeIdValue (TricklesSocketFactory::GetTypeId ()),
    //                   MakeTypeIdAccessor (&TricklesSink::m_tid),
    //                   MakeTypeIdChecker ())
    ;
    return tid;
}


TricklesSink::TricklesSink ()
: m_socket (0),
m_peer (),
m_packetSize (0),
m_dataRate (0),
m_sendEvent (),
m_running (false),
m_packetsSent (0),
m_totalRx(0)
{
    NS_LOG_FUNCTION (this);
}

TricklesSink::~TricklesSink ()
{
    NS_LOG_FUNCTION (this);
    m_socket = 0;
}

void
TricklesSink::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, DataRate dataRate)
{
    NS_LOG_FUNCTION (this);
    m_socket = socket;
    m_peer = address;
    m_packetSize = packetSize;
    m_dataRate = dataRate;
}

void
TricklesSink::StartApplication (void)
{
    NS_LOG_FUNCTION (this);
    m_running = true;
    m_packetsSent = 0;
    if (!m_socket) {
        m_socket = Socket::CreateSocket(GetNode(), TricklesSocketFactory::GetTypeId());
        if (Inet6SocketAddress::IsMatchingType (m_peer))
        {
            m_socket->Bind6 ();
        }
        else
        {
            m_socket->Bind ();
        }
        m_socket->Connect (m_peer);
        m_socket->SetRecvCallback (MakeCallback (&TricklesSink::HandleRead, this));
    }
    GetData ();
}

void
TricklesSink::StopApplication (void)
{
    NS_LOG_FUNCTION (this);
    
    m_running = false;
    
    if (m_sendEvent.IsRunning ())
    {
        Simulator::Cancel (m_sendEvent);
    }
    
    if (m_socket)
    {
        m_socket->Close ();
    }
}

void
TricklesSink::GetData (void)
{
    NS_LOG_FUNCTION (this);
    
    if (m_socket->GetRxAvailable()>0) HandleRead(m_socket);
    m_packetsSent += m_packetSize;
    m_socket->Recv(m_packetSize, TricklesSocketBase::QUEUE_RECV);
    ScheduleRq ();
}

void
TricklesSink::ScheduleRq (void)
{
    NS_LOG_FUNCTION (this);
    
    if (m_running)
    {
        Time tNext (Seconds (m_packetSize * 8.0 / static_cast<double> (m_dataRate.GetBitRate ())));
        m_sendEvent = Simulator::Schedule (tNext, &TricklesSink::GetData, this);
    }
}

void TricklesSink::HandleRead (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
    Ptr<Packet> packet;
    Address from;
    while (socket->GetRxAvailable()>0)
    {
        packet = socket->RecvFrom(from);
        if (packet == 0) break;
        if (packet->GetSize () == 0)
        { //EOF
            break;
        }
        m_totalRx += packet->GetSize ();
    }
}

uint32_t TricklesSink::GetTotalRx() {
    return m_totalRx;
}

uint32_t TricklesSink::GetTotalRq() {
    return m_packetsSent;
}
