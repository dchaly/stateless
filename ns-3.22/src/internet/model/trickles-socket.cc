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

#include "ns3/object.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/nstime.h"
#include "trickles-socket.h"

NS_LOG_COMPONENT_DEFINE ("TricklesSocket");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TricklesSocket);

TypeId
TricklesSocket::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TricklesSocket")
    .SetParent<Socket> ()
    .AddAttribute ("RcvBufSize",
                   "TricklesSocket maximum receive buffer size (bytes)",
                   UintegerValue (131072),
                   MakeUintegerAccessor (&TricklesSocket::GetRcvBufSize,
                                         &TricklesSocket::SetRcvBufSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("SegmentSize",
                   "Trickles maximum segment size in bytes (may be adjusted based on MTU discovery)",
                   UintegerValue (536),
                   MakeUintegerAccessor (&TricklesSocket::GetSegSize,
                                         &TricklesSocket::SetSegSize),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

TricklesSocket::TricklesSocket ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

TricklesSocket::~TricklesSocket ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

} // namespace ns3
