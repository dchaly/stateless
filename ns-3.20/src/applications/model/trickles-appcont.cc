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

#include "trickles-appcont.h"


TricklesRequestSizeTag::TricklesRequestSizeTag() {
}

void TricklesRequestSizeTag::SetRequestSize(uint16_t rqSize) {
    m_rqSize = rqSize;
}

uint16_t TricklesRequestSizeTag::GetRequestSize() const {
    return m_rqSize;
}

TypeId TricklesRequestSizeTag::GetTypeId() {
    static TypeId tid = TypeId("ns3::TricklesRequestSizeTag")
    .SetParent<Tag>()
    .AddConstructor<TricklesRequestSizeTag>();
    return tid;
}

TypeId TricklesRequestSizeTag::GetInstanceTypeId() const {
    return GetTypeId();
}

uint32_t TricklesRequestSizeTag::GetSerializedSize() const {
    return 2;
}

void TricklesRequestSizeTag::Serialize(TagBuffer i) const {
    i.WriteU16(m_rqSize);
}

void TricklesRequestSizeTag::Deserialize(TagBuffer i) {
    m_rqSize = i.ReadU16();
}

void TricklesRequestSizeTag::Print(std::ostream &os) const {
    os << "TricklesRqSize=" << (uint16_t) m_rqSize;
}
