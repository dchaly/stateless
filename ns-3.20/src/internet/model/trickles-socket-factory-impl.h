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

#ifndef TRICKLES_SOCKET_FACTORY_IMPL_H
#define TRICKLES_SOCKET_FACTORY_IMPL_H

#include "trickles-socket-factory.h"
#include "trickles-l4-protocol.h"
#include "ns3/ptr.h"

namespace ns3 {

class TricklesL4Protocol;

/**
 * \ingroup tricklestp
 *
 * \brief реализация socket factory для протокола Trickles
 *
 * Не совсем понятно зачем производить наследование от ns3::TricklesSocketFactory, но это было сделано в соответствии с реализацией модели протокола TCP.
 *
 * Основная функциональность класса состоит в том, чтобы создавать экземпляры сокетов так, чтобы они одновременно появлялись в объекте класса ns3::TricklesL4Protocol.
 */
class TricklesSocketFactoryImpl : public TricklesSocketFactory
{
public:
  TricklesSocketFactoryImpl ();
  virtual ~TricklesSocketFactoryImpl ();

    /**
     * \brief Задание работающего объекта, реализующего транспортный уровень протокола Trickles
     *
     * В объекте trickles и будут храниться ссылки на сокеты
     */
  void SetTrickles (Ptr<TricklesL4Protocol> trickles);

    /**
     * \brief Создание нового сокета 
     */
  virtual Ptr<Socket> CreateSocket (void);

protected:
    /**
     * \brief Освобождение сокета
     */
  virtual void DoDispose (void);
private:
    /**
     * \brief Указатель на работающий экземпляр протокола Trickles
     */
  Ptr<TricklesL4Protocol> m_trickles;
};

} // namespace ns3

#endif /* TRICKLES_SOCKET_FACTORY_IMPL_H */
