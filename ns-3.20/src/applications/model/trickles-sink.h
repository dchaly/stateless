/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 P.G. Demidov Yaroslavl State University
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
 * Author: Dmitry Ju. Chalyy (chaly@uniyar.ac.ru)
 */

#ifndef TRICKLES_SINK_H
#define TRICKLES_SINK_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"
#include "ns3/socket.h"

using namespace ns3;

/**
 * \ingroup tricklesapps
 * Класс реализует приложение-клиент, которое запрашивает данные с другого хоста сети.
 *
 * После старта приложения осуществляется вызов функции Connect для связанного с ним сокета.
 
 * Затребование каждой порции данных осуществляется при помощи вызова функции сокета Recv. При этом сокет является ответственным за постановку этого требования в очередь и передачу его серверу.
 *
 * Как только очередная порция данных получена от сервера, она автоматически читается из сокета (при помощи того же самого вызова Recv).
 */
class TricklesSink : public Application
{
public:
    static TypeId GetTypeId (void);
    TricklesSink ();
    virtual ~TricklesSink ();
    
    /**
     * \brief Установка параметров приложения
     *
     * \param socket сокет, который используется для работы приложения
     * \param address адрес сервера в сети
     * \param packetSize порция данных, запрашиваемая за один раз
     * \param dataRate скорость, с которой генерируются запросы данных
     
     * Функция устанавливает параметры приложения
     */
    void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, DataRate dataRate);
    
    uint32_t GetTotalRx();
    uint32_t GetTotalRq();
    
private:
    /**
     * \brief Старт процесса генерации запросов
     *
     * Приложение начинает отправлять запросы после вызова этого метода.
     *
     * После вызова этого метода происходит вызов метода Connect на ассоциированном сокете.
     */
    virtual void StartApplication (void);
    /**
     * \brief Остановка приложения и закрытие сокета
     *
     * Приложение перестает отправлять запросы после вызова этого метода.
     *
     * После вызова этого метода ассоциированный с приложением сокет закрывается.
     */
    virtual void StopApplication (void);

    /**
     * \brief Чтение данных из сокета
     *
     * Метод реализует чтение пакетов с данными, полученными от сервера.
     *
     * В результате увеличивается счетчик полученных данных \ref m_totalRx
     */
    void HandleRead (Ptr<Socket>);
    
    /**
     * \brief Планирование следующего запроса
     *
     * Метод производит планирование события, которое сгенерирует следующий запрос данных от сервера в соответствии со скоростью соединения и размером порции данных.
     */
    void ScheduleRq (void);
    /**
     * \brief Генерация следующего запроса данных от сервера.
     *
     * Запрос осуществляется при помощи вызова метода Socket::Recv с параметром \ref m_packetSize.
     * При этом считываются все накопленные данные в сокете и только после этого отправляется запрос.
     *
     * Сокет является ответственным за корректную постановку запроса в очередь.
     */
    void GetData (void);

    /**
     * \brief Ассоциированный с приложением сокет
     */
    Ptr<Socket>     m_socket;
    /**
     * \brief Адрес сервера, с которого будут запрашиваться данные
     */
    Address         m_peer;
    /**
     * \brief Размер одной порции данных
     */
    uint32_t        m_packetSize;
    /**
     * \brief Скорость, с которой данные будут запрашиваться
     */
    DataRate        m_dataRate;
    /**
     * \brief Событие, указывающее на генерацию следующего запроса
     */
    EventId         m_sendEvent;
    /**
     * \brief Флаг, указывающий на то, что процесс генерации запросов запущен
     */
    bool            m_running;
    /**
     * \brief Счетчик, указывающий на объем запрошенных данных
     */
    uint32_t        m_packetsSent;
    /**
     * \brief Счетчик, указывающий на объем полученных данных
     */
    uint32_t        m_totalRx;
};

#endif