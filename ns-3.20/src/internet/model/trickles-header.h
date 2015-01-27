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

#ifndef TRICKLES_HEADER_H
#define TRICKLES_HEADER_H

#include <stdint.h>
#include "ns3/header.h"
#include "ns3/buffer.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/sequence-number.h"
#include "ns3/trickles-sack.h"
#include "ns3/nstime.h"

namespace ns3 {
    /**
     * \ingroup internet
     * \defgroup tricklestp Trickles
     *
     * Набор классов, реализующий сокеты и экземпляры протоколов семейства Trickles.
     *
     * Общая архитектура классов:
     * - ns3::TricklesL4Protocol - опеределяет экземпляр транспортного протокола Trickles на отдельном узле сети
     * - ns3::TricklesSocketFactory - создание отдельных сокетов
     */
    
    /**
     * \brief Тип передаваемого пакета
     */
    typedef enum Trickle_t {
        /**
         * Запрос данных
         */
        REQUEST = 0,
        /**
         * Ответ на запрос данных
         */
        CONTINUATION = 1 } Trickle_t;
    
    /**
     * \brief Что являлось причиной повторной передачи
     */
    typedef enum Recovery_t {
        /**
         * Не было повторной передачи
         */
        NO_RECOVERY = 0,
        /**
         * Fast retransmit
         */
        FAST_RETRANSMIT = 1,
        /*
         * Retransmit time-out
         */
        RTO_TIMEOUT = 2 } Recovery_t;
    
    /**
     * \ingroup tricklestp
     * \class TricklesHeader
     * \brief Класс, моделирующий заголовок протокола Trickles в пакете
     *
     * Класс содержит поля, которые находятся в передаваемых пакетах и необходимых для функционирования протокола Trickles.
     
     * Подразумевается что в передаваемых пакетах есть заголовок протокола TCP, за которым следует заголовок протокола Trickles.
     */
    
    class TricklesHeader : public Header
    {
    private:
        static uint8_t m_magic;
    public:
        TricklesHeader ();
        virtual ~TricklesHeader ();
        
        static TypeId GetTypeId (void);
        virtual TypeId GetInstanceTypeId (void) const;
        virtual void Print (std::ostream &os) const;
        virtual uint32_t GetSerializedSize (void) const;
        virtual void Serialize (Buffer::Iterator start) const;
        virtual uint32_t Deserialize (Buffer::Iterator start);
        Trickle_t GetPacketType() const;
        void SetPacketType(Trickle_t t);
        uint16_t GetRequestSize() const;
        void SetRequestSize(uint16_t reqSize);
        SequenceNumber32 GetTrickleNumber() const;
        void SetTrickleNumber(SequenceNumber32 i);
        void SetTrickleNumber(uint32_t i);
        SequenceNumber32 GetParentNumber() const;
        void SetParentNumber(SequenceNumber32 i);
        void SetParentNumber(uint32_t i);
        Recovery_t IsRecovery() const;
        void SetRecovery(Recovery_t rec);
        SequenceNumber32 GetTSVal() const;
        void SetTSVal(SequenceNumber32 tsval);
        SequenceNumber32 GetTSEcr() const;
        void SetTSEcr(SequenceNumber32 tsecr);
        Time GetRTT() const;
        void SetRTT(Time tval);
        TricklesSack GetSacks() const;
        void SetSacks(TricklesSack newsack);
        SequenceNumber32 GetFirstLoss() const;
        void SetFirstLoss(SequenceNumber32 firstLoss);
    private:
        /**
         * \brief Тип передаваемого пакета
         */
        Trickle_t m_packetType;
        /**
         * \brief Размер запрашиваемой порции данных
         */
        uint16_t m_requestSize;
        /**
         * \brief Порядковый номер пакета
         */
        SequenceNumber32 m_trickleNo;
        /**
         * \brief Порядковый номер пакета, который инициировал создание этого элементарного потока
         */
        SequenceNumber32 m_parentNo;
        /**
         * \brief Флаг, является ли пакет recovery-пакетом (т.е. таким, который передается на стадии восстановления после потерь)
         */
        Recovery_t m_recovery;
        /**
         * \brief Параметр tsval для функционирования временных меток (см. rfc 1323)
         */
        SequenceNumber32 m_tsval;
        /**
         * \brief Параметр tsecr для функционирования временных меток (см. rfc 1323)
         */
        SequenceNumber32 m_tsecr;
        /**
         * \brief Рассчитанное сервером значение RTT (в миллисекундах), что делается на основе временных меток
         */
        Time m_rtt;
        /**
         * \brief SACK-блок, который подтверждает получение запросов клиентской стороной
         */
        TricklesSack m_sacks;
        /**
         * \brief Первый потерянный пакет
         */
        SequenceNumber32 m_firstLoss;
    };

#undef LOG_TRICKLES_PACKET
#define LOG_TRICKLES_PACKET(p) \
if (p) { \
TricklesHeader th; \
if (p->PeekHeader(th)) th.Print(std::clog); else std::clog << " No TricklesHeader "; \
} else { \
std::clog << " Packet is NULL "; }

#undef LOG_TRICKLES_HEADER
#define LOG_TRICKLES_HEADER(t) \
t.Print(std::clog);
    
} // namespace ns3

#endif /* TRICKLES_HEADER */
