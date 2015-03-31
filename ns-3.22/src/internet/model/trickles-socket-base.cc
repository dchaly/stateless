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
#include <queue>
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
#include "trickles-socket-factory.h"
#include "trickles-socket-base.h"
#include "trickles-l4-protocol.h"
#include "trickles-header.h"
#include "ipv4-end-point.h"
#include "ipv6-end-point.h"
#include "ipv6-l3-protocol.h"
#include "tcp-tx-buffer.h"
#include "tcp-rx-buffer.h"
#include "rtt-estimator.h"
#include <limits>

NS_LOG_COMPONENT_DEFINE ("TricklesSocketBase");

namespace ns3 {
    
    NS_OBJECT_ENSURE_REGISTERED (TricklesSocketBase);
    
    // Add attributes generic to all TricklesSockets to base class TricklesSocket
    TypeId
    TricklesSocketBase::GetTypeId (void)
    {
        static TypeId tid = TypeId ("ns3::TricklesSocketBase")
        .SetParent<TricklesSocket> ()
        .AddAttribute ("IcmpCallback", "Callback invoked whenever an icmp error is received on this socket.",
                       CallbackValue (),
                       MakeCallbackAccessor (&TricklesSocketBase::m_icmpCallback),
                       MakeCallbackChecker ())
        .AddAttribute ("IcmpCallback6", "Callback invoked whenever an icmpv6 error is received on this socket.",
                       CallbackValue (),
                       MakeCallbackAccessor (&TricklesSocketBase::m_icmpCallback6),
                       MakeCallbackChecker ())
        ;
        return tid;
    }
    
    uint32_t TricklesSocketBase::QUEUE_RECV = 0x1;
    
    TricklesSocketBase::TricklesSocketBase ()
    : m_endPoint (0),
    m_endPoint6 (0),
    m_node (0),
    m_trickles (0),
    m_rtt(0),
    m_reqDataSize(0),
    m_segSize(0),
    m_tsecr (SequenceNumber32(0)),
    m_retries(0),
    m_shutdownSend(false),
    m_shutdownRecv(false)
    {
        NS_LOG_FUNCTION_NOARGS ();
        m_tsstart = Simulator::Now();
        m_tsgranularity = MilliSeconds(5);
        m_rqQueue.clear();
    }
    
    TricklesSocketBase::TricklesSocketBase(const TricklesSocketBase &sock)
    : TricklesSocket(sock),
    m_endPoint(0),
    m_endPoint6(0),
    m_node(sock.m_node),
    m_trickles(sock.m_trickles),
    m_rtt(0),
    m_reqDataSize(sock.m_reqDataSize),
    m_rxBuffer(sock.m_rxBuffer),
    m_segSize(sock.m_segSize),
    m_tsstart(sock.m_tsstart),
    m_tsgranularity(sock.m_tsgranularity),
    m_tsecr(sock.m_tsecr),
    m_retries(sock.m_retries),
    m_errno(sock.m_errno),
    m_shutdownSend(sock.m_shutdownSend),
    m_shutdownRecv(sock.m_shutdownRecv) {
        NS_LOG_FUNCTION (this);
        NS_LOG_LOGIC ("Invoked the copy constructor");
        // Copy the rtt estimator if it is set
        if (sock.m_rtt)
        {
            m_rtt = sock.m_rtt->Copy ();
        }
        // Reset all callbacks to null
        Callback<void, Ptr< Socket > > vPS = MakeNullCallback<void, Ptr<Socket> > ();
        Callback<void, Ptr<Socket>, const Address &> vPSA = MakeNullCallback<void, Ptr<Socket>, const Address &> ();
        Callback<void, Ptr<Socket>, uint32_t> vPSUI = MakeNullCallback<void, Ptr<Socket>, uint32_t> ();
        SetConnectCallback (vPS, vPS);
        SetDataSentCallback (vPSUI);
        SetSendCallback (vPSUI);
        SetRecvCallback (vPS);
    }
    
    TricklesSocketBase::~TricklesSocketBase ()
    {
        NS_LOG_FUNCTION (this);
        m_node = 0;
        if (m_endPoint != 0)
        {
            NS_ASSERT (m_trickles != 0);
            /*
             * Upon Bind, an Ipv4Endpoint is allocated and set to m_endPoint, and
             * DestroyCallback is set to TcpSocketBase::Destroy. If we called
             * m_tcp->DeAllocate, it wil destroy its Ipv4EndpointDemux::DeAllocate,
             * which in turn destroys my m_endPoint, and in turn invokes
             * TcpSocketBase::Destroy to nullify m_node, m_endPoint, and m_tcp.
             */
            NS_ASSERT (m_endPoint != 0);
            m_trickles->DeAllocate (m_endPoint);
            NS_ASSERT (m_endPoint == 0);
        }
        if (m_endPoint6 != 0)
        {
            NS_ASSERT (m_trickles != 0);
            NS_ASSERT (m_endPoint6 != 0);
            m_trickles->DeAllocate (m_endPoint6);
            NS_ASSERT (m_endPoint6 == 0);
        }
        m_trickles = 0;
        CancelAllTimers ();
    }
    
    void
    TricklesSocketBase::SetNode (Ptr<Node> node)
    {
        NS_LOG_FUNCTION_NOARGS ();
        m_node = node;
        
    }
    void
    TricklesSocketBase::SetTrickles (Ptr<TricklesL4Protocol> trickles)
    {
        NS_LOG_FUNCTION_NOARGS ();
        m_trickles = trickles;
    }
    
    /** Set an RTT estimator with this socket */
    void
    TricklesSocketBase::SetRtt (Ptr<RttEstimator> rtt)
    {
        m_rtt = rtt;
    }
    
    enum Socket::SocketErrno
    TricklesSocketBase::GetErrno (void) const
    {
        return m_errno;
    }
    
    enum Socket::SocketType
    TricklesSocketBase::GetSocketType (void) const
    {
        return NS3_SOCK_STREAM;
    }
    
    Ptr<Node>
    TricklesSocketBase::GetNode (void) const
    {
        NS_LOG_FUNCTION_NOARGS ();
        return m_node;
    }
    
    void
    TricklesSocketBase::Destroy (void)
    {
        NS_LOG_FUNCTION (this);
        m_endPoint = 0;
        if (m_trickles != 0)
        {
            std::vector<Ptr<TricklesSocketBase> >::iterator it
            = std::find (m_trickles->m_sockets.begin (), m_trickles->m_sockets.end (), this);
            if (it != m_trickles->m_sockets.end ())
            {
                m_trickles->m_sockets.erase (it);
            }
        }
        CancelAllTimers ();
    }
    
    void
    TricklesSocketBase::Destroy6 (void)
    {
        NS_LOG_FUNCTION (this);
        m_endPoint6 = 0;
        if (m_trickles != 0)
        {
            std::vector<Ptr<TricklesSocketBase> >::iterator it
            = std::find (m_trickles->m_sockets.begin (), m_trickles->m_sockets.end (), this);
            if (it != m_trickles->m_sockets.end ())
            {
                m_trickles->m_sockets.erase (it);
            }
        }
        
        CancelAllTimers ();
    }
    
    int
    TricklesSocketBase::Bind (void)
    {
        NS_LOG_FUNCTION (this);
        m_endPoint = m_trickles->Allocate ();
        if (0 == m_endPoint) {
            m_errno = ERROR_ADDRNOTAVAIL;
            return -1;
        }
        m_trickles->m_sockets.push_back (this);
        return SetupCallback ();
    }
    
    int
    TricklesSocketBase::Bind6 (void)
    {
        NS_LOG_FUNCTION_NOARGS ();
        m_endPoint6 = m_trickles->Allocate6 ();
        if (0 == m_endPoint6)
        {
            m_errno = ERROR_ADDRNOTAVAIL;
            return -1;
        }
        m_trickles->m_sockets.push_back (this);
        return SetupCallback ();
    }
    
    int
    TricklesSocketBase::Bind (const Address &address)
    {
        NS_LOG_FUNCTION (this << address);
        
        if (InetSocketAddress::IsMatchingType (address))
        {
            InetSocketAddress transport = InetSocketAddress::ConvertFrom (address);
            Ipv4Address ipv4 = transport.GetIpv4 ();
            uint16_t port = transport.GetPort ();
            if (ipv4 == Ipv4Address::GetAny () && port == 0)
            {
                m_endPoint = m_trickles->Allocate ();
            }
            else if (ipv4 == Ipv4Address::GetAny () && port != 0)
            {
                m_endPoint = m_trickles->Allocate (port);
            }
            else if (ipv4 != Ipv4Address::GetAny () && port == 0)
            {
                m_endPoint = m_trickles->Allocate (ipv4);
            }
            else if (ipv4 != Ipv4Address::GetAny () && port != 0)
            {
                m_endPoint = m_trickles->Allocate (ipv4, port);
            }
            if (0 == m_endPoint)
            {
                m_errno = port ? ERROR_ADDRINUSE : ERROR_ADDRNOTAVAIL;
                return -1;
            }
        }
        else if (Inet6SocketAddress::IsMatchingType (address))
        {
            Inet6SocketAddress transport = Inet6SocketAddress::ConvertFrom (address);
            Ipv6Address ipv6 = transport.GetIpv6 ();
            uint16_t port = transport.GetPort ();
            if (ipv6 == Ipv6Address::GetAny () && port == 0)
            {
                m_endPoint6 = m_trickles->Allocate6 ();
            }
            else if (ipv6 == Ipv6Address::GetAny () && port != 0)
            {
                m_endPoint6 = m_trickles->Allocate6 (port);
            }
            else if (ipv6 != Ipv6Address::GetAny () && port == 0)
            {
                m_endPoint6 = m_trickles->Allocate6 (ipv6);
            }
            else if (ipv6 != Ipv6Address::GetAny () && port != 0)
            {
                m_endPoint6 = m_trickles->Allocate6 (ipv6, port);
            }
            if (0 == m_endPoint6)
            {
                m_errno = port ? ERROR_ADDRINUSE : ERROR_ADDRNOTAVAIL;
                return -1;
            }
        }
        else
        {
            NS_LOG_ERROR ("Not IsMatchingType");
            m_errno = ERROR_INVAL;
            return -1;
        }
        m_trickles->m_sockets.push_back (this);
        NS_LOG_LOGIC ("TricklesSocketBase " << this << " got an endpoint: " << m_endPoint);
        
        return SetupCallback ();
    }
    
    int
    TricklesSocketBase::ShutdownSend (void)
    {
        NS_LOG_FUNCTION_NOARGS ();
        m_shutdownSend = true;
        return 0;
    }
    
    int
    TricklesSocketBase::ShutdownRecv (void)
    {
        NS_LOG_FUNCTION_NOARGS ();
        m_shutdownRecv = true;
        return 0;
    }
    
    bool
    TricklesSocketBase::SetAllowBroadcast (bool allowBroadcast)
    {
        // Broadcast is not implemented. Return true only if allowBroadcast==false
        return (!allowBroadcast);
    }
    
    bool
    TricklesSocketBase::GetAllowBroadcast (void) const
    {
        return false;
    }
    
    int
    TricklesSocketBase::Close (void)
    {
        return 0;
    }
    
    int
    TricklesSocketBase::Connect (const Address & address) {
        NS_LOG_FUNCTION (this << address);
        
        // If haven't do so, Bind() this socket first
        if (InetSocketAddress::IsMatchingType (address) && m_endPoint6 == 0)
        {
            if (m_endPoint == 0)
            {
                if (Bind () == -1)
                {
                    NS_ASSERT (m_endPoint == 0);
                    return -1; // Bind() failed
                }
                NS_ASSERT (m_endPoint != 0);
            }
            InetSocketAddress transport = InetSocketAddress::ConvertFrom (address);
            m_endPoint->SetPeer (transport.GetIpv4 (), transport.GetPort ());
            m_endPoint6 = 0;
            
            // Get the appropriate local address and port number from the routing protocol and set up endpoint
            if (SetupEndpoint () != 0)
            { // Route to destination does not exist
                return -1;
            }
        }
        else if (Inet6SocketAddress::IsMatchingType (address)  && m_endPoint == 0)
        {
            // If we are operating on a v4-mapped address, translate the address to
            // a v4 address and re-call this function
            Inet6SocketAddress transport = Inet6SocketAddress::ConvertFrom (address);
            Ipv6Address v6Addr = transport.GetIpv6 ();
            if (v6Addr.IsIpv4MappedAddress () == true)
            {
                Ipv4Address v4Addr = v6Addr.GetIpv4MappedAddress ();
                return Connect (InetSocketAddress (v4Addr, transport.GetPort ()));
            }
            
            if (m_endPoint6 == 0)
            {
                if (Bind6 () == -1)
                {
                    NS_ASSERT (m_endPoint6 == 0);
                    return -1; // Bind() failed
                }
                NS_ASSERT (m_endPoint6 != 0);
            }
            m_endPoint6->SetPeer (v6Addr, transport.GetPort ());
            m_endPoint = 0;
            
            // Get the appropriate local address and port number from the routing protocol and set up endpoint
            if (SetupEndpoint6 () != 0)
            { // Route to destination does not exist
                return -1;
            }
        }
        else
        {
            m_errno = ERROR_INVAL;
            return -1;
        }
        
        // Re-initialize parameters in case this socket is being reused after CLOSE
        m_rtt->Reset ();
        
        return 0;
    }
    
    int
    TricklesSocketBase::Listen (void)
    {
        if (m_endPoint) SetupEndpoint();
        if (m_endPoint6) SetupEndpoint6();
        SetRcvBufSize(0);
        return 0;
    }
    
    int
    TricklesSocketBase::Send (Ptr<Packet> p, uint32_t flags)
    {
        NS_LOG_FUNCTION (this << p << flags);
        TricklesHeader th;
        if (!p->RemoveHeader(th)) return 0;
        if (m_reqDataSize) {
            m_reqDataSize -= (th.IsRecovery()==NO_RECOVERY)?th.GetRequestSize():0;
        }
        if (th.GetPacketType()==REQUEST) {
            th.SetSacks(m_RcvdRequests);
        }
        th.SetTSEcr(m_tsecr);
        th.SetTSVal(GetCurTSVal());
        p->AddHeader(th);
        // Update transport continuation if data is sent
        if (IsManualIpTos ())
        {
            SocketIpTosTag ipTosTag;
            ipTosTag.SetTos (GetIpTos ());
            p->AddPacketTag (ipTosTag);
        }
        
        if (IsManualIpv6Tclass ())
        {
            SocketIpv6TclassTag ipTclassTag;
            ipTclassTag.SetTclass (GetIpv6Tclass ());
            p->AddPacketTag (ipTclassTag);
        }
        
        if (IsManualIpTtl ())
        {
            SocketIpTtlTag ipTtlTag;
            ipTtlTag.SetTtl (GetIpTtl ());
            p->AddPacketTag (ipTtlTag);
        }
        
        if (IsManualIpv6HopLimit ())
        {
            SocketIpv6HopLimitTag ipHopLimitTag;
            ipHopLimitTag.SetHopLimit (GetIpv6HopLimit ());
            p->AddPacketTag (ipHopLimitTag);
        }
        SocketAddressTag tag;
        bool tagflag = p->RemovePacketTag(tag);
        if (m_endPoint)
        {
            Ipv4Address peerIpv4 = tagflag?(InetSocketAddress::ConvertFrom (tag.GetAddress()).GetIpv4()):(m_endPoint->GetPeerAddress());
            uint16_t peerPort = tagflag?(InetSocketAddress::ConvertFrom (tag.GetAddress()).GetPort()):(m_endPoint->GetPeerPort());
            
            Ipv4Address localIpv4 = m_endPoint->GetLocalAddress();
            if (localIpv4==Ipv4Address::GetAny()) {
                Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4> ();
                NS_ASSERT (ipv4 != 0);
                if (ipv4->GetRoutingProtocol () == 0)
                {
                    NS_FATAL_ERROR ("No Ipv4RoutingProtocol in the node");
                }
                // Create a dummy packet, then ask the routing function for the best output
                // interface's address
                Ipv4Header header;
                header.SetDestination (peerIpv4);
                header.SetProtocol (TricklesL4Protocol::PROT_NUMBER);
                Socket::SocketErrno errno_;
                Ptr<Ipv4Route> route;
                Ptr<NetDevice> oif = m_boundnetdevice;
                route = ipv4->GetRoutingProtocol ()->RouteOutput (Ptr<Packet> (), header, oif, errno_);
                if (route == 0)
                {
                    NS_LOG_LOGIC ("Route to " << peerIpv4 << " does not exist");
                    NS_LOG_ERROR (errno_);
                    m_errno = errno_;
                    return -1;
                }
                localIpv4 = route->GetSource();
            }
            //LOG_TRICKLES_HEADER(th);
            NS_LOG_DEBUG("Sending from " << localIpv4 << ":" << m_endPoint->GetLocalPort() << " to " << peerIpv4 << ":" << peerPort);
            m_trickles->Send (p, localIpv4,
                              peerIpv4, m_endPoint->GetLocalPort(), peerPort, m_boundnetdevice);
        }
        else
        {
            Ipv6Address peerIpv6 = tagflag?(Inet6SocketAddress::ConvertFrom (tag.GetAddress()).GetIpv6()):(m_endPoint6->GetPeerAddress());
            uint16_t peerPort = tagflag?(Inet6SocketAddress::ConvertFrom (tag.GetAddress()).GetPort()):(m_endPoint6->GetPeerPort());
            Ipv6Address localIpv6 = m_endPoint6->GetLocalAddress();
            if (localIpv6==Ipv6Address::GetAny()) {
                Ptr<Ipv6L3Protocol> ipv6 = m_node->GetObject<Ipv6L3Protocol> ();
                NS_ASSERT (ipv6 != 0);
                if (ipv6->GetRoutingProtocol () == 0)
                {
                    NS_FATAL_ERROR ("No Ipv6RoutingProtocol in the node");
                }
                // Create a dummy packet, then ask the routing function for the best output
                // interface's address
                Ipv6Header header;
                header.SetDestinationAddress (m_endPoint6->GetPeerAddress ());
                Socket::SocketErrno errno_;
                Ptr<Ipv6Route> route;
                Ptr<NetDevice> oif = m_boundnetdevice;
                route = ipv6->GetRoutingProtocol ()->RouteOutput (Ptr<Packet> (), header, oif, errno_);
                if (route == 0)
                {
                    NS_LOG_LOGIC ("Route to " << m_endPoint6->GetPeerAddress () << " does not exist");
                    NS_LOG_ERROR (errno_);
                    m_errno = errno_;
                    return -1;
                }
                localIpv6 = route->GetSource();
            }
            SocketSetDontFragmentTag tag;
            bool found = p->RemovePacketTag (tag);
            if (!found) p->AddPacketTag (tag);
            m_trickles->Send (p, localIpv6,
                              peerIpv6, m_endPoint6->GetLocalPort(), peerPort, m_boundnetdevice);
        }
        return 0;
    }
    
    //  maximum message size for UDP broadcast is limited by MTU
    // size of underlying link; we are not checking that now.
    /// \todo Check MTU size of underlying link
    uint32_t
    TricklesSocketBase::GetTxAvailable (void) const
    {
        NS_LOG_FUNCTION_NOARGS ();
        return MAX_TRICKLES_TX_SIZE;
    }
    
    int
    TricklesSocketBase::SendTo (Ptr<Packet> p, uint32_t flags, const Address &address)
    {
        SocketAddressTag tag;
        p->RemovePacketTag(tag);
        tag.SetAddress(address);
        p->AddPacketTag(tag);
        return Send(p, flags);
    }
    
    uint32_t
    TricklesSocketBase::GetRxAvailable (void) const
    {
        NS_LOG_FUNCTION_NOARGS ();
        // Если есть пакеты, которые содержат запросы данных от серверной части, передаем их количество...
        if (m_rqQueue.size()) {
            return m_rqQueue.size();
        } else
            //... иначе проверяем, есть ли в буфере накопленные данные
            return(m_rxBuffer.Available());
    }
    
    Ptr<Packet>
    TricklesSocketBase::Recv (uint32_t maxSize, uint32_t flags)
    {
        NS_LOG_FUNCTION (this << maxSize << flags);
        // Delivering trickles request packet to server application
        Ptr<Packet> outPacket = NULL;
        if (m_rqQueue.size()>0) {
            outPacket = m_rqQueue.front();
            m_rqQueue.pop_front();
        } else
            // If there's no requests and no sufficient data at hand then queue request
            if (flags & TricklesSocketBase::QUEUE_RECV) {
                m_reqDataSize += maxSize;
                NewRequest();
                return 0;
            } else
                // ...or else return data packet from rxBuffer.
                outPacket = m_rxBuffer.Extract(maxSize);
        SocketAddressTag tag;
        if ((outPacket != 0) && (outPacket->GetSize () != 0) && (!outPacket->PeekPacketTag(tag)))
        {
            if (m_endPoint != 0)
            {
                tag.SetAddress (InetSocketAddress (m_endPoint->GetPeerAddress (), m_endPoint->GetPeerPort ()));
            }
            else if (m_endPoint6 != 0)
            {
                tag.SetAddress (Inet6SocketAddress (m_endPoint6->GetPeerAddress (), m_endPoint6->GetPeerPort ()));
            }
            outPacket->AddPacketTag (tag);
        }
        return outPacket;
    }
    
    Ptr<Packet>
    TricklesSocketBase::RecvFrom (uint32_t maxSize, uint32_t flags,
                                  Address &fromAddress)
    {
        NS_LOG_FUNCTION (this << maxSize << flags);
        Ptr<Packet> packet = Recv (maxSize, flags);
        if (packet != 0)
        {
            SocketAddressTag tag;
            bool found;
            found = packet->PeekPacketTag (tag);
            NS_ASSERT (found);
            fromAddress = tag.GetAddress ();
        }
        return packet;
    }
    
    int
    TricklesSocketBase::GetSockName (Address &address) const
    {
        NS_LOG_FUNCTION_NOARGS ();
        if (m_endPoint != 0)
        {
            address = InetSocketAddress (m_endPoint->GetLocalAddress (), m_endPoint->GetLocalPort ());
        }
        else if (m_endPoint6 != 0)
        {
            address = Inet6SocketAddress (m_endPoint6->GetLocalAddress (), m_endPoint6->GetLocalPort ());
        }
        else
        { // It is possible to call this method on a socket without a name
            // in which case, behavior is unspecified
            // Should this return an InetSocketAddress or an Inet6SocketAddress?
            address = InetSocketAddress (Ipv4Address::GetZero (), 0);
        }
        return 0;
    }
    
    void
    TricklesSocketBase::BindToNetDevice (Ptr<NetDevice> netdevice)
    {
        NS_LOG_FUNCTION (netdevice);
        Socket::BindToNetDevice (netdevice); // Includes sanity check
        if (m_endPoint == 0 && m_endPoint6 == 0)
        {
            if (Bind () == -1)
            {
                NS_ASSERT ((m_endPoint == 0 && m_endPoint6 == 0));
                return;
            }
            NS_ASSERT ((m_endPoint != 0 && m_endPoint6 != 0));
        }
        
        if (m_endPoint != 0)
        {
            m_endPoint->BindToNetDevice (netdevice);
        }
        // No BindToNetDevice() for Ipv6EndPoint
        return;
    }
    
    void TricklesSocketBase::SetRcvBufSize (uint32_t size) {
        m_rxBuffer.SetMaxBufferSize(size);
    }
    
    uint32_t TricklesSocketBase::GetRcvBufSize (void) const {
        return(m_rxBuffer.MaxBufferSize());
    }
    
    uint32_t TricklesSocketBase::FreeWindowSize() const {
        return(1); // Это хак чтобы откомпилировать исходники
    }
    
    void TricklesSocketBase::SetSegSize (uint32_t size) {
        m_segSize = size;
    }
    
    uint32_t TricklesSocketBase::GetSegSize (void) const {
        return(m_segSize);
    }
    
    int
    TricklesSocketBase::SetupCallback (void)
    {
        NS_LOG_FUNCTION (this);
        
        if (m_endPoint == 0 && m_endPoint6 == 0)
        {
            return -1;
        }
        if (m_endPoint != 0)
        {
            m_endPoint->SetRxCallback (MakeCallback (&TricklesSocketBase::ForwardUp, Ptr<TricklesSocketBase> (this)));
            m_endPoint->SetIcmpCallback (MakeCallback (&TricklesSocketBase::ForwardIcmp, Ptr<TricklesSocketBase> (this)));
            m_endPoint->SetDestroyCallback (MakeCallback (&TricklesSocketBase::Destroy, Ptr<TricklesSocketBase> (this)));
        }
        if (m_endPoint6 != 0)
        {
            m_endPoint6->SetRxCallback (MakeCallback (&TricklesSocketBase::ForwardUp6, Ptr<TricklesSocketBase> (this)));
            m_endPoint6->SetIcmpCallback (MakeCallback (&TricklesSocketBase::ForwardIcmp6, Ptr<TricklesSocketBase> (this)));
            m_endPoint6->SetDestroyCallback (MakeCallback (&TricklesSocketBase::Destroy6, Ptr<TricklesSocketBase> (this)));
        }
        
        return 0;
    }
    
    void
    TricklesSocketBase::ForwardUp (Ptr<Packet> packet, Ipv4Header header, uint16_t port, Ptr<Ipv4Interface> incomingInterface)
    {
        NS_LOG_FUNCTION (this << packet << header << port);
        Address fromAddress = InetSocketAddress (header.GetSource (), port);
        Address toAddress = InetSocketAddress (header.GetDestination (), m_endPoint->GetLocalPort ());
        SocketAddressTag tag;
        tag.SetAddress(fromAddress);
        packet->AddPacketTag(tag);
        DoForwardUp(packet, fromAddress, toAddress, port);
    }
    
    void
    TricklesSocketBase::ForwardUp6 (Ptr<Packet> packet, Ipv6Header header, uint16_t port, Ptr<Ipv6Interface> incomingInterface)
    {
        NS_LOG_FUNCTION (this << packet << header.GetSourceAddress () << port);
        Address fromAddress = Inet6SocketAddress (header.GetSourceAddress (), port);
        Address toAddress = Inet6SocketAddress (header.GetDestinationAddress (), m_endPoint6->GetLocalPort ());
        SocketAddressTag tag;
        tag.SetAddress(fromAddress);
        packet->AddPacketTag(tag);
        DoForwardUp(packet, fromAddress, toAddress, port);
    }
    
    /* void
     TricklesSocketBase::CompleteFork (Ptr<Packet> packet, const TricklesHeader& h,
     const Address& fromAddress, const Address& toAddress)
     {
     // Get port and address from peer (connecting host)
     if (InetSocketAddress::IsMatchingType (toAddress))
     {
     m_endPoint = m_trickles->Allocate (InetSocketAddress::ConvertFrom (toAddress).GetIpv4 (),
     InetSocketAddress::ConvertFrom (toAddress).GetPort (),
     InetSocketAddress::ConvertFrom (fromAddress).GetIpv4 (),
     InetSocketAddress::ConvertFrom (fromAddress).GetPort ());
     m_endPoint6 = 0;
     }
     else if (Inet6SocketAddress::IsMatchingType (toAddress))
     {
     m_endPoint6 = m_trickles->Allocate6 (Inet6SocketAddress::ConvertFrom (toAddress).GetIpv6 (),
     Inet6SocketAddress::ConvertFrom (toAddress).GetPort (),
     Inet6SocketAddress::ConvertFrom (fromAddress).GetIpv6 (),
     Inet6SocketAddress::ConvertFrom (fromAddress).GetPort ());
     m_endPoint = 0;
     }
     m_trickles->m_sockets.push_back (this);
     
     SetupCallback ();
     ProcessTricklesPacket(packet, h);
     }
     */
    
    void TricklesSocketBase::DoForwardUp(Ptr<Packet> packet, Address fromAddress, Address toAddress, uint16_t port) {
        TricklesHeader tricklesHeader;
        TcpHeader tcpHeader;
        if ((!packet->RemoveHeader(tcpHeader)) || (!packet->RemoveHeader(tricklesHeader))) return;
        // if new packet incoming - fork socket
        //        if ((m_endPoint->GetPeerAddress() == Ipv4Address::GetAny()) || (m_endPoint->GetPeerPort() == 0)) {
        //            Ptr<TricklesSocketBase> newSock = Fork ();
        //            NS_LOG_LOGIC ("Cloned a TricklesSocketBase " << newSock);
        //            Simulator::ScheduleNow (&TricklesSocketBase::CompleteFork, newSock,
        //                                    packet, tricklesHeader, fromAddress, toAddress);
        //        } else {
        // Trickles protocol processing
        ProcessTricklesPacket(packet, tricklesHeader);
        //        }
    }
    
    void
    TricklesSocketBase::ForwardIcmp (Ipv4Address icmpSource, uint8_t icmpTtl,
                                     uint8_t icmpType, uint8_t icmpCode,
                                     uint32_t icmpInfo)
    {
        NS_LOG_FUNCTION (this << icmpSource << (uint32_t)icmpTtl << (uint32_t)icmpType <<
                         (uint32_t)icmpCode << icmpInfo);
        if (!m_icmpCallback.IsNull ())
        {
            m_icmpCallback (icmpSource, icmpTtl, icmpType, icmpCode, icmpInfo);
        }
    }
    
    void
    TricklesSocketBase::ForwardIcmp6 (Ipv6Address icmpSource, uint8_t icmpTtl,
                                      uint8_t icmpType, uint8_t icmpCode,
                                      uint32_t icmpInfo)
    {
        NS_LOG_FUNCTION (this << icmpSource << (uint32_t)icmpTtl << (uint32_t)icmpType <<
                         (uint32_t)icmpCode << icmpInfo);
        if (!m_icmpCallback6.IsNull ())
        {
            m_icmpCallback6 (icmpSource, icmpTtl, icmpType, icmpCode, icmpInfo);
        }
    }
    
    /** Configure the endpoint to a local address. Called by Connect() if Bind() didn't specify one. */
    int
    TricklesSocketBase::SetupEndpoint ()
    {
        NS_LOG_FUNCTION (this);
        Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4> ();
        NS_ASSERT (ipv4 != 0);
        if (ipv4->GetRoutingProtocol () == 0)
        {
            NS_FATAL_ERROR ("No Ipv4RoutingProtocol in the node");
        }
        // Create a dummy packet, then ask the routing function for the best output
        // interface's address
        Ipv4Header header;
        header.SetDestination (m_endPoint->GetPeerAddress ());
        Socket::SocketErrno errno_;
        Ptr<Ipv4Route> route;
        Ptr<NetDevice> oif = m_boundnetdevice;
        route = ipv4->GetRoutingProtocol ()->RouteOutput (Ptr<Packet> (), header, oif, errno_);
        if (route == 0)
        {
            NS_LOG_LOGIC ("Route to " << m_endPoint->GetPeerAddress () << " does not exist");
            NS_LOG_ERROR (errno_);
            m_errno = errno_;
            return -1;
        }
        NS_LOG_LOGIC ("Route exists");
        m_endPoint->SetLocalAddress (route->GetSource ());
        return 0;
    }
    
    int
    TricklesSocketBase::SetupEndpoint6 ()
    {
        NS_LOG_FUNCTION (this);
        Ptr<Ipv6L3Protocol> ipv6 = m_node->GetObject<Ipv6L3Protocol> ();
        NS_ASSERT (ipv6 != 0);
        if (ipv6->GetRoutingProtocol () == 0)
        {
            NS_FATAL_ERROR ("No Ipv6RoutingProtocol in the node");
        }
        // Create a dummy packet, then ask the routing function for the best output
        // interface's address
        Ipv6Header header;
        header.SetDestinationAddress (m_endPoint6->GetPeerAddress ());
        Socket::SocketErrno errno_;
        Ptr<Ipv6Route> route;
        Ptr<NetDevice> oif = m_boundnetdevice;
        route = ipv6->GetRoutingProtocol ()->RouteOutput (Ptr<Packet> (), header, oif, errno_);
        if (route == 0)
        {
            NS_LOG_LOGIC ("Route to " << m_endPoint6->GetPeerAddress () << " does not exist");
            NS_LOG_ERROR (errno_);
            m_errno = errno_;
            return -1;
        }
        NS_LOG_LOGIC ("Route exists");
        m_endPoint6->SetLocalAddress (route->GetSource ());
        return 0;
    }
    
    void TricklesSocketBase::ProcessTricklesPacket(Ptr<Packet> packet, TricklesHeader &th) {
        // Client processing
        NS_LOG_FUNCTION (this);
        if (th.GetPacketType()==CONTINUATION) {
            //SequenceNumber32 from = m_RcvdRequests.firstBlock()->first;
            SequenceNumber32 to = m_RcvdRequests.firstBlock()->second;
            m_RcvdRequests.AddBlock(th.GetTrickleNumber(), th.GetTrickleNumber()+1);

            if (packet->GetSize()) {
                // Очередную порцию данных мы добавляем вперед в буфер, несмотря на возможные потери - это очень оптимистично, но сейчас это сделано чтобы не усложнять и так непростой код
                Ptr<Packet> datapacket = Create<Packet>(packet->GetSize());
                TcpHeader tcpHeader;
                tcpHeader.SetSequenceNumber(m_rxBuffer.NextRxSequence());
                m_rxBuffer.Add(datapacket, tcpHeader);
                packet->RemoveAtEnd(packet->GetSize());
            }
            if ((m_RcvdRequests.firstBlock()->second)>to) {
                m_tsecr = th.GetTSVal();
            }
            th.SetTSEcr(m_tsecr);
            th.SetTSVal(GetCurTSVal());
            th.SetPacketType(REQUEST);
            m_rtt->Measurement(th.GetRTT());
            if (m_rxBuffer.Available()) NotifyDataRecv();
        } else
            // Server processing
            if (th.GetPacketType()==REQUEST) {
                SequenceNumber32 tsval = GetCurTSVal();
                //std::clog << "TSVal = " << tsval << " TSEcr = " << th.GetTSEcr() << " in ms=" << (tsval-th.GetTSEcr())*m_tsgranularity.GetMilliSeconds() << "\n";
                th.SetRTT(MilliSeconds((tsval-th.GetTSEcr())*m_tsgranularity.GetMilliSeconds()));
                th.SetTSVal(tsval);
                m_tsecr = th.GetTSVal();
                th.SetTSEcr(m_tsecr);
                //std::clog << "Server processing: "; LOG_TRICKLES_HEADER(th); std::clog << "\n";
                if (m_rqQueue.size()) NotifyDataRecv();
                th.SetPacketType(CONTINUATION);
            }
    }
    
    SequenceNumber32 TricklesSocketBase::GetCurTSVal() const {
        return(SequenceNumber32(((Simulator::Now()-m_tsstart).GetMilliSeconds()/(m_tsgranularity.GetMilliSeconds()))));
    }
    
    void TricklesSocketBase::QueueToServerApp(Ptr<Packet> packet) {
        m_rqQueue.push_back(packet);
        NotifyDataRecv();
    }
    
    void TricklesSocketBase::CancelAllTimers() {
        
    }
    
    Time TricklesSocketBase::GetRto() const {
        return(Max (m_rtt->GetEstimate () + m_rtt->GetVariation ()*4, Time::FromDouble (1,  Time::S)));
    }

    
} // namespace ns3
