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
#include "ns3/ipv4-raw-socket-factory.h"
#include "ns3/simulator.h"
#include "ns3/simple-channel.h"
#include "ns3/simple-net-device.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/socket.h"
#include "ns3/trickles-sack.h"

#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/inet-socket-address.h"
#include "ns3/boolean.h"

#include "ns3/arp-l3-protocol.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/icmpv4-l4-protocol.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/trickles-header.h"
#include "ns3/trickles-shieh-header.h"

#include <iostream>
#include <string>
#include <sstream>
#include <limits>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

using namespace ns3;

#define NS_TEST_ASSERT_EQUAL(a,b) NS_TEST_ASSERT_MSG_EQ (a,b, "foo")
#define NS_TEST_ASSERT(a) NS_TEST_ASSERT_MSG_EQ (bool(a), true, "foo")

class TricklesHeaderSimpleTest : public TestCase
{
public:
  virtual void DoRun (void);
  TricklesHeaderSimpleTest ();

};


TricklesHeaderSimpleTest::TricklesHeaderSimpleTest ()
  : TestCase ("Trickles Header simple test")
{
}

void
TricklesHeaderSimpleTest::DoRun (void)
{
    for (uint32_t i = 2; i<10; i++) {
        TricklesSack sack;
        TricklesHeader trh;
        trh.SetPacketType(REQUEST);
        trh.SetTrickleNumber(i+1);
        uint16_t m_segSize = 1000;
        trh.SetRequestSize(m_segSize);
        trh.SetRecovery(NO_RECOVERY);
        trh.SetTSVal(SequenceNumber32(0));
        trh.SetTSEcr(SequenceNumber32(0));
        trh.SetRTT(Seconds(0.0));
        trh.SetFirstLoss(SequenceNumber32(0));
        TricklesShiehHeader tsh;
        SequenceNumber32 m_tcpBase = SequenceNumber32(0);
        tsh.SetTcpBase(m_tcpBase);
        uint16_t m_cwnd = i;
        tsh.SetStartCwnd(m_cwnd);
        uint16_t m_ssthresh = 4;
        tsh.SetSsthresh(m_ssthresh);
        sack.AddBlock(SequenceNumber32(i+1001),SequenceNumber32(i+1002));
        trh.SetSacks(sack);
        Ptr<Packet> p = Create<Packet> ();
        p->AddHeader(tsh);
        p->AddHeader(trh);
        NS_TEST_ASSERT_MSG_EQ(((p->RemoveHeader(trh))!=0), true, "Header 1 not found");
        NS_TEST_ASSERT_MSG_EQ(((p->RemoveHeader(tsh))!=0), true, "Header 2 not found");
        NS_TEST_ASSERT_EQUAL (trh.GetTrickleNumber(), SequenceNumber32(i+1));
        NS_TEST_ASSERT_EQUAL (trh.GetPacketType(), REQUEST);
        NS_TEST_ASSERT_EQUAL (trh.GetFirstLoss(), SequenceNumber32(0));
        sack = trh.GetSacks();
        NS_TEST_ASSERT_EQUAL(sack.numBlocks(), 1);
        SackConstIterator j = sack.firstBlock();
        NS_TEST_ASSERT_EQUAL(j->first.GetValue(), i+1001);
        NS_TEST_ASSERT_EQUAL(j->second.GetValue(), i+1002);
        NS_TEST_ASSERT_EQUAL(tsh.GetTcpBase(), SequenceNumber32(0));
        NS_TEST_ASSERT_EQUAL(tsh.GetStartCwnd(), i);
        NS_TEST_ASSERT_EQUAL(tsh.GetSsthresh(), 4);
    }
}


//-----------------------------------------------------------------------------
class TricklesHeaderTestSuite : public TestSuite
{
public:
  TricklesHeaderTestSuite () : TestSuite ("trickles-header", UNIT)
  {
    AddTestCase (new TricklesHeaderSimpleTest, TestCase::QUICK);
  }
} g_tricklesHeaderTestSuite;
