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

#ifndef TRICKLES_SOCKET_FACTORY_H
#define TRICKLES_SOCKET_FACTORY_H

#include "ns3/socket-factory.h"

namespace ns3 {

class Socket;

/**
 * \ingroup tricklestp
 *
 * \brief API для создания экземпляров сокетов протокола Trickles
 *
 * This abstract class defines the API for Trickles sockets.
 * This class also holds the global default variables used to
 * initialize newly created sockets, such as values that are
 * set through the sysctl or proc interfaces in Linux.
 *
 * Как сказано абзацем выше, это абстрактный класс, часть реализации которого находится в ns3::TricklesSocketFactoryImpl
 *
 * Пока не совсем понятно, зачем устраивать такие сложности, для того чтобы уменьшить вероятность непредвиденных ошибок было сделано по образцу с протоколом TCP.
 *
 */
class TricklesSocketFactory : public SocketFactory
{
public:
  static TypeId GetTypeId (void);

};

} // namespace ns3

#endif /* TRICKLES_SOCKET_FACTORY_H */
