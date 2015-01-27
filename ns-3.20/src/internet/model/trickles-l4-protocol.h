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

#ifndef TRICKLES_L4_PROTOCOL_H
#define TRICKLES_L4_PROTOCOL_H

#include <stdint.h>

#include "ns3/packet.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/ptr.h"
#include "ns3/object-factory.h"
#include "ip-l4-protocol.h"
#include "ns3/net-device.h"

namespace ns3 {

class Node;
class Socket;
class TcpHeader;
class Ipv4EndPointDemux;
class Ipv6EndPointDemux;
class Ipv4Interface;
class TricklesSocketBase;
class Ipv4EndPoint;
class Ipv6EndPoint;

/**
 * \ingroup tricklestp
 * \brief Уровень между протоколом IP и сокетами
 * 
 * Этот класс является промежуточным уровнем между сетевым уровнем и сокетами. Он занимается приемом пакетов с уровня IP и передачей их сокету-адресату, а также передачей сформированных сокетом пакетов сетевому уровню.
 
 * Класс также занимается выделением памяти под конечные объекты (ns3::Ipv4EndPoint) для протокола Trickles и должен рассчитывать контрольную сумму для всех пакетов, что сейчас не работает.
*/

class TricklesL4Protocol : public IpL4Protocol {
public:
  static TypeId GetTypeId (void);
    /**
     * \brief Номер, идентифицирующий протокол. Был взят с учетом номеров для других протоколов в системе ns-3, которая, по всей видимости, полагается на номера IANA Assigned Protocol Numbers.
     *
     * Этот номер записывается в каждый отправляемый пакет, что, при получении пакета, позволяет идентифицировать, какому протоколу транспортного уровня необходимо его передать.
     *
     * После передачи пакета объекту этого класса он дальше его направляет в конкретный сокет.
     */
  static const uint8_t PROT_NUMBER;
  /**
   * \brief Конструктор
   */
  TricklesL4Protocol ();
  virtual ~TricklesL4Protocol ();

    /**
     * \brief Функция приписывает объект этого класса к узлу сети
     */
  void SetNode (Ptr<Node> node);

    /**
     * \brief Функция возвращает номер, идентифицирующий протокол
     *
     * см. \ref PROT_NUMBER
     */
  virtual int GetProtocolNumber (void) const;

  /**
   * \brief Функция создает сокет с той версией протокола Trickles, что задана по умолчанию
   * \returns Возвращает созданный сокет
   */
  Ptr<Socket> CreateSocket (void);
    /**
     * \brief Функция создает сокет с той версией протокола Trickles, что задана в качестве аргумента
     * \param socketTypeId версия протокола Trickles
     * \returns Возвращает созданный сокет
     */
  Ptr<Socket> CreateSocket (TypeId socketTypeId);

    /**@{*/
    /**
     * \brief Создание конечной точки транспортного уровня для стека протоколов v4
     */
  Ipv4EndPoint *Allocate (void);
  Ipv4EndPoint *Allocate (Ipv4Address address);
  Ipv4EndPoint *Allocate (uint16_t port);
  Ipv4EndPoint *Allocate (Ipv4Address address, uint16_t port);
  Ipv4EndPoint *Allocate (Ipv4Address localAddress, uint16_t localPort,
                          Ipv4Address peerAddress, uint16_t peerPort);
    /**@}*/
    /**@{*/
    /**
     * \brief Создание конечной точки транспортного уровня для стека протоколов v6
     */
  Ipv6EndPoint *Allocate6 (void);
  Ipv6EndPoint *Allocate6 (Ipv6Address address);
  Ipv6EndPoint *Allocate6 (uint16_t port);
  Ipv6EndPoint *Allocate6 (Ipv6Address address, uint16_t port);
  Ipv6EndPoint *Allocate6 (Ipv6Address localAddress, uint16_t localPort,
                           Ipv6Address peerAddress, uint16_t peerPort);
    /**@}*/
    /**@{*/
    /**
     * \brief Уничтожение конечной точки транспортного уровня 
     */
  void DeAllocate (Ipv4EndPoint *endPoint);
  void DeAllocate (Ipv6EndPoint *endPoint);
    /**@}*/
    /**@{*/
  /**
   * \brief Отправить пакет с помощью протокола Trickles
   * \param packet пакет для отправки
   * \param saddr ns3::Ipv4Address-адрес отправителя
   * \param daddr ns3::Ipv4Address-адрес получателя
   * \param sport порт отправителя
   * \param dport порт получателя
   * \param oif интерфейс, через который осуществляется отправка.
   *
   * В процессе отправки формируется заголовок протокола TCP по умолчанию. Предполагается, что заголовок протокола Trickles уже включен в пакет конечной точкой.
   */
  void Send (Ptr<Packet> packet,
             Ipv4Address saddr, Ipv4Address daddr, 
             uint16_t sport, uint16_t dport, Ptr<NetDevice> oif = 0);
  void Send (Ptr<Packet> packet,
             Ipv6Address saddr, Ipv6Address daddr, 
             uint16_t sport, uint16_t dport, Ptr<NetDevice> oif = 0);
    /**@}*/
    /**@{*/
  /**
   * \brief Получить пакет от сетевого уровня
   * \param p пакет, куда будет скопировано содержимое
   * \param header заголовок протокола IP
   * \param incomingInterface ns3::Ipv4Interface/ns3::Ipv6Interface, откуда был получен пакет
   *
   * Функция получает пакет от сетевого уровня и пытается сопоставить его с конечными точками, которые могли бы быть адресатами этого пакета.
   
   * Если это получается, пакет передается им, в противном случае генерируется RST-пакет, который передается отправителю.
   */
  virtual enum IpL4Protocol::RxStatus Receive (Ptr<Packet> p,
                                                 Ipv4Header const &header,
                                                 Ptr<Ipv4Interface> incomingInterface);
  virtual enum IpL4Protocol::RxStatus Receive (Ptr<Packet> p,
                                                 Ipv6Header const &header,
                                                 Ptr<Ipv6Interface> interface);
    /**@}*/
    /**@{*/
  /**
   * \brief Получение пакета ICMP
   * \param icmpSource IP-адрес отправителя пакета
   * \param icmpTtl TTL из заголовка IP-пакета
   * \param icmpType тип сообщения из заголовка ICMP-пакета
   * \param icmpCode код сообщения из заголовка ICMP-пакета
   * \param icmpInfo 32-битное целое, содержащее дополнительную информацию
   * \param payloadSource IP-адрес отправителя
   * \param payloadDestination IP-адрес получателя
   * \param payload данные ICMP-пакета
   *
   * Функция пытается определить конечную точку, которой предназначен этот пакет и передать его ей.
   */
  virtual void ReceiveIcmp (Ipv4Address icmpSource, uint8_t icmpTtl,
                            uint8_t icmpType, uint8_t icmpCode, uint32_t icmpInfo,
                            Ipv4Address payloadSource,Ipv4Address payloadDestination,
                            const uint8_t payload[8]);
  virtual void ReceiveIcmp (Ipv6Address icmpSource, uint8_t icmpTtl,
                            uint8_t icmpType, uint8_t icmpCode, uint32_t icmpInfo,
                            Ipv6Address payloadSource,Ipv6Address payloadDestination,
                            const uint8_t payload[8]);
    /**@}*/
  // From IpL4Protocol
  virtual void SetDownTarget (IpL4Protocol::DownTargetCallback cb);
  virtual void SetDownTarget6 (IpL4Protocol::DownTargetCallback6 cb);
  // From IpL4Protocol
  virtual IpL4Protocol::DownTargetCallback GetDownTarget (void) const;
  virtual IpL4Protocol::DownTargetCallback6 GetDownTarget6 (void) const;
    
    void DoDisposePublic(void);
    void NotifyNewAggregatePublic(void);

protected:
    /**
     * \brief Обнуление класса
     */
  virtual void DoDispose (void);
  /**
   * \brief This function will notify other components connected to the node that a new stack member is now connected
   * This will be used to notify Layer 3 protocol of layer 4 protocol stack to connect them together.
   */
  virtual void NotifyNewAggregate ();
private:
    /**
     * \brief Узел коммуникационной сети, на котором выполняется экземпляр протокола
     */
  Ptr<Node> m_node;
    /**
     * \brief Конечные точки транспортного уровня протокола Trickles для v4, которые выполняются на данном узле.
     */
  Ipv4EndPointDemux *m_endPoints;
    /**
     * \brief Конечные точки транспортного уровня протокола Trickles для v6, которые выполняются на данном узле.
     */
  Ipv6EndPointDemux *m_endPoints6;
    /**
     * \brief Тип объекта, который рассчитывает время возврата (RTT - round-trip time) для конечных точек
     */
  TypeId m_rttTypeId;
    /**
     * \brief Тип сокета, который создается для конечных точек транспортного уровня
     *
     * Нужен для того, чтобы создавать точки, реализующие различные версии протокола Trickles
     */
  TypeId m_socketTypeId;
private:
  friend class TricklesSocketBase;
    /**@{*/
    /**
     * \brief Передача пакета в сеть с заданным TCP-заголовком
     */
  void SendPacket (Ptr<Packet>, const TcpHeader &,
                   Ipv4Address, Ipv4Address, Ptr<NetDevice> oif = 0);
  void SendPacket (Ptr<Packet>, const TcpHeader &,
                   Ipv6Address, Ipv6Address, Ptr<NetDevice> oif = 0);
    /**@}*/
  TricklesL4Protocol (const TricklesL4Protocol &o);
  TricklesL4Protocol &operator = (const TricklesL4Protocol &o);

    /**
     * \brief Сокеты, которые открыты на транспортном уровне
     */
  std::vector<Ptr<TricklesSocketBase> > m_sockets;
  IpL4Protocol::DownTargetCallback m_downTarget;
  IpL4Protocol::DownTargetCallback6 m_downTarget6;
};

} // namespace ns3

#endif /* TRICKLES_L4_PROTOCOL_H */
