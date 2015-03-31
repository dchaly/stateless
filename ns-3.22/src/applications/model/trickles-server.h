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

#ifndef TRICKLES_SERVER_H
#define TRICKLES_SERVER_H

#include "ns3/application.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"

using namespace ns3;

/**
 * \ingroup applications
 * \defgroup tricklesapps TricklesApps
 *
 * Набор приложений, которые предназначены для работы с семейством асимметричных протоколов.
 *
 * Активной стороной является приложение, моделируемое классом \ref TricklesSink, который отправляет запросы серверу, моделируемому классом \ref TricklesServer. В свою очередь сервер в ответ отправляет данные.
 */

/** \ingroup tricklesapps
 * \class TricklesServer
 * \brief Класс, реализующий серверное приложение для семейства протоколов Trickles
 *
 * Этот класс реализует серверное приложение, которое отправляет пользовательские данные клиенту, который реализуется классом \ref TricklesSink.
 *
 * Сервер не хранит никаких данных о состоянии приложения и обрабатывает запросы по мере их поступления.
 *
 * Существенная часть функциональности сервера реализуется методом #HandleRead.
 */
class TricklesServer : public Application
{
public:
    static TypeId GetTypeId (void);
    TricklesServer ();
    
    virtual ~TricklesServer ();
    
    /**
     * \brief Возврат количества переданных байтов
     *
     * Возвращается общее количество переданных данных. Повторные передачи также учитываются.
     */
    uint32_t GetTotalTx () const;
    /**
     * \brief Возвращает слушающий сокет
     *
     */
    Ptr<Socket> GetListeningSocket (void) const;
    /**
     * \brief Возврат активных сокетов
     *
     * Функция возвращает список активных сокетов
     */
    std::list<Ptr<Socket> > GetAcceptedSockets (void) const;
protected:
    /**
     * \brief Освобождение сокетов
     *
     * Функция освобождает все занятые сокеты
     */
    virtual void DoDispose (void);
private:
    /**
     * \brief Функция вызывается когда приложение стартует
     *
     * Если при вызове функции сокет не задан, то он создается в состоянии LISTEN.
     */
    virtual void StartApplication (void);
    /**
     * \brief Функция вызывается при завершении работы приложения
     *
     * Все сокеты закрываются и callbacks на чтение данных из сокета закрываются.
     */
    virtual void StopApplication (void);
    
    /**
     * \brief Обработка полученных пакетов из сокета
     *
     * Функция считывает полученные из сокета пакеты, а в том случае, если заголовок пакета содержит ns3::TricklesHeader, то формируется пакет данных, который добавляется к полученному. Итоговый пакет отправляется обратно через сокет.
     *
     * Если пакет не содержит ns3::TricklesHeader, то он никак не обрабатывается.
     *
     */
    void HandleRead (Ptr<Socket>);
    /**
     * \brief Установка нового соединения
     *
     * Если устанавливается новое соединение, то функция запоминает сокет, с которого пришел этот запрос, а также устанавливает для этого сокета callback #HandleRead
     */
    void HandleAccept (Ptr<Socket>, const Address& from);
    /**
     * \brief Закрытие сокета
     *
     * Не делаем ничего, пишем в лог. Как callback не установлен.
     */
    void HandlePeerClose (Ptr<Socket>);
    /**
     * \brief Ошибка сокета
     *
     * Не делаем ничего, пишем в лог. Как callback не установлен.
     */
    void HandlePeerError (Ptr<Socket>);
    
    /**
     * \brief Указатель на слушающий сокет
     */
    Ptr<Socket>     m_socket;
    /**
     * \brief Список активных сокетов
     */
    std::list<Ptr<Socket> > m_socketList;
    /**
     * \brief Локальный IP-адрес
     */
    Address         m_local;
    /**
     * \brief Количество переданных данных. 
     *
     * Повторно переданные данные учитываются повторно.
     */
    uint32_t        m_totalTx;
};

#endif