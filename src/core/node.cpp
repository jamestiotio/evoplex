/**
 *  This file is part of Evoplex.
 *
 *  Evoplex is a multi-agent system for networks.
 *  Copyright (C) 2018 - Marcos Cardinot <marcos@cardinot.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "include/node.h"
#include "node_p.h"

namespace evoplex {

Node::Node()
    : m_ptr(nullptr)
{}

Node::Node(NodePtr node)
    : m_ptr(node)
{}

Node::Node(const std::pair<const int, Node>& p)
    : m_ptr(p.second.m_ptr)
{}

Node::Node(const std::pair<const int, Edge>& p)
    : m_ptr(p.second.neighbour().m_ptr)
{}

bool Node::isNull() const
{ return m_ptr ? false : true; }

NodePtr Node::clone() const
{ return m_ptr->clone(); }

int Node::id() const
{ return m_ptr->id(); }

int Node::x() const
{ return m_ptr->x(); }

int Node::y() const
{ return m_ptr->y(); }

const Attributes& Node::attrs() const
{ return m_ptr->attrs(); }

const Value& Node::attr(const int& id) const
{ return m_ptr->attr(id); }

const Value& Node::attr(const QString& name) const
{ return m_ptr->attr(name); }

const Node& Node::randNeighbour(PRG* prg) const
{ return m_ptr->randNeighbour(prg); }

const Edges& Node::inEdges() const
{ return m_ptr->inEdges(); }

const Edges& Node::outEdges() const
{ return m_ptr->outEdges(); }

int Node::degree() const
{ return m_ptr->degree(); }

int Node::inDegree() const
{ return m_ptr->inDegree(); }

int Node::outDegree() const
{ return m_ptr->outDegree(); }

void Node::setAttr(const int id, const Value& value)
{ m_ptr->setAttr(id, value); }

void Node::setX(int x)
{ m_ptr->setX(x); }

void Node::setY(int y)
{ m_ptr->setY(y); }

void Node::setCoords(int x, int y)
{ m_ptr->setCoords(x, y); }

} // evoplex
