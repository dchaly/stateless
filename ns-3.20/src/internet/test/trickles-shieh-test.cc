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

#include "ns3/test.h"
#include "ns3/socket-factory.h"
#include "ns3/trickles-socket-factory.h"
#include "ns3/simulator.h"
#include "ns3/simple-channel.h"
#include "ns3/simple-net-device.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/config.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/ipv6-static-routing.h"
#include "ns3/ipv6-list-routing.h"
#include "ns3/node.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/uinteger.h"
#include "ns3/log.h"

#include "ns3/core-module.h"

#include "ns3/ipv4-end-point.h"
#include "ns3/arp-l3-protocol.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/icmpv4-l4-protocol.h"
#include <ns3/point-to-point-net-device.h>
#include "ns3/point-to-point-module.h"

#include "ns3/icmpv6-l4-protocol.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/trickles-l4-protocol.h"
#include "ns3/trickles-header.h"
#include "ns3/trickles-shieh-header.h"
#include "ns3/trickles-socket-base.h"

#include "ns3/core-module.h"
#include "ns3/global-route-manager.h"
#include "ns3/bridge-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/point-to-point-helper.h"

#include <string>

NS_LOG_COMPONENT_DEFINE ("TricklesShiehTestSuite");

using namespace ns3;

#define NS_TEST_ASSERT_EQUAL(a,b) NS_TEST_ASSERT_MSG_EQ (a,b, "foo")
#define NS_TEST_ASSERT(a) NS_TEST_ASSERT_MSG_EQ (bool(a), true, "foo")

class TricklesShiehTestCase : public TestCase
{
public:
    TricklesShiehTestCase (bool useIpv6);
protected:
    uint32_t GetServerTx() { return m_serverTx; };
    uint32_t GetClientRx() { return m_clientRx; };
    Ptr<Socket> sock_server;
    Ptr<Socket> sock_client;
private:
    void DoRun (void);
    void DoTeardown (void);
    void SetupDefaultSim (void);
    void SetupDefaultSim6 (void);
    void RxServerTrace(const Ipv4Header &h, Ptr<const Packet> p, uint32_t iface);
    void RxClientTrace(const Ipv4Header &h, Ptr<const Packet> p, uint32_t iface);
    void TxServerTrace(const Ipv4Header &h, Ptr<const Packet> p, uint32_t iface);
    void TxClientTrace(const Ipv4Header &h, Ptr<const Packet> p, uint32_t iface);
    
    Ptr<Node> CreateInternetNode (void);
    Ptr<Node> CreateInternetNode6 (void);
    Ptr<SimpleNetDevice> AddSimpleNetDevice (Ptr<Node> node, const char* ipaddr, const char* netmask);
    Ptr<SimpleNetDevice> AddSimpleNetDevice6 (Ptr<Node> node, Ipv6Address ipaddr, Ipv6Prefix prefix);
    void ServerHandleRecv (Ptr<Socket> sock);
    void ClientHandleRecv (Ptr<Socket> sock);
    void ServerScheduleSend(Ptr<Packet> p, Address from);
    virtual void SetupApp() = 0;
    virtual void ServerRx(Ptr<Packet> packet) = 0;
    virtual void ServerTx(Ptr<Packet> packet) = 0;
    virtual void ClientRx(Ptr<Packet> packet) = 0;
    virtual void ClientTx(Ptr<Packet> packet) = 0;
    
    bool m_useIpv6;
    uint32_t m_serverTx;
    uint32_t m_clientRx;
};

#ifdef NS3_LOG_ENABLE
/*static std::string GetString (Ptr<Packet> p)
{
    std::ostringstream oss;
    p->CopyData (&oss, p->GetSize ());
    return oss.str ();
} */
#endif /* NS3_LOG_ENABLE */

TricklesShiehTestCase::TricklesShiehTestCase (bool useIpv6)
: TestCase ("TricklesShieh test case 1"),
sock_server(0),
sock_client(0),
m_useIpv6 (useIpv6),
m_serverTx (0),
m_clientRx (0) {
}

void
TricklesShiehTestCase::DoRun (void)
{
    NS_LOG_FUNCTION(this);

    if (m_useIpv6 == true)
    {
        SetupDefaultSim6 ();
    }
    else
    {
        SetupDefaultSim ();
    }
    SetupApp();
    Simulator::Run ();
    Simulator::Destroy ();
}

void
TricklesShiehTestCase::DoTeardown (void)
{
    Simulator::Destroy ();
}

void
TricklesShiehTestCase::ServerHandleRecv (Ptr<Socket> sock)
{
    NS_LOG_FUNCTION(this);

    Ptr<Packet> packet;
    Address from;
    while ((packet = sock->RecvFrom (from)))
    {
        if (packet->GetSize () == 0)
        { //EOF
            NS_LOG_DEBUG("Received packet of zero size");
            break;
        }
        // Code to process incoming packet
        TricklesHeader tricklesHeader;
        if (!packet->PeekHeader(tricklesHeader)) continue;
        if (tricklesHeader.GetPacketType()==CONTINUATION) {
            Ptr<Packet> data = Create<Packet>(tricklesHeader.GetRequestSize());
            packet->AddAtEnd(data);
            Simulator::Schedule(Seconds(1.0), &TricklesShiehTestCase::ServerScheduleSend, this, packet, from);
        }
    }
}

void  TricklesShiehTestCase::ServerScheduleSend(Ptr<Packet> p, Address from) {
    TricklesHeader tricklesHeader;
    p->PeekHeader(tricklesHeader);
    m_serverTx += tricklesHeader.GetRequestSize();
    sock_server->SendTo(p, 0, from);
}

void
TricklesShiehTestCase::ClientHandleRecv (Ptr<Socket> sock)
{
    NS_LOG_FUNCTION(this);

    Ptr<Packet> packet = 0;
    Address from;
    while (sock->GetRxAvailable()>0)
    {
        packet = sock->RecvFrom(from);
        if (packet == 0) break;
        if (packet->GetSize () == 0)
        { //EOF
            break;
        }
        m_clientRx += packet->GetSize ();
    }
}

Ptr<Node>
TricklesShiehTestCase::CreateInternetNode ()
{
    Ptr<Node> node = CreateObject<Node> ();
    //ARP
    Ptr<ArpL3Protocol> arp = CreateObject<ArpL3Protocol> ();
    node->AggregateObject (arp);
    //IPV4
    Ptr<Ipv4L3Protocol> ipv4 = CreateObject<Ipv4L3Protocol> ();
    //Routing for Ipv4
    Ptr<Ipv4ListRouting> ipv4Routing = CreateObject<Ipv4ListRouting> ();
    ipv4->SetRoutingProtocol (ipv4Routing);
    Ptr<Ipv4StaticRouting> ipv4staticRouting = CreateObject<Ipv4StaticRouting> ();
    ipv4Routing->AddRoutingProtocol (ipv4staticRouting, 0);
    node->AggregateObject (ipv4);
    //ICMP
    Ptr<Icmpv4L4Protocol> icmp = CreateObject<Icmpv4L4Protocol> ();
    node->AggregateObject (icmp);
    //UDP
    Ptr<UdpL4Protocol> udp = CreateObject<UdpL4Protocol> ();
    node->AggregateObject (udp);
    //Trickles
    Ptr<TricklesL4Protocol> trickles = CreateObject<TricklesL4Protocol> ();
    node->AggregateObject (trickles);
    return node;
}

Ptr<SimpleNetDevice>
TricklesShiehTestCase::AddSimpleNetDevice (Ptr<Node> node, const char* ipaddr, const char* netmask)
{
    Ptr<SimpleNetDevice> dev = CreateObject<SimpleNetDevice> ();
    dev->SetAddress (Mac48Address::ConvertFrom (Mac48Address::Allocate ()));
    node->AddDevice (dev);
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
    uint32_t ndid = ipv4->AddInterface (dev);
    Ipv4InterfaceAddress ipv4Addr = Ipv4InterfaceAddress (Ipv4Address (ipaddr), Ipv4Mask (netmask));
    ipv4->AddAddress (ndid, ipv4Addr);
    ipv4->SetUp (ndid);
    return dev;
}

void
TricklesShiehTestCase::SetupDefaultSim (void)
{
    NS_LOG_FUNCTION(this);

    const char* netmask = "255.255.255.0";
    const char* ipaddr0 = "192.168.1.1";
    const char* ipaddr1 = "192.168.1.2";
    Config::SetDefault ("ns3::TricklesL4Protocol::SocketType", StringValue ("ns3::TricklesShieh"));
    Ptr<Node> node0 = CreateInternetNode ();
    Ptr<Node> node1 = CreateInternetNode ();
    Ptr<SimpleNetDevice> dev0 = AddSimpleNetDevice (node0, ipaddr0, netmask);
    Ptr<SimpleNetDevice> dev1 = AddSimpleNetDevice (node1, ipaddr1, netmask);
    
    Ptr<SimpleChannel> channel = CreateObject<SimpleChannel> ();
    dev0->SetChannel (channel);
    dev1->SetChannel (channel);

    Ptr<Ipv4> ipv4_server = node0->GetObject<Ipv4>();
    Ptr<Ipv4> ipv4_client = node1->GetObject<Ipv4>();
    ipv4_server->TraceConnectWithoutContext("LocalDeliver", MakeCallback(&TricklesShiehTestCase::RxServerTrace, this));
    ipv4_server->TraceConnectWithoutContext("SendOutgoing", MakeCallback(&TricklesShiehTestCase::TxServerTrace, this));
    ipv4_client->TraceConnectWithoutContext("LocalDeliver", MakeCallback(&TricklesShiehTestCase::RxClientTrace, this));
    ipv4_client->TraceConnectWithoutContext("SendOutgoing", MakeCallback(&TricklesShiehTestCase::TxClientTrace, this));
    
    Ptr<SocketFactory> sockFactory0 = node0->GetObject<TricklesSocketFactory> ();
    Ptr<SocketFactory> sockFactory1 = node1->GetObject<TricklesSocketFactory> ();
    
    sock_server = sockFactory0->CreateSocket ();
    sock_client = sockFactory1->CreateSocket ();
    
    uint16_t port = 50000;
    InetSocketAddress serverlocaladdr (Ipv4Address::GetAny (), port);
    InetSocketAddress serverremoteaddr (Ipv4Address (ipaddr0), port);
    
    sock_server->Bind (serverlocaladdr);
    sock_server->Listen ();
    
    sock_server->SetRecvCallback (MakeCallback (&TricklesShiehTestCase::ServerHandleRecv, this));
    sock_client->SetRecvCallback (MakeCallback (&TricklesShiehTestCase::ClientHandleRecv, this));
    
    sock_client->Connect (serverremoteaddr);
}

void
TricklesShiehTestCase::SetupDefaultSim6 (void)
{
    Ipv6Prefix prefix = Ipv6Prefix(64);
    Ipv6Address ipaddr0 = Ipv6Address("2001:0100:f00d:cafe::1");
    Ipv6Address ipaddr1 = Ipv6Address("2001:0100:f00d:cafe::2");
    Config::SetDefault ("ns3::TricklesL4Protocol::SocketType", StringValue ("ns3::TricklesShieh"));
    Ptr<Node> node0 = CreateInternetNode6 ();
    Ptr<Node> node1 = CreateInternetNode6 ();
    Ptr<SimpleNetDevice> dev0 = AddSimpleNetDevice6 (node0, ipaddr0, prefix);
    Ptr<SimpleNetDevice> dev1 = AddSimpleNetDevice6 (node1, ipaddr1, prefix);
    
    Ptr<SimpleChannel> channel = CreateObject<SimpleChannel> ();
    dev0->SetChannel (channel);
    dev1->SetChannel (channel);
    
    Ptr<SocketFactory> sockFactory0 = node0->GetObject<TricklesSocketFactory> ();
    Ptr<SocketFactory> sockFactory1 = node1->GetObject<TricklesSocketFactory> ();
    
    sock_server = sockFactory0->CreateSocket ();
    sock_client = sockFactory1->CreateSocket ();
    
    uint16_t port = 50000;
    Inet6SocketAddress serverlocaladdr (Ipv6Address::GetAny (), port);
    Inet6SocketAddress serverremoteaddr (ipaddr0, port);
    
    sock_server->Bind (serverlocaladdr);
    sock_server->Listen ();
    
    sock_server->SetRecvCallback (MakeCallback (&TricklesShiehTestCase::ServerHandleRecv, this));
    sock_client->SetRecvCallback (MakeCallback (&TricklesShiehTestCase::ClientHandleRecv, this));
    
    sock_client->Connect (serverremoteaddr);
}

Ptr<Node>
TricklesShiehTestCase::CreateInternetNode6 ()
{
    Ptr<Node> node = CreateObject<Node> ();
    //IPV6
    Ptr<Ipv6L3Protocol> ipv6 = CreateObject<Ipv6L3Protocol> ();
    //Routing for Ipv6
    Ptr<Ipv6ListRouting> ipv6Routing = CreateObject<Ipv6ListRouting> ();
    ipv6->SetRoutingProtocol (ipv6Routing);
    Ptr<Ipv6StaticRouting> ipv6staticRouting = CreateObject<Ipv6StaticRouting> ();
    ipv6Routing->AddRoutingProtocol (ipv6staticRouting, 0);
    node->AggregateObject (ipv6);
    //ICMP
    Ptr<Icmpv6L4Protocol> icmp = CreateObject<Icmpv6L4Protocol> ();
    node->AggregateObject (icmp);
    //Ipv6 Extensions
    ipv6->RegisterExtensions ();
    ipv6->RegisterOptions ();
    //UDP
    Ptr<UdpL4Protocol> udp = CreateObject<UdpL4Protocol> ();
    node->AggregateObject (udp);
    //TCP
    Ptr<TricklesL4Protocol> trickles = CreateObject<TricklesL4Protocol> ();
    node->AggregateObject (trickles);
    return node;
}

Ptr<SimpleNetDevice>
TricklesShiehTestCase::AddSimpleNetDevice6 (Ptr<Node> node, Ipv6Address ipaddr, Ipv6Prefix prefix)
{
    Ptr<SimpleNetDevice> dev = CreateObject<SimpleNetDevice> ();
    dev->SetAddress (Mac48Address::ConvertFrom (Mac48Address::Allocate ()));
    node->AddDevice (dev);
    Ptr<Ipv6> ipv6 = node->GetObject<Ipv6> ();
    uint32_t ndid = ipv6->AddInterface (dev);
    Ipv6InterfaceAddress ipv6Addr = Ipv6InterfaceAddress (ipaddr, prefix);
    ipv6->AddAddress (ndid, ipv6Addr);
    ipv6->SetUp (ndid);
    return dev;
}

void TricklesShiehTestCase::RxServerTrace(const Ipv4Header &h, Ptr<const Packet> p, uint32_t iface) {
    Ptr<Packet> cp = p->CreateFragment(0, p->GetSize());
    TcpHeader th;
    NS_TEST_ASSERT_MSG_EQ((cp->RemoveHeader(th)>0), true, "No TCP");
    ServerRx(cp);
//    cp->AddHeader(th);
}

void TricklesShiehTestCase::RxClientTrace(const Ipv4Header &h, Ptr<const Packet> p, uint32_t iface) {
    Ptr<Packet> cp = p->CreateFragment(0, p->GetSize());
    TcpHeader th;
    NS_TEST_ASSERT_MSG_EQ((cp->RemoveHeader(th)>0), true, "No TCP");
    ClientRx(cp);
//    cp->AddHeader(th);
}

void TricklesShiehTestCase::TxServerTrace(const Ipv4Header &h, Ptr<const Packet> p, uint32_t iface) {
    Ptr<Packet> cp = p->CreateFragment(0, p->GetSize());
    TcpHeader th;
    NS_TEST_ASSERT_MSG_EQ((cp->RemoveHeader(th)>0), true, "No TCP");
    ServerTx(cp);
//    cp->AddHeader(th);
}

void TricklesShiehTestCase::TxClientTrace(const Ipv4Header &h, Ptr<const Packet> p, uint32_t iface) {
    Ptr<Packet> cp = p->CreateFragment(0, p->GetSize());
    TcpHeader th;
    NS_TEST_ASSERT_MSG_EQ((cp->RemoveHeader(th)>0), true, "No TCP");
    ClientTx(cp);
//    cp->AddHeader(th);
}

class TricklesShiehTestCase1 : public TricklesShiehTestCase {
public:
    TricklesShiehTestCase1 ();
private:
    virtual void SetupApp();
    void ClientGetNextData();
    virtual void ServerRx(Ptr<Packet> packet);
    virtual void ServerTx(Ptr<Packet> packet);
    virtual void ClientRx(Ptr<Packet> packet);
    virtual void ClientTx(Ptr<Packet> packet);
    TricklesSack visible;
    uint16_t packetDiff;
};

TricklesShiehTestCase1::TricklesShiehTestCase1 ()
: TricklesShiehTestCase (false) {
    packetDiff = 0;
}

void TricklesShiehTestCase1::SetupApp() {
    Simulator::Schedule(Seconds(1.0), &TricklesShiehTestCase1::ClientGetNextData, this);
    Simulator::Stop(Seconds(10.0));    
}

void TricklesShiehTestCase1::ClientGetNextData() {
    NS_LOG_FUNCTION(this);
    sock_client->Recv(600000, TricklesSocketBase::QUEUE_RECV);
}

void TricklesShiehTestCase1::ServerRx(Ptr<Packet> packet) {
    NS_LOG_FUNCTION(this << " " << packet);
    TricklesHeader th;
    NS_TEST_ASSERT_MSG_EQ((packet->RemoveHeader(th)>0), true, "Peeking TricklesHeader");
    TricklesShiehHeader tsh;
    NS_TEST_ASSERT_MSG_EQ((packet->RemoveHeader(tsh)>0), true, "Peeking TricklesShiehHeader");
    NS_LOG_DEBUG("Last trickles seen " << th.GetTrickleNumber());
    NS_TEST_ASSERT_MSG_EQ((packet->GetSize()>0), false, "Received packet of size>0");
}

void TricklesShiehTestCase1::ServerTx(Ptr<Packet> packet) {
    NS_LOG_FUNCTION(this << " " << packet);
    TricklesHeader th;
    NS_TEST_ASSERT_MSG_EQ((packet->RemoveHeader(th)>0), true, "Peeking TricklesHeader");
    TricklesShiehHeader tsh;
    NS_TEST_ASSERT_MSG_EQ((packet->RemoveHeader(tsh)>0), true, "Peeking TricklesShiehHeader");
    NS_LOG_DEBUG("Last trickles seen " << th.GetTrickleNumber());
}

void TricklesShiehTestCase1::ClientRx(Ptr<Packet> packet) {
    NS_LOG_FUNCTION(this << " " << packet);
    TricklesHeader th;
    NS_TEST_ASSERT_MSG_EQ((packet->RemoveHeader(th)>0), true, "Peeking TricklesHeader");
    TricklesShiehHeader tsh;
    NS_TEST_ASSERT_MSG_EQ((packet->RemoveHeader(tsh)>0), true, "Peeking TricklesShiehHeader");
    NS_LOG_DEBUG("Last trickles seen " << th.GetTrickleNumber());
    visible.AddBlock(th.GetTrickleNumber(), th.GetTrickleNumber()+1);
    packetDiff++;
}

void TricklesShiehTestCase1::ClientTx(Ptr<Packet> packet) {
    NS_LOG_FUNCTION(this << " " << packet);
    if (packetDiff>0) packetDiff--;
    TricklesHeader th;
    NS_TEST_ASSERT_MSG_EQ((packet->RemoveHeader(th)>0), true, "Peeking TricklesHeader");
    TricklesShiehHeader tsh;
    NS_TEST_ASSERT_MSG_EQ((packet->RemoveHeader(tsh)>0), true, "Peeking TricklesShiehHeader");
//    NS_LOG_DEBUG("Last trickles seen " << th.GetTrickleNumber());
    NS_TEST_ASSERT_MSG_EQ((packet->GetSize()==0), true, "Client sent packet of size>0");
    TricklesSack tsack = th.GetSacks();
    SackConstIterator i = tsack.firstBlock();
    TricklesSack vblocks = visible;
/*    std::clog << "Visible: "; vblocks.Print(std::clog); std::clog << "\n";
    std::clog << "Sack: "; tsack.Print(std::clog); std::clog << "\n";
    std::clog << "Diff: " << packetDiff << "\n"; */
    while (!tsack.isEnd(i)) {
        vblocks.AckBlock(i->first, i->second);
        i++;
    }
//    std::clog << "After: "; vblocks.Print(std::clog); std::clog << "\n";
    NS_TEST_ASSERT_EQUAL(vblocks.DataSize(), packetDiff);
}

static class TricklesShiehTestSuite : public TestSuite
{
public:
    TricklesShiehTestSuite ()
    : TestSuite ("trickles-shieh", UNIT)
    {
        AddTestCase (new TricklesShiehTestCase1 (), TestCase::QUICK);
    }
    
} g_tricklesShiehTestSuite;
