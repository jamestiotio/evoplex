/**
 * Copyright (C) 2016 - Marcos Cardinot
 * @author Marcos Cardinot <mcardinot@gmail.com>
 */

#include <QFile>
#include <QTextStream>
#include <QVector>
#include <QtDebug>

#include "core/project.h"
#include "utils/utils.h"

Project::Project(MainApp* mainapp, MainApp::Model *model,
        const QString& name, const QString& descr, const QString& dir)
    : m_mainApp(mainapp)
    , m_model(model)
    , m_name(name)
    , m_description(descr)
    , m_dir(dir)
{
}

Project::~Project()
{
}

int Project::runExperiment(const int id)
{
    if (id < 0 || id >= m_experiments.size()) {
        return -1;
    }
    return m_mainApp->getProcessesMgr()->addAndPlay(m_experiments.at(id));
}

int Project::newExperiment(Graph* graph, const QVariantHash& params)
{
    IModel* mi = m_model->factory->create();
    Simulation* sim = new Simulation(mi, params.value("stopAt", -1).toInt());
    if (!mi->init(graph, params) || !sim->isValid()) {
        delete mi;
        delete sim;
        return -1;
    }
    m_experiments.append(sim);
    return m_experiments.size() - 1;
}

QVector<IAgent*> Project::getAgentsFromFile(QString filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[Project]: unable to read csv file with the set of agents." << filePath;
        return QVector<IAgent*>();
    }

    QTextStream in(&file);

    // read and validate header
    QStringList header;
    if (!in.atEnd()) {
        header = in.readLine().split(",");
        if (header.size() == m_model->agentParamsDomain.size()) {
            foreach (QString prop, header) {
                if (!m_model->agentParamsDomain.contains(prop)) {
                    header.clear();
                    break;
                }
            }
        } else {
            header.clear();
        }
    }

    if (header.isEmpty()) {
        qWarning() << "[Project]: unable to read the set of agents from" << filePath
                   << "Expected properties:" << m_model->agentParamsDomain.keys();
        return QVector<IAgent*>();
    }

    // create agents
    IModel* model = m_model->factory->create();
    QList<IAgent*> agents;
    while (!in.atEnd()) {
        QStringList values = in.readLine().split(",");
        if (values.size() != header.size()) {
            qWarning() << "[Project]: rows must have the same number of columns!";
            return QVector<IAgent*>();
        }

        IAgent* agent = model->newDefaultAgent();
        for (int i = 0; i < values.size(); ++i) {
            QString domain = m_model->agentParamsDomain.value(header.at(i)).toString();
            if (!Utils::validateProperty(domain, values.at(i))) {
                return QVector<IAgent*>();
            }
            agent->setProperty(header.at(i), QVariant(values.at(i)));
        }
        agents.append(agent);
    }
    delete model;

    return agents.toVector();
}
