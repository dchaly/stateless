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

#ifndef TRICKLES_SACK_H
#define TRICKLES_SACK_H

#include <map>
#include "ns3/traced-value.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/sequence-number.h"
#include "ns3/ptr.h"
#include "tcp-header.h"

namespace ns3 {
    class Packet;
    typedef std::map<SequenceNumber32, SequenceNumber32>::const_iterator SackConstIterator;
    typedef std::map<SequenceNumber32, SequenceNumber32>::iterator SackIterator;
    
    /**
     * \ingroup tricklestp
     * \class TricklesSack
     *
     * \brief Класс, позволяющий работать в блоками SACK (Selective Acknowledgements)
     */
    class TricklesSack : public Object
    {
    public:
        static TypeId GetTypeId (void);
        TricklesSack ();
        virtual ~TricklesSack ();

        /**
         * \brief Добавить новый блок
         *
         * Функция добавляет новый блок к текущей структуре данных. Количество блоков может измениться.
         
         * Например, [0:100][200:300]+[50:250] -> [0:300]
         */
        bool AddBlock (SequenceNumber32 from, SequenceNumber32 to);
        /**
         * \brief Удалить блок из структуры данных
         *
         * Функция удаляет блок из структуры данных.
         */
        bool AckBlock (SequenceNumber32 from, SequenceNumber32 to);
        /**
         * \brief Функция возвращает итератор, указывающий на первый SACK-блок
         */
        SackConstIterator firstBlock() const;
        /**
         * \brief Истина, если итератор указывает на последний блок
         */
        bool isEnd(SackConstIterator i) const;
        /**
         * \brief Количество блоков в структуре данных
         */
        uint32_t numBlocks() const;
        /**
         * \brief Объем данных в SACK-блоках
         */
        uint32_t DataSize() const;
        /**
         * \brief Вывод блоков в поток
         */
        void Print(std::ostream &os) const;
        /**
         * \brief Стереть все блоки
         */
        void Clear();
        inline TricklesSack &operator= (const TricklesSack &o) {
            m_blocks = o.m_blocks;
            return *this;
        }
        
    private:
        /**
         * \brief Список SACK-блоков
         */
        std::map<SequenceNumber32, SequenceNumber32> m_blocks;
    };
    
} //namepsace ns3

#endif /* TCP_RX_BUFFER_H */
