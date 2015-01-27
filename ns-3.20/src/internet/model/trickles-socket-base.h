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

#ifndef TRICKLES_SOCKET_BASE_H
#define TRICKLES_SOCKET_BASE_H

#include <stdint.h>
#include <queue>
#include "ns3/callback.h"
#include "ns3/traced-callback.h"
#include "ns3/socket.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv6-address.h"
#include "ns3/ipv6-interface.h"
#include "icmpv4.h"
#include "trickles-socket.h"
#include "tcp-tx-buffer.h"
#include "tcp-rx-buffer.h"
#include "rtt-estimator.h"
#include "trickles-sack.h"
#include "trickles-header.h"

namespace ns3 {
    
    class Ipv4EndPoint;
    class Ipv6EndPoint;
    class Node;
    class Packet;
    class TricklesL4Protocol;
    
    static const uint32_t MAX_TRICKLES_TX_SIZE = 65000;
    
    /**
     * \ingroup tricklestp
     * \brief Интерфейс для отдельного сокета протокола Trickles
     *
     * Этот класс наследуется от класса ns3::TricklesSocket и предоставляет интерфейс в виде сокетов для оригинальной модели протокола Trickles (и его модификаций) в системе ns3.
     *
     * Класс обрабатывает запросы пользовательского процесса, однако предполагается, что алгоритм управления потоком реализован в его классах-наследниках.
     *
     * Предполагается, что конкретные версии протокола Trickles реализуются наследованием от этого класса.
     *
     * Модель протокола Trickles была описана в \ref http://mais.uniyar.ac.ru/ru/article/658 
     *
     * Можно считать, что одна сторона соединения выступает в роли клиента, который отправляет запросы, а другая сторона является сервером и в ответ отправляет данные.
     
     * Этот класс реализует API для приложений пользовательского уровня и для клиента и для сервера.
     */
    class TricklesSocketBase : public TricklesSocket
    {
    public:
        static TypeId GetTypeId (void);
        /**
         * \brief Константа используется для задания нового запроса от клиента в методе ::Recv
         */
        static uint32_t QUEUE_RECV;
        /**
         * \brief Конструктор для создания неприкрепленного ни к какому узлу сети сокета
         */
        TricklesSocketBase ();
        /**
         * \brief Конструктор копии
         */
        TricklesSocketBase(const TricklesSocketBase &sock);
        virtual ~TricklesSocketBase ();
        
        /**
         * \brief Метод устанавливает, на каком узле сети создан этот сокет
         *
         * Предполагается, что метод вызывается экземпляром протокола ns3::TricklesL4Protocol при создании сокета.
         */
        void SetNode (Ptr<Node> node);
        /**
         * \brief Метод устанавливает алгоритм, по которому будет производиться рассчет RTT
         *
         * Класс ns3::RttEstimator, рассчитывающий RTT, берется из стандартной поставки ns3.
         */
        void SetRtt (Ptr<RttEstimator> rtt);
        /**
         * \brief Метода устанавливает объект транспортного уровня, через который происходит взаимодействие с коммуникационной сетью
         *
         * Фактически установка этого объекта обеспечивает то, что когда сокет создается/уничтожается это отражается на транспортном уровне в списке активных сокетов.
         */
        void SetTrickles (Ptr<TricklesL4Protocol> trickles);
        
        /**
         * \brief Возврат ошибки, случившиейся при работе сокета
         */
        virtual enum SocketErrno GetErrno (void) const;
        /**
         * \brief Возврат типа сокета
         *
         * Тип сокета для протокола Trickles - NS3_SOCK_STREAM, т.е. последовательность пронумерованных байтов
         */
        virtual enum SocketType GetSocketType (void) const;
        /**
         * \brief Возврат узла сети, на котором открыт этот сокет
         */
        virtual Ptr<Node> GetNode (void) const;
        /**@{*/
        /**
         * \brief Создание конечной точки для данного сокета
         */
        virtual int Bind (void);
        virtual int Bind6 (void);
        virtual int Bind (const Address &address);
        /**@}*/
        /**
         * \brief Закрытие сокета
         *
         * В данный момент ничего не происходит. Любая сторона соединения закрывается в любой момент времени.
         */
        virtual int Close (void);
        /**
         * \brief Прекращение передачи данных
         *
         * Ничего не происходит, только выставляется флажок m_shutdownSend
         */
        virtual int ShutdownSend (void);
        /**
         * \brief Прекращение приема данных
         *
         * Ничего не происходит, только выставляется флажок m_shutdownRecv
         */
        virtual int ShutdownRecv (void);
        /**
         * \brief Установка соединения с адресом address
         *
         * Функция предназначена для вызова с клиентской стороны соединения. 
         *
         * В результате вызова производится только инициализация параметров соединения непосредственно перед началом работы.
         *
         * В отличие от протокола TCP, никаких сегментов удаленному концу соединения не передается.
         */
        virtual int Connect (const Address &address);
        /**
         * \brief Начало прослушивания сокета
         *
         * Функция ничего не делает, так как сервер протокола Trickles не может (?) прослушивать сокет в ожидании соединения.
         */
        virtual int Listen (void);
        virtual uint32_t GetTxAvailable (void) const;
        /**@{*/
        /**
         * \brief Передача данных от приложения
         *
         * Функция передает пакет от приложения в сеть. При вызове функции вызывается UpdateContinuation, которая должна обновить заголовок Trickles, а также заголовок TCP.
         *
         * Предполагается, что этот вызов API между протоколом и пользовательским процессом может быть использован только со стороны сервера, который отправляет данные в ответ на запрос клиента.
         */
        virtual int Send (Ptr<Packet> p, uint32_t flags);
        virtual int SendTo (Ptr<Packet> p, uint32_t flags, const Address &address);
         /**@}*/
        virtual uint32_t GetRxAvailable (void) const;
        /**@{*/
        /**
         * \brief Получение данных из сокета
         * \param maxSize максимальный размер получаемых данных
         * \param flags флаги - возможные значения (0 - прочитать данные из сокета, QUEUE_RECV - осуществить запрос новой порции данных)
         *
         * Получение данных из сокета отличается со стороны клиента и со стороны сервера.
         *
         * <b> Клиент </b>
         *
         * У клиента есть две возможности отправить запрос ::Recv:
         *
         * 1. Без флага QUEUE_RECV, т.е. flags=0. Тогда будет сделана попытка вернуть те данные, которые пришли от сервера. Очевидно, что в начале будет проверена серверная функциональность.
         *
         * 2. С флагом QUEUE_RECV, т.е. flags = QUEUE_RECV. Тогда запрос интерпретируется как запрос новой очередной порции данных, которая еще должна быть получена от сервера.
         *
         * <b> Сервер </b>
         * 
         * Если в очереди поступивших запросов ::m_rqQueue есть запрос, то он обслуживается. В противном случае, запускается функциональность клиента.
         *
         * Из описанных функциональностей более приоритетной является функциональность сервера, т.е. сначала приложению будут переданы все запросы данных, а уже потом будут передаваться данные.
         **/
        virtual Ptr<Packet> Recv (uint32_t maxSize, uint32_t flags);
        virtual Ptr<Packet> RecvFrom (uint32_t maxSize, uint32_t flags,
                                      Address &fromAddress);
        /**@}*/
        virtual int GetSockName (Address &address) const;
        virtual void BindToNetDevice (Ptr<NetDevice> netdevice);
/*        virtual Ptr<TricklesSocketBase> Fork (void) = 0;
        void CompleteFork (Ptr<Packet> p, TricklesHeader th, const Address& fromAddress, const Address& toAddress); */
    protected:
        /**
         * \brief Задание размера буфера, в котором хранятся пришедшие от сервера данные
         *
         * Размер этого буфера влияет на максимальный размер буфера m_rxBuffer. Протокол не должен запрашивать больше данных, чем размер этого буфера.
         */
        virtual void SetRcvBufSize (uint32_t size);
        /**
         * \brief Функция возвращает размер буфера, в котором хранятся данные, пришедшие от сервера
         */
        /**
         * \brief Функция возвращает размер свободного места в открытом окне
         */
        uint32_t FreeWindowSize() const;
        virtual uint32_t GetRcvBufSize (void) const;
        /**
         * \brief Функция задает максимальный размер одного запроса
         */
        virtual void SetSegSize (uint32_t size);
        /**
         * \brief Функция возвращает максимальный размер одного запроса
         */
        virtual uint32_t GetSegSize (void) const;
        /**@{*/
        /**
         * \brief Текущее значение часов для временной метки
         */
        SequenceNumber32 GetCurTSVal() const;
        /**
         * \brief Широковещательная передача запрещена
         *
         * Пока в протоколе не разрешается широковещательная передача. Однако в перспективе возможна реализация чтобы серверная часть была распределенной.
         */
        virtual bool     SetAllowBroadcast (bool allowBroadcast);
        virtual bool     GetAllowBroadcast (void) const;
        /**@}*/
        int SetupCallback (void);        // Common part of the two Bind(), i.e. set callback and remembering local addr:port
        int SetupEndpoint (void);        // Configure m_endpoint for local addr for given remote addr
        int SetupEndpoint6 (void);       // Configure m_endpoint6 for local addr for given remote addr
        void QueueToServerApp(Ptr<Packet>);
        virtual void NewRequest() = 0;
        virtual void ProcessTricklesPacket(Ptr<Packet> packet, TricklesHeader &th);
        void CancelAllTimers(void); // Stop all timers
        void ForwardUp (Ptr<Packet> p, Ipv4Header header, uint16_t port,
                        Ptr<Ipv4Interface> incomingInterface);
        void ForwardUp6 (Ptr<Packet> p, Ipv6Header header, uint16_t port, Ptr<Ipv6Interface> incomingInterface);
        virtual void DoForwardUp(Ptr<Packet> packet, Address fromAddress, Address toAddress, uint16_t port);
    protected:
        
        friend class TricklesSocketFactory;
        void Destroy (void);
        void Destroy6 (void);
        void ForwardIcmp (Ipv4Address icmpSource, uint8_t icmpTtl,
                          uint8_t icmpType, uint8_t icmpCode,
                          uint32_t icmpInfo);
        void ForwardIcmp6 (Ipv6Address icmpSource, uint8_t icmpTtl,
                           uint8_t icmpType, uint8_t icmpCode,
                           uint32_t icmpInfo);
        
        /**@{*/
        /**
         * \brief Переменные, идентифицирующие транспортное соединение
         *
         * Каждая переменная хранит четыре параметра: пару IP-адресов и пару портов для уникальной идентификации транспортного соединения.
         *
         * Переменная m_endPoint предназначена для идентификации соединения IPv4, переменная m_endPoint6 используется для идентификации соединения IPv6.
         */
        Ipv4EndPoint *m_endPoint;
        Ipv6EndPoint *m_endPoint6;
        /**@}*/
        /**
         * \brief Указатель на узел сети, на котором работает сокет
         */
        Ptr<Node> m_node;
        /**
         * \brief Указатель на протокол транспортного уровня, который работает на узле
         */
        Ptr<TricklesL4Protocol> m_trickles;
        /**
         * \brief Переменная используется для расчета времени RTT
         */
        Ptr<RttEstimator> m_rtt;
        /**
         * \brief Количество данных, которые запрошены клиентом от сервера
         *
         * В переменной хранится количество данных, которые клиентское приложение хочет получить от сервера и запросы на которые еще не отправлены.
         */
        uint32_t m_reqDataSize;
        /**
         * \brief Отправленные запросы от клиента к серверу
         *
         * В структуре данных хранятся порядковые номера байтов данных, на которые отправлены запросы к серверу, но еще не получены ответы.
         *
         * При получении данных от сервера этот факт отражается и из списка запрошенных байтов такие данные удаляются.
         *
         * Протоколом Trickles эта структура данных используется для формирования опции SACK.
         */
        TricklesSack m_RcvdRequests;
        /**
         * \brief Данные для передачи приложению
         *
         * В структуре данных хранятся данные, готовые для передачи приложению. Эти данные были получены от сервера.
         */
        TcpRxBuffer m_rxBuffer;
        /**
         * \brief Очередь пакетов, содержащих запросы данных
         *
         * В этой очереди находятся пакеты, пришедшие от клиентской стороны и содержащие запросы на получение данные от сервера.
         */
        std::list<Ptr<Packet> > m_rqQueue;
        /**
         * \brief Максимальный объем запрашиваемых данных за один раз
         */
        uint32_t m_segSize;
        
        /**
         * \brief Момент модельного времени, когда был запущен протокол. Необходимо для реализации временных меток
         */
        Time m_tsstart;
        
        /**
         * \brief Время (в миллисекундах), за которое часы m_tsclock делают один тик. Необходимо для реализации временных меток
         */
        Time m_tsgranularity;
        
        /**
         * \brief Последняя метка, видимая от противоположной стороны
         */
        SequenceNumber32 m_tsecr;
        
        /**
         * \brief Начальный размер окна cwnd
         *
         * \todo Сделать функцию для его задания, пока по умолчанию задается как 3*m_segSize в ns3::TricklesSocketBase::SetSegSize
         */
        
        enum SocketErrno m_errno;
        bool m_shutdownSend;
        bool m_shutdownRecv;
        
        // Socket attributes
        Callback<void, Ipv4Address,uint8_t,uint8_t,uint8_t,uint32_t> m_icmpCallback;
        Callback<void, Ipv6Address,uint8_t,uint8_t,uint8_t,uint32_t> m_icmpCallback6;
    };
    
} // namespace ns3

#endif /* TRICKLES_SOCKET_BASE_H */
