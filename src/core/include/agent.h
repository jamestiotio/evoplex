/**
 *  This file is part of Evoplex.
 *
 *  Evoplex is a multi-agent system for networks.
 *  Copyright (C) 2016 - Marcos Cardinot <marcos@cardinot.net>
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

#ifndef ABSTRACT_AGENT_H
#define ABSTRACT_AGENT_H

#include "attributes.h"
#include "edge.h"
#include "prg.h"

namespace evoplex
{

typedef std::vector<Agent*> Agents;

class Agent
{
    friend class Edge;

public:
    explicit Agent(int id, Attributes attr, int x, int y)
        : m_id(id), m_attrs(attr), m_x(x), m_y(y) {}

    explicit Agent(int id, Attributes attr)
        : Agent(id, attr, 0, id) {}

    inline Agent* clone() { return new Agent(m_id, m_attrs); }

    inline const Attributes& attrs() const { return m_attrs; }
    inline const Value& attr(const char* name) const { return m_attrs.value(name); }
    inline const Value& attr(const int id) const { return m_attrs.value(id); }
    inline void setAttr(const int id, const Value& value) { m_attrs.setValue(id, value); }

    inline const int id() const { return m_id; }

    inline const int x() const { return m_x; }
    inline void setX(int x) { m_x = x; }
    inline const int y() const { return m_y; }
    inline void setY(int y) { m_y = y; }
    inline void setCoords(int x, int y) { setX(x); setY(y); }

    inline const Edges edges() const { return m_edges; }
    inline Agent* neighbour(int localId) const { return m_edges.at(localId)->neighbour(); }
    inline Agent* randNeighbour(PRG* prg) const {
        return m_edges.at(prg->randI(m_edges.size()-1))->neighbour();
    }

private:
    const int m_id;
    int m_x;
    int m_y;
    Attributes m_attrs;
    Edges m_edges;
};
}

#endif // ABSTRACT_AGENT_H
