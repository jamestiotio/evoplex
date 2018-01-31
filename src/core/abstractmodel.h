/**
 * Copyright (C) 2016 - Marcos Cardinot
 * @author Marcos Cardinot <mcardinot@gmail.com>
 */

#ifndef ABSTRACT_MODEL_H
#define ABSTRACT_MODEL_H

#include "abstractgraph.h"
#include "agent.h"
#include "prg.h"

namespace evoplex {

class AbstractBaseModel
{
    friend class Experiment;

public:
    inline AbstractGraph* graph() const { return m_graph; }
    inline PRG* prg() const { return m_prg; }
    inline const Attributes* attrs() const { return m_attributes; }
    inline const Value& attr(const QString& name) const { return m_attributes->value(name); }
    inline const Value& attr(int attrId) const { return m_attributes->value(attrId);  }
    inline const QString& attrName(int attrId) const { return m_attributes->name(attrId); }
    inline const int currStep() const { return m_currStep; }
    inline const int status() const { return m_status; }

protected:
    explicit AbstractBaseModel()
        : m_graph(nullptr), m_prg(nullptr), m_attributes(nullptr),
          m_currStep(0), m_status(0) {}

    virtual ~AbstractBaseModel() {
        delete m_graph;
        m_graph = nullptr;
        delete m_prg;
        m_prg = nullptr;
    }

private:
    AbstractGraph* m_graph;
    PRG* m_prg;
    Attributes* m_attributes;
    int m_currStep;
    int m_status;

    // takes the ownership of the graph and the PRG
    inline bool setup(PRG* prg, AbstractGraph* graphObj, Attributes* attrs) {
        Q_ASSERT(!m_prg && !m_graph); // make sure it'll be called only once
        m_prg = prg;
        m_graph = graphObj;
        m_attributes = attrs;
        m_currStep = 0;

        if (!m_prg || !m_graph) {
            return false;
        }
        return true;
    }
};

class AbstractModel : public AbstractBaseModel
{
public:
    // destructor
    virtual ~AbstractModel() {}

    // This method is called before the actual simulation and
    // is mainly used to set the environment and parameters.
    // Return false if anything goes wrong with the initialization.
    virtual bool init() = 0;

    // Implements the metaheuristic.
    // That is, it has to contain all the logic to perform ONE step.
    // If return is true, algorithm converged and simulation will stop asap.
    virtual bool algorithmStep() = 0;

    // This method allows you to custom outputs which, for example,
    // might be used by the GUI to generate custom plots or to be stored in a file.
    // The requested "header" must be defined in the modelMetaData.json file.
    virtual std::vector<Value> customOutputs(const Values& inputs) const {
        return std::vector<Value>();
    }
};
} // evoplex

#endif // ABSTRACT_MODEL_H
