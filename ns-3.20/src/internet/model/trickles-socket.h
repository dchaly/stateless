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

#ifndef TRICKLES_SOCKET_H
#define TRICKLES_SOCKET_H

#include "ns3/socket.h"
#include "ns3/traced-callback.h"
#include "ns3/callback.h"
#include "ns3/ptr.h"
#include "ns3/object.h"
#include "ns3/nstime.h"

namespace ns3 {

class Node;
class Packet;

/**
 * \ingroup tricklestp
 *
 * \brief (абстрактный) базовый класс для сокетов протокола Trickles
 *
 * Этот абстрактный класс был создан для последующего наследования классами, реализующими сокеты протокола Trickles.
 *
 * Это было сделано по примеру реализации модели протокола TCP.
 *
 * Возможно в будущем стоит избавиться от этого уровня иерархии классов.
 */
class TricklesSocket : public Socket
{
public:
  static TypeId GetTypeId (void);
 
  TricklesSocket (void);
  virtual ~TricklesSocket (void);

private:
  // Indirect the attribute setting and getting through private virtual methods
  virtual void SetRcvBufSize (uint32_t size) = 0;
  virtual uint32_t GetRcvBufSize (void) const = 0;
  virtual void SetSegSize (uint32_t size) = 0;
  virtual uint32_t GetSegSize (void) const = 0;
};

} // namespace ns3

#endif /* TRICKLES_SOCKET_H */


