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

#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/trickles-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/trickles-socket-factory.h"
#include "ns3/trickles-header.h"
#include "trickles-appcont.h"
#include "trickles-server.h"
#include "ns3/trickles-shieh-header.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TricklesServer");
NS_OBJECT_ENSURE_REGISTERED (TricklesServer);

TypeId
TricklesServer::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::TricklesServer")
    .SetParent<Application> ()
    .AddConstructor<TricklesServer> ()
    .AddAttribute ("Local", "The Address on which to Bind the rx socket.",
                   AddressValue (),
                   MakeAddressAccessor (&TricklesServer::m_local),
                   MakeAddressChecker ())
//    .AddAttribute ("Protocol", "The type id of the protocol to use for the rx socket.",
//                   TypeIdValue (TricklesSocketFactory::GetTypeId ()),
//                   MakeTypeIdAccessor (&TricklesServer::m_tid),
//                   MakeTypeIdChecker ())
    ;
    return tid;
}

TricklesServer::TricklesServer ()
{
    NS_LOG_FUNCTION (this);
    m_socket = 0;
    m_totalTx = 0;
}

TricklesServer::~TricklesServer()
{
    NS_LOG_FUNCTION (this);
}

uint32_t TricklesServer::GetTotalTx () const
{
    NS_LOG_FUNCTION (this);
    return m_totalTx;
}

Ptr<Socket>
TricklesServer::GetListeningSocket (void) const
{
    NS_LOG_FUNCTION (this);
    return m_socket;
}

std::list<Ptr<Socket> >
TricklesServer::GetAcceptedSockets (void) const
{
    NS_LOG_FUNCTION (this);
    return m_socketList;
}

void TricklesServer::DoDispose (void)
{
    NS_LOG_FUNCTION (this);
    m_socket = 0;
    m_socketList.clear ();
    
    // chain up
    Application::DoDispose ();
}

void TricklesServer::StartApplication ()    // Called at time specified by Start
{
    NS_LOG_FUNCTION (this);
    // Create the socket if not already
    if (!m_socket)
    {
        TypeId m_tid = TricklesSocketFactory::GetTypeId();
        NS_LOG_INFO("TypeId is " << m_tid);
        m_socket = Socket::CreateSocket (GetNode (), m_tid);
        m_socket->Bind(m_local);
        m_socket->Listen();
        
    }
    
    
    m_socket->SetAcceptCallback (
                                 MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
                                 MakeCallback (&TricklesServer::HandleAccept, this));
    m_socket->SetRecvCallback (MakeCallback (&TricklesServer::HandleRead, this));
}

void TricklesServer::StopApplication ()     // Called at time specified by Stop
{
    NS_LOG_FUNCTION (this);
    while(!m_socketList.empty ()) //these are accepted sockets, close them
    {
        Ptr<Socket> acceptedSocket = m_socketList.front ();
        m_socketList.pop_front ();
        acceptedSocket->Close ();
    }
    if (m_socket)
    {
        m_socket->Close ();
        m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void TricklesServer::HandleRead (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom (from)))
    {
        if (packet->GetSize () == 0)
        { //EOF
            NS_LOG_DEBUG("Received packet of zero size");
            break;
        }
        // Code to process incoming packet
        TricklesHeader tricklesHeader;
        if (!packet->PeekHeader(tricklesHeader)) continue;
        NS_LOG_DEBUG(" ");
        // LOG_TRICKLES_HEADER(tricklesHeader); std::clog << "\n";
        if (tricklesHeader.GetPacketType()==CONTINUATION) {
            Ptr<Packet> data = Create<Packet>(tricklesHeader.GetRequestSize());
            packet->AddAtEnd(data);
            NS_LOG_DEBUG("Sending packet of size " << packet->GetSize() << " to " << InetSocketAddress::ConvertFrom (from).GetIpv4 ());
            //LOG_TRICKLES_PACKET(packet);
            //std::clog << " ";
            //LOG_TRICKLES_SHIEH_PACKET_(packet);
            //std::clog << "\n";
            socket->SendTo(packet, 0, from);
            m_totalTx += tricklesHeader.GetRequestSize();
        }
    }
}

void TricklesServer::HandlePeerError (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
}

void TricklesServer::HandlePeerClose (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
}

void TricklesServer::HandleAccept (Ptr<Socket> s, const Address& from)
{
    NS_LOG_FUNCTION (this << s << from);
    s->SetRecvCallback (MakeCallback (&TricklesServer::HandleRead, this));
    m_socketList.push_back (s);
}

