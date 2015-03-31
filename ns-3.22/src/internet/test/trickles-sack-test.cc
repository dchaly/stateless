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

class TricklesSackSimpleTest : public TestCase
{
public:
  virtual void DoRun (void);
  TricklesSackSimpleTest ();

};


TricklesSackSimpleTest::TricklesSackSimpleTest ()
  : TestCase ("Trickles Sack simple test")
{
}

void
TricklesSackSimpleTest::DoRun (void)
{
    TricklesSack sack;
    sack.AddBlock(SequenceNumber32(100), SequenceNumber32(200));
    SackConstIterator i = sack.firstBlock();
    NS_TEST_ASSERT_EQUAL (i->first.GetValue(), 100);
    NS_TEST_ASSERT_EQUAL (i->second.GetValue(), 200);
    i++;
    NS_TEST_ASSERT_EQUAL (sack.isEnd(i), 1);
}

class TricklesSackAddTest : public TestCase
{
public:
    virtual void DoRun (void);
    TricklesSackAddTest ();
private:
    void initTest();
    void FirstTest();
    void SecondTest();
    void ThirdTest();
    void FourthTest();
    void FifthTest();
    void SixthTest();
    void SeventhTest();
    void EighthTest();
    void NinethTest();
    void TenthTest();

    TricklesSack sack;
};


TricklesSackAddTest::TricklesSackAddTest ()
: TestCase ("Trickles Sack add block test")
{
}

// [100:200][300:400]
void TricklesSackAddTest::initTest() {
    sack.Clear();
    sack.AddBlock(SequenceNumber32(100), SequenceNumber32(200));
    sack.AddBlock(SequenceNumber32(300), SequenceNumber32(400));
}

void TricklesSackAddTest::FirstTest() {
    initTest();
    SackConstIterator i = sack.firstBlock();
    i++;
    NS_TEST_ASSERT_EQUAL(sack.isEnd(i), 0);
    NS_TEST_ASSERT_EQUAL (i->first.GetValue(), 300);
    NS_TEST_ASSERT_EQUAL (i->second.GetValue(), 400);
    i++;
    NS_TEST_ASSERT_EQUAL(sack.isEnd(i), 1);
}

// Add [200:250] + [100:200][300:400] -> [100:250][300:400]
void TricklesSackAddTest::SecondTest() {
    initTest();
    sack.AddBlock(SequenceNumber32(200), SequenceNumber32(250));
    NS_TEST_ASSERT_EQUAL(sack.numBlocks(), 2);
    SackConstIterator i = sack.firstBlock();
    NS_TEST_ASSERT_EQUAL(i->first.GetValue(), 100);
    NS_TEST_ASSERT_EQUAL(i->second.GetValue(), 250);
    i++; i++;
    NS_TEST_ASSERT_EQUAL(sack.isEnd(i), 1);
}

// Add [50:100] + [100:200][300:400] -> [50:200][300:400]
void TricklesSackAddTest::ThirdTest() {
    initTest();
    sack.AddBlock(SequenceNumber32(50), SequenceNumber32(100));
    NS_TEST_ASSERT_EQUAL(sack.numBlocks(), 2);
    SackConstIterator i = sack.firstBlock();
    NS_TEST_ASSERT_EQUAL(i->first.GetValue(), 50);
    NS_TEST_ASSERT_EQUAL(i->second.GetValue(), 200);
    i++; i++;
    NS_TEST_ASSERT_EQUAL(sack.isEnd(i), 1);
}

// Add [50:250] + [100:200][300:400] -> [50:250][300:400]
void TricklesSackAddTest::FourthTest() {
    initTest();
    sack.AddBlock(SequenceNumber32(50), SequenceNumber32(250));
    NS_TEST_ASSERT_EQUAL(sack.numBlocks(), 2);
    SackConstIterator i = sack.firstBlock();
    NS_TEST_ASSERT_EQUAL(i->first.GetValue(), 50);
    NS_TEST_ASSERT_EQUAL(i->second.GetValue(), 250);
    i++; i++;
    NS_TEST_ASSERT_EQUAL(sack.isEnd(i), 1);
}

// Add [150:350] + [100:200][300:400] -> [100:400]
void TricklesSackAddTest::FifthTest() {
    initTest();
    sack.AddBlock(SequenceNumber32(150), SequenceNumber32(350));
    NS_TEST_ASSERT_EQUAL(sack.numBlocks(), 1);
    SackConstIterator i = sack.firstBlock();
    NS_TEST_ASSERT_EQUAL(i->first.GetValue(), 100);
    NS_TEST_ASSERT_EQUAL(i->second.GetValue(), 400);
}

// Add [350:450] + [100:200][300:400] -> [100:200][300:450]
void TricklesSackAddTest::SixthTest() {
    initTest();
    sack.AddBlock(SequenceNumber32(350), SequenceNumber32(450));
    NS_TEST_ASSERT_EQUAL(sack.numBlocks(), 2);
    SackConstIterator i = sack.firstBlock();
    i++;
    NS_TEST_ASSERT_EQUAL(i->first.GetValue(), 300);
    NS_TEST_ASSERT_EQUAL(i->second.GetValue(), 450);
}

// Add [50:450] + [100:200][300:400] -> [50:450]
void TricklesSackAddTest::SeventhTest() {
    initTest();
    sack.AddBlock(SequenceNumber32(50), SequenceNumber32(450));
    NS_TEST_ASSERT_EQUAL(sack.numBlocks(), 1);
    SackConstIterator i = sack.firstBlock();
    NS_TEST_ASSERT_EQUAL(i->first.GetValue(), 50);
    NS_TEST_ASSERT_EQUAL(i->second.GetValue(), 450);
}

// Add [200:300] + [100:200][300:400] -> [100:400]
void TricklesSackAddTest::EighthTest() {
    initTest();
    sack.AddBlock(SequenceNumber32(200), SequenceNumber32(300));
    NS_TEST_ASSERT_EQUAL(sack.numBlocks(), 1);
    SackConstIterator i = sack.firstBlock();
    NS_TEST_ASSERT_EQUAL(i->first.GetValue(), 100);
    NS_TEST_ASSERT_EQUAL(i->second.GetValue(), 400);
}

// Add [400:450] + [100:200][300:400] -> [100:200][300:450]
void TricklesSackAddTest::NinethTest() {
    initTest();
    sack.AddBlock(SequenceNumber32(400), SequenceNumber32(450));
    NS_TEST_ASSERT_EQUAL(sack.numBlocks(), 2);
    SackConstIterator i = sack.firstBlock();
    i++;
    NS_TEST_ASSERT_EQUAL(i->first.GetValue(), 300);
    NS_TEST_ASSERT_EQUAL(i->second.GetValue(), 450);
    i++;
    NS_TEST_ASSERT_EQUAL(sack.isEnd(i), 1);
}

// Add [225:250] + [100:200][300:400] -> [100:200][225:250][300:450]
void TricklesSackAddTest::TenthTest() {
    initTest();
    sack.AddBlock(SequenceNumber32(225), SequenceNumber32(250));
    NS_TEST_ASSERT_EQUAL(sack.numBlocks(), 3);
    SackConstIterator i = sack.firstBlock();
    i++;
    NS_TEST_ASSERT_EQUAL(i->first.GetValue(), 225);
    NS_TEST_ASSERT_EQUAL(i->second.GetValue(), 250);
    i++; i++;
    NS_TEST_ASSERT_EQUAL(sack.isEnd(i), 1);
}

void
TricklesSackAddTest::DoRun () {
    FirstTest();
    SecondTest();
    ThirdTest();
    FourthTest();
    FifthTest();
    SixthTest();
    SeventhTest();
    EighthTest();
    NinethTest();
    TenthTest();
}

class TricklesSackAckTest : public TestCase
{
public:
    virtual void DoRun (void);
    TricklesSackAckTest ();
private:
    void initTest();
    void FirstTest();
    void SecondTest();
    void ThirdTest();
    void FourthTest();
    void FifthTest();
//    void SixthTest();
//    void SeventhTest();
//    void EighthTest();
//    void NinethTest();
//    void TenthTest();
    
    TricklesSack sack;
};


TricklesSackAckTest::TricklesSackAckTest ()
: TestCase ("Trickles Sack ack block test")
{
}

// [100:200][300:400]
void TricklesSackAckTest::initTest() {
    sack.Clear();
    sack.AddBlock(SequenceNumber32(100), SequenceNumber32(200));
    sack.AddBlock(SequenceNumber32(300), SequenceNumber32(400));
}

// [100:200][300:400] - [150:250] = [100:150][300:400]
void TricklesSackAckTest::FirstTest() {
    initTest();
    sack.AckBlock(SequenceNumber32(150),SequenceNumber32(250));
    SackConstIterator i = sack.firstBlock();
    NS_TEST_ASSERT_EQUAL(sack.isEnd(i), 0);
    NS_TEST_ASSERT_EQUAL (i->first.GetValue(), 100);
    NS_TEST_ASSERT_EQUAL (i->second.GetValue(), 150);
    i++; i++;
    NS_TEST_ASSERT_EQUAL(sack.isEnd(i), 1);
}

// [100:200][300:400] - [50:250] = [300:400]
void TricklesSackAckTest::SecondTest() {
    initTest();
    sack.AckBlock(SequenceNumber32(50),SequenceNumber32(250));
    NS_TEST_ASSERT_EQUAL(sack.numBlocks(), 1);
    SackConstIterator i = sack.firstBlock();
    NS_TEST_ASSERT_EQUAL(sack.isEnd(i), 0);
    NS_TEST_ASSERT_EQUAL (i->first.GetValue(), 300);
    NS_TEST_ASSERT_EQUAL (i->second.GetValue(), 400);
    i++;
    NS_TEST_ASSERT_EQUAL(sack.isEnd(i), 1);
}

// [100:200][300:400] - [50:100] = [100:200][300:400]
// [100:200][300:400] - [200:300] = [100:200][300:400]
// [100:200][300:400] - [400:500] = [100:200][300:400]
void TricklesSackAckTest::ThirdTest() {
    initTest();
    sack.AckBlock(SequenceNumber32(50),SequenceNumber32(100));
    NS_TEST_ASSERT_EQUAL(sack.numBlocks(), 2);
    SackConstIterator i = sack.firstBlock();
    NS_TEST_ASSERT_EQUAL(sack.isEnd(i), 0);
    NS_TEST_ASSERT_EQUAL (i->first.GetValue(), 100);
    NS_TEST_ASSERT_EQUAL (i->second.GetValue(), 200);
    i++;
    NS_TEST_ASSERT_EQUAL (i->first.GetValue(), 300);
    NS_TEST_ASSERT_EQUAL (i->second.GetValue(), 400);
    i++;
    NS_TEST_ASSERT_EQUAL(sack.isEnd(i), 1);

    initTest();
    sack.AckBlock(SequenceNumber32(200),SequenceNumber32(300));
    NS_TEST_ASSERT_EQUAL(sack.numBlocks(), 2);
    i = sack.firstBlock();
    NS_TEST_ASSERT_EQUAL(sack.isEnd(i), 0);
    NS_TEST_ASSERT_EQUAL (i->first.GetValue(), 100);
    NS_TEST_ASSERT_EQUAL (i->second.GetValue(), 200);
    i++;
    NS_TEST_ASSERT_EQUAL (i->first.GetValue(), 300);
    NS_TEST_ASSERT_EQUAL (i->second.GetValue(), 400);
    i++;
    NS_TEST_ASSERT_EQUAL(sack.isEnd(i), 1);

    initTest();
    sack.AckBlock(SequenceNumber32(400),SequenceNumber32(500));
    NS_TEST_ASSERT_EQUAL(sack.numBlocks(), 2);
    i = sack.firstBlock();
    NS_TEST_ASSERT_EQUAL(sack.isEnd(i), 0);
    NS_TEST_ASSERT_EQUAL (i->first.GetValue(), 100);
    NS_TEST_ASSERT_EQUAL (i->second.GetValue(), 200);
    i++;
    NS_TEST_ASSERT_EQUAL (i->first.GetValue(), 300);
    NS_TEST_ASSERT_EQUAL (i->second.GetValue(), 400);
    i++;
    NS_TEST_ASSERT_EQUAL(sack.isEnd(i), 1);
}

// [100:200][300:400] - [50:500] = 0
void TricklesSackAckTest::FourthTest() {
    initTest();
//    std::cout << "Fourth test: "; sack.Print(std::cout); std::cout << "\n";
    sack.AckBlock(SequenceNumber32(50),SequenceNumber32(500));
//    std::cout << "Result: "; sack.Print(std::cout); std::cout << "\n";
    NS_TEST_ASSERT_EQUAL(sack.numBlocks(), 0);
    SackConstIterator i = sack.firstBlock();
    NS_TEST_ASSERT_EQUAL(sack.isEnd(i), 1);
}

// [100:200][300:400] - [150:350] = [100:150][350:400]
void TricklesSackAckTest::FifthTest() {
    initTest();
//    std::cout << "Fifth test: "; sack.Print(std::cout); std::cout << "\n";
    sack.AckBlock(SequenceNumber32(150),SequenceNumber32(350));
//    std::cout << "Result: "; sack.Print(std::cout); std::cout << "\n";
    NS_TEST_ASSERT_EQUAL(sack.numBlocks(), 2);
    SackConstIterator i = sack.firstBlock();
    NS_TEST_ASSERT_EQUAL(sack.isEnd(i), 0);
    NS_TEST_ASSERT_EQUAL (i->first.GetValue(), 100);
    NS_TEST_ASSERT_EQUAL (i->second.GetValue(), 150);
    i++;
    NS_TEST_ASSERT_EQUAL (i->first.GetValue(), 350);
    NS_TEST_ASSERT_EQUAL (i->second.GetValue(), 400);
    i++;
    NS_TEST_ASSERT_EQUAL(sack.isEnd(i), 1);
}

void
TricklesSackAckTest::DoRun () {
    FirstTest();
    SecondTest();
    ThirdTest();
    FourthTest();
    FifthTest();
}

//-----------------------------------------------------------------------------
class TricklesSackTestSuite : public TestSuite
{
public:
  TricklesSackTestSuite () : TestSuite ("trickles-sack", UNIT)
  {
    AddTestCase (new TricklesSackSimpleTest, TestCase::QUICK);
      AddTestCase(new TricklesSackAddTest, TestCase::QUICK);
      AddTestCase(new TricklesSackAckTest, TestCase::QUICK);
  }
} g_tricklesSackTestSuite;
