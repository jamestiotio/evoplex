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

#include <QtConcurrent>
#include <QFutureWatcher>
#include <QMessageBox>
#include <QMutex>

#include "core/trial.h"

#include "graphwidget.h"
#include "ui_graphwidget.h"
#include "titlebar.h"

namespace evoplex
{

GraphWidget::GraphWidget(MainGUI* mainGUI, ExperimentPtr exp, ExperimentWidget* parent)
    : QDockWidget(parent)
    , m_ui(new Ui_GraphWidget)
    , m_settingsDlg(new GraphSettings(mainGUI, exp, this))
    , m_exp(exp)
    , m_trial(nullptr)
    , m_currStep(-1)
    , m_selectedNode(-1)
    , m_zoomLevel(0)
    , m_nodeSizeRate(10.)
    , m_nodeRadius(m_nodeSizeRate)
    , m_origin(5., 25.)
    , m_cacheStatus(CacheStatus::Ready)
    , m_expWidget(parent)
    , m_posEntered(0., 0.)
    , m_currTrialId(0)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setFocusPolicy(Qt::StrongFocus);

    Q_ASSERT_X(!m_exp->autoDeleteTrials(), "GraphWidget",
               "tried to build a GraphWidget for a experiment that will be auto-deleted!");

    connect(m_exp.get(), SIGNAL(restarted()), SLOT(slotRestarted()));

    QWidget* front = new QWidget(this);
    m_ui->setupUi(front);
    front->setFocusPolicy(Qt::StrongFocus);
    setWidget(front);

    TitleBar* titleBar = new TitleBar(exp.get(), this);
    setTitleBarWidget(titleBar);
    connect(titleBar, SIGNAL(openSettingsDlg()), m_settingsDlg, SLOT(show()));
    connect(titleBar, SIGNAL(trialSelected(quint16)), SLOT(setTrial(quint16)));

    // setTrial() triggers a timer that needs to be exec in the main thread
    // thus, we need to use queuedconnection here
    connect(exp.get(), &Experiment::trialCreated, this,
            [this](int trialId) { if (trialId == m_currTrialId) setTrial(m_currTrialId); },
            Qt::QueuedConnection);

    connect(exp.get(), &Experiment::statusChanged, [this](Status s) {
        for (AttrWidget* aw : m_attrWidgets) {
            if (aw && s != Status::Invalid) {
                aw->setReadOnly(s == Status::Running);
            }
        }
    });

    connect(m_settingsDlg, &GraphSettings::nodeAttrUpdated, [this](int idx) { m_nodeAttr = idx; });
    connect(m_settingsDlg, SIGNAL(nodeCMapUpdated(ColorMap*)), SLOT(setNodeCMap(ColorMap*)));
    m_nodeAttr = m_settingsDlg->nodeAttr();
    m_nodeCMap = m_settingsDlg->nodeCMap();

    connect(m_ui->bZoomIn, SIGNAL(clicked(bool)), SLOT(zoomIn()));
    connect(m_ui->bZoomOut, SIGNAL(clicked(bool)), SLOT(zoomOut()));
    connect(m_ui->bReset, SIGNAL(clicked(bool)), SLOT(resetView()));

    connect(m_ui->bCloseInspector, SIGNAL(clicked(bool)), SLOT(clearSelection()));
    setupInspector();

    m_updateCacheTimer.setSingleShot(true);
    connect(&m_updateCacheTimer, &QTimer::timeout, [this]() { updateCache(true); });

    QPalette pal = palette();
    pal.setColor(QPalette::Background, QColor(239,235,231));
    setAutoFillBackground(true);
    setPalette(pal);
}

GraphWidget::~GraphWidget()
{
    m_trial = nullptr;
    m_exp = nullptr;
    delete m_ui;
    delete m_nodeCMap;
}

void GraphWidget::setupInspector()
{
    QLayoutItem* item;
    while (m_ui->modelAttrs->count() &&
           (item = m_ui->modelAttrs->takeAt(0))) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    m_attrWidgets.clear();
    m_attrWidgets.resize(static_cast<size_t>(m_exp->modelPlugin()->nodeAttrsScope().size()));

    for (auto attrRange : m_exp->modelPlugin()->nodeAttrsScope()) {
        AttrWidget* aw = new AttrWidget(attrRange, this);
        aw->setToolTip(attrRange->attrRangeStr());
        connect(aw, &AttrWidget::valueChanged, [this, aw]() { attrChanged(aw); });
        m_attrWidgets.at(attrRange->id()) = aw;
        m_ui->modelAttrs->insertRow(attrRange->id(), attrRange->attrName(), aw);

        QWidget* l = m_ui->modelAttrs->labelForField(aw);
        l->setToolTip(attrRange->attrName());
        l->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        l->setMinimumWidth(m_ui->lNodeId->minimumWidth());
    }

    m_ui->inspector->adjustSize();
    const int margin = 5;
    m_inspGeo = m_ui->inspector->frameGeometry();
    m_inspGeo.setHeight(m_inspGeo.height() + titleBarWidget()->height() + margin);
    m_inspGeo.setWidth(m_inspGeo.width() + margin);
    m_inspGeo.setX(m_inspGeo.x() - margin);
    m_inspGeo.setY(m_inspGeo.y() - margin);

    m_ui->inspector->hide();
}

void GraphWidget::attrChanged(AttrWidget* aw) const
{
    if (!m_trial || !m_trial->graph() || m_ui->nodeId->value() < 0) {
        return;
    }

    if (m_trial->status() == Status::Running) {
        Node node = m_trial->graph()->node(m_ui->nodeId->value());
        aw->blockSignals(true);
        aw->setValue(node.attr(aw->id()));
        aw->blockSignals(false);
        QMessageBox::warning(parentWidget(), "Graph",
            "You cannot change things in a running experiment.\n"
            "Please, pause it and try again.");
    }

    Node node = m_trial->graph()->node(m_ui->nodeId->value());
    Value v = aw->validate();
    if (v.isValid()) {
        node.setAttr(aw->id(), v);
        // let the other widgets aware that they all need to be updated
        emit (m_expWidget->updateWidgets(true));
    } else {
        aw->blockSignals(true);
        aw->setValue(node.attr(aw->id()));
        aw->blockSignals(false);
        QString err = "The input for '" + aw->attrName() +
                "' is invalid.\nExpected: " + aw->attrRangeStr();
        QMessageBox::warning(parentWidget(), "Graph", err);
    }
}

void GraphWidget::updateCache(bool force)
{
    if (!force) {
        m_updateCacheTimer.start(10);
        return;
    }

    if (m_cacheStatus == CacheStatus::Updating) {
        return;
    }

    QMutex mutex;
    mutex.lock();
    m_cacheStatus = CacheStatus::Updating;
    QFuture<CacheStatus> future = QtConcurrent::run(this, &GraphWidget::refreshCache);
    QFutureWatcher<CacheStatus>* watcher = new QFutureWatcher<CacheStatus>;
    connect(watcher, &QFutureWatcher<int>::finished, [this, watcher]() {
        m_cacheStatus = static_cast<CacheStatus>(watcher->result());
        watcher->deleteLater();
        if (m_cacheStatus == CacheStatus::Scheduled) {
            m_updateCacheTimer.start(10);
        }
        update();
    });
    watcher->setFuture(future);
    mutex.unlock();
}

void GraphWidget::slotRestarted()
{
    if (m_exp->autoDeleteTrials()) {
        close();
        return;
    }
    m_selectedNode = -1;
    setupInspector();
    m_trial = nullptr;
    m_ui->currStep->setText("--");
    updateCache(true);
}

void GraphWidget::setNodeCMap(ColorMap* cmap)
{
    delete m_nodeCMap;
    m_nodeCMap = cmap;
    update();
}

void GraphWidget::setTrial(quint16 trialId)
{
    m_currTrialId = trialId;
    m_trial = m_exp->trial(trialId);
    if (m_trial && m_trial->model()) {
        m_ui->currStep->setText(QString::number(m_trial->step()));
    } else {
        m_ui->currStep->setText("--");
    }
    updateCache();
}

void GraphWidget::zoomIn()
{
    ++m_zoomLevel;
    m_nodeRadius = m_nodeSizeRate * std::pow(1.25f, m_zoomLevel);
    updateCache();
}

void GraphWidget::zoomOut()
{
    --m_zoomLevel;
    m_nodeRadius = m_nodeSizeRate * std::pow(1.25f, m_zoomLevel);
    updateCache();
}

void GraphWidget::resetView()
{
    m_origin = QPointF(5., 25.);
    m_zoomLevel = 0;
    m_nodeRadius = m_nodeSizeRate;
    m_selectedNode = -1;
    m_ui->inspector->hide();
    updateCache();
}

void GraphWidget::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        m_posEntered = e->localPos();
    }
}

void GraphWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (!m_trial || !m_trial->model() || e->button() != Qt::LeftButton ||
            (m_ui->inspector->isVisible() && m_inspGeo.contains(e->pos()))) {
        return;
    }

    if (e->localPos() == m_posEntered) {
        const Node& node = selectNode(e->pos());
        if (node.isNull() || m_selectedNode == node.id()) {
            m_selectedNode = -1;
            m_ui->inspector->hide();
        } else {
            m_selectedNode = node.id();
            updateInspector(node);
            m_ui->inspector->show();
        }
        update();
    } else {
        m_selectedNode = -1;
        m_origin += (e->localPos() - m_posEntered);
        m_ui->inspector->hide();
        updateCache();
    }
}

void GraphWidget::resizeEvent(QResizeEvent* e)
{
    updateCache();
    QWidget::resizeEvent(e);
}

void GraphWidget::updateInspector(const Node& node)
{
    m_ui->nodeId->setValue(node.id());
    m_ui->neighbors->setText(QString::number(node.outDegree()));
    for (auto aw : m_attrWidgets) {
        aw->blockSignals(true);
        aw->setValue(node.attr(aw->id()));
        aw->blockSignals(false);
    }
}

void GraphWidget::updateView(bool forceUpdate)
{
    if (!m_trial || !m_trial->model() || (!forceUpdate && m_trial->step() == m_currStep)) {
        return;
    }
    m_currStep = m_trial->step();
    m_ui->currStep->setText(QString::number(m_currStep));
    update();
}

void GraphWidget::clearSelection()
{
    m_selectedNode = -1;
    m_ui->inspector->hide();
    update();
}

} // evoplex
