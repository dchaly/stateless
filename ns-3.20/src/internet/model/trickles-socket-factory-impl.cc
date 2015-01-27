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

#include "trickles-socket-factory-impl.h"
#include "trickles-l4-protocol.h"
#include "ns3/socket.h"
#include "ns3/assert.h"

namespace ns3 {

TricklesSocketFactoryImpl::TricklesSocketFactoryImpl ()
  : m_trickles (0)
{
}
    
TricklesSocketFactoryImpl::~TricklesSocketFactoryImpl ()
{
  NS_ASSERT (m_trickles == 0);
}

void
TricklesSocketFactoryImpl::SetTrickles (Ptr<TricklesL4Protocol> trickles)
{
  m_trickles = trickles;
}

Ptr<Socket>
TricklesSocketFactoryImpl::CreateSocket (void)
{
  return m_trickles->CreateSocket ();
}

void 
TricklesSocketFactoryImpl::DoDispose (void)
{
  m_trickles = 0;
  TricklesSocketFactory::DoDispose ();
}

} // namespace ns3
