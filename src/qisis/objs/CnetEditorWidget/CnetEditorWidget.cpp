#include "IsisDebug.h"

#include "CnetEditorWidget.h"

#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QByteArray>
#include <QCloseEvent>
#include <QDebug>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelection>
#include <QMenu>
#include <QMessageBox>
#include <QModelIndex>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSplitter>
#include <QString>
#include <QStringList>
#include <QTime>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWhatsThis>

#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "IException.h"

#include "AbstractMeasureItem.h"
#include "AbstractPointItem.h"
#include "AbstractTreeItem.h"
#include "CnetDisplayProperties.h"
#include "CnetEditorSortConfigDialog.h"
#include "FileName.h"
#include "MeasureTableModel.h"
#include "PointTableModel.h"
#include "TableView.h"
#include "TableViewHeader.h"
#include "TreeView.h"
#include "ImageImageTreeModel.h"
#include "FilterWidget.h"
#include "PointMeasureTreeModel.h"
#include "ImagePointTreeModel.h"


namespace Isis {
  using namespace CnetViz;

//**************************************************************
//**************************************************************
//**************************************************************

  const QString CnetEditorWidget::VERSION = "0.1";

//**************************************************************
//**************************************************************
//**************************************************************


  CnetEditorWidget::CnetEditorWidget(ControlNet *cNet,
      QString pathForSettings) {
    nullify();

    m_workingVersion = new QString;
    m_menuActions = new QMap< QAction *, QList< QString > >;
    m_toolBarActions = new QMap< QString, QList< QAction * > >;

    m_updatingSelection = false;

    m_controlNet = cNet;
    connect(CnetDisplayProperties::getInstance(), SIGNAL(compositionFinished()),
        this, SLOT(rebuildModels()));

    m_settingsPath = new QString(pathForSettings);

    QBoxLayout *mainLayout = createMainLayout();
    setLayout(mainLayout);

    createActions();

    readSettings();

    upgradeVersion();

    installEventFilter(this);
  }


  CnetEditorWidget::~CnetEditorWidget() {
    writeSettings();

    delete m_workingVersion;
    m_workingVersion = NULL;

    delete m_settingsPath;
    m_settingsPath = NULL;

    delete m_pointTreeView;
    m_pointTreeView = NULL;

    delete m_imageTreeView;
    m_imageTreeView = NULL;

    delete m_connectionTreeView;
    m_connectionTreeView = NULL;

    delete m_pointTableView;
    m_pointTableView = NULL;

    delete m_measureTableView;
    m_measureTableView = NULL;

    delete m_pointFilterWidget;
    m_pointFilterWidget = NULL;

    delete m_serialFilterWidget;
    m_serialFilterWidget = NULL;

    delete m_connectionFilterWidget;
    m_connectionFilterWidget = NULL;

    delete m_menuActions;
    m_menuActions = NULL;

    delete m_toolBarActions;
    m_toolBarActions = NULL;

    m_pointTableBox = NULL;
    m_measureTableBox = NULL;
    m_mainSplitter = NULL;

    // TODO: null all member widgets!

    delete m_pointModel;
    m_pointModel = NULL;

    delete m_imageModel;
    m_imageModel = NULL;

    delete m_connectionModel;
    m_connectionModel = NULL;
  }


  void CnetEditorWidget::nullify() {
    m_pointTreeView = NULL;
    m_imageTreeView = NULL;
    m_connectionTreeView = NULL;

    m_pointModel = NULL;
    m_imageModel = NULL;
    m_connectionModel = NULL;

    m_pointTableModel = NULL;
    m_measureTableModel = NULL;

    m_pointTableBox = NULL;
    m_measureTableBox = NULL;

    m_pointTableView = NULL;
    m_measureTableView = NULL;

    m_mainSplitter = NULL;

    m_menuActions = NULL;
    m_toolBarActions = NULL;

    m_filterArea = NULL;

    m_pointFilterWidget = NULL;
    m_serialFilterWidget = NULL;
    m_connectionFilterWidget = NULL;

    m_controlNet = NULL;
    m_settingsPath = NULL;
    m_workingVersion = NULL;
  }


  void CnetEditorWidget::rebuildModels(QList<AbstractTreeItem *> itemsToDelete) {
    m_pointModel->stopWorking();
    m_imageModel->stopWorking();
    m_connectionModel->stopWorking();

    bool ignoreAll = false;
    foreach (AbstractTreeItem * item, itemsToDelete) {
      try {
        item->deleteSource();
      }
      catch (IException &e) {
        QString message = e.what();

        if (!ignoreAll) {
          if (item == itemsToDelete.last()) {
            QMessageBox::warning(
              this, "Failed to delete row", message, QMessageBox::Ok);
          }
          else {
            message += "\n\nOkay to continue?";

            QMessageBox::StandardButton status = QMessageBox::warning(
                this, "Failed to delete row", message, QMessageBox::Yes |
                QMessageBox::YesToAll | QMessageBox::No);

            if (status == QMessageBox::YesToAll)
              ignoreAll = true;
            else if (status == QMessageBox::No)
              break;
          }
        }
      }
    }

    m_pointModel->rebuildItems();
    m_imageModel->rebuildItems();
    m_connectionModel->rebuildItems();
  }


  QBoxLayout *CnetEditorWidget::createMainLayout() {
    createPointTreeView();
    createSerialTreeView();
    createConnectionTreeView();

    connect(m_pointTreeView, SIGNAL(activated()),
        m_imageTreeView, SLOT(deactivate()));
    connect(m_pointTreeView, SIGNAL(activated()),
        m_connectionTreeView, SLOT(deactivate()));

    connect(m_imageTreeView, SIGNAL(activated()),
        m_pointTreeView, SLOT(deactivate()));
    connect(m_imageTreeView, SIGNAL(activated()),
        m_connectionTreeView, SLOT(deactivate()));

    connect(m_connectionTreeView, SIGNAL(activated()),
        m_pointTreeView, SLOT(deactivate()));
    connect(m_connectionTreeView, SIGNAL(activated()),
        m_imageTreeView, SLOT(deactivate()));

    createFilterArea();

    createPointTableView();
    m_pointTableBox = new QGroupBox(tr("Control Point Table"));
    QHBoxLayout *pointTableLayout = new QHBoxLayout;
    pointTableLayout->addWidget(m_pointTableView);
    m_pointTableBox->setLayout(pointTableLayout);

    createMeasureTableView();
    m_measureTableBox = new QGroupBox(tr("Control Measure Table"));
    QHBoxLayout *measureTableLayout = new QHBoxLayout;
    measureTableLayout->addWidget(m_measureTableView);
    m_measureTableBox->setLayout(measureTableLayout);

    m_mainSplitter = new QSplitter(Qt::Vertical);
    m_mainSplitter->addWidget(m_pointTableBox);
    m_mainSplitter->addWidget(m_measureTableBox);

    QBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addWidget(m_mainSplitter);

    return mainLayout;
  }


  void CnetEditorWidget::createActions() {
    ASSERT(m_menuActions);

    QAction *freezeTablesAct = new QAction(QIcon(FileName("$base/icons/ice.png").expanded()),
        tr("&Freeze Tables"), this);
    freezeTablesAct->setCheckable(true);
    freezeTablesAct->setToolTip(tr("Freeze tables (filters will not take "
        "effect until unfrozen)"));
    freezeTablesAct->setStatusTip(tr("Freeze tables (filters will not take "
        "effect until unfrozen)"));
    freezeTablesAct->setWhatsThis(tr("<html>When frozen, the contents of the "
        "tables will be locked.  Current filters will not be applied to the "
        "tables until they are unfrozen.</html>"));
    connect(freezeTablesAct, SIGNAL(toggled(bool)),
        this, SLOT(setTablesFrozen(bool)));
    QList< QString > freezeTablesLocation;
    freezeTablesLocation.append(tr("&Tables"));
    m_menuActions->insert(freezeTablesAct, freezeTablesLocation);

    QAction *configureSortAct = new QAction(QIcon(FileName("$base/icons/sort.png").expanded()),
        tr("&Sorting Options..."), this);
    QString configureSortToolTipText = tr("Configure table sorting options");
    configureSortAct->setToolTip(configureSortToolTipText);
    configureSortAct->setStatusTip(configureSortToolTipText);
    configureSortAct->setWhatsThis(tr("<html>Click here to configure options "
        "related to the sorting of table columns.</html>"));
    connect(configureSortAct, SIGNAL(triggered()),
        this, SLOT(configSorting()));
    QList< QString > configureSortLocation;
    configureSortLocation.append(tr("&Tables"));
    m_menuActions->insert(configureSortAct, configureSortLocation);

    QAction *whatsThisAct = QWhatsThis::createAction(this);
    QList< QString > whatsThisLocation;
    whatsThisLocation.append(tr("&Help"));
    m_menuActions->insert(whatsThisAct, whatsThisLocation);

    QList< QAction * > tbActionList;
    tbActionList.append(freezeTablesAct);
    tbActionList.append(configureSortAct);
    m_toolBarActions->insert("settingsToolBar", tbActionList);
  }


  void CnetEditorWidget::createPointTreeView() {
    m_pointTreeView = new TreeView();
    m_pointTreeView->setTitle("Point View");
    m_pointModel = new PointMeasureTreeModel(m_controlNet, m_pointTreeView, qApp);
    m_pointTreeView->setModel(m_pointModel);
  }


  void CnetEditorWidget::createSerialTreeView() {
    m_imageTreeView = new TreeView();
    m_imageTreeView->setTitle("Cube View");
    m_imageModel = new ImagePointTreeModel(m_controlNet, m_imageTreeView, qApp);
    m_imageTreeView->setModel(m_imageModel);
  }


  void CnetEditorWidget::createConnectionTreeView() {
    m_connectionTreeView = new TreeView();
    m_connectionTreeView->setTitle("Cube Connection View");
    m_connectionModel = new ImageImageTreeModel(m_controlNet, m_connectionTreeView, qApp);
    m_connectionTreeView->setModel(m_connectionModel);
  }


  void CnetEditorWidget::createFilterArea() {
    ASSERT(m_pointModel);
    ASSERT(m_imageModel);
    ASSERT(m_connectionModel);

    FilterWidget *pointFilter = new FilterWidget("Points and Measures");
    if (m_pointModel) {
      m_pointModel->setFilter(pointFilter);
    }

    QHBoxLayout *pointFilterLayout = new QHBoxLayout;
    pointFilterLayout->addWidget(pointFilter);
    QWidget *pointArea = new QWidget;
    pointArea->setLayout(pointFilterLayout);
    QScrollArea *pointFilterScrollArea = new QScrollArea;
    pointFilterScrollArea->setWidget(pointArea);
    pointFilterScrollArea->setWidgetResizable(true);
    m_pointFilterWidget = pointFilterScrollArea;

    FilterWidget *serialFilter = new FilterWidget("Images and Points");
    if (m_imageModel) {
      m_imageModel->setFilter(serialFilter);
    }

    QHBoxLayout *serialFilterLayout = new QHBoxLayout;
    serialFilterLayout->addWidget(serialFilter);
    QWidget *serialArea = new QWidget;
    serialArea->setLayout(serialFilterLayout);
    QScrollArea *serialFilterScrollArea = new QScrollArea;
    serialFilterScrollArea->setWidget(serialArea);
    serialFilterScrollArea->setWidgetResizable(true);
    m_serialFilterWidget = serialFilterScrollArea;

    FilterWidget *connectionFilter = new FilterWidget("Connections");
    if (m_connectionModel) {
      m_connectionModel->setFilter(connectionFilter);
    }

    QHBoxLayout *connectionFilterLayout = new QHBoxLayout;
    connectionFilterLayout->addWidget(connectionFilter);
    QWidget *connectionArea = new QWidget;
    connectionArea->setLayout(connectionFilterLayout);
    QScrollArea *connectionFilterScrollArea = new QScrollArea;
    connectionFilterScrollArea->setWidget(connectionArea);
    connectionFilterScrollArea->setWidgetResizable(true);
    m_connectionFilterWidget = connectionFilterScrollArea;
  }


  void CnetEditorWidget::createPointTableView() {
    m_pointTableModel = new PointTableModel(m_pointModel);
    m_pointTableView = new TableView(m_pointTableModel, *m_settingsPath,
        "m_pointTableView");
    m_pointTableView->setWhatsThis("<html>Each row in the table is a control "
        "point.  Each column in the table is an attribute of a control "
        "point.<br/><br/>Cells that are gray are not editable.</html>");
    connect(m_pointTableView, SIGNAL(modelDataChanged()),
        this, SIGNAL(cnetModified()));

    connect(m_pointTreeView, SIGNAL(selectionChanged()),
        m_pointTableView, SLOT(handleModelSelectionChanged()));
    connect(m_pointTableView, SIGNAL(selectionChanged()),
        m_pointTreeView, SLOT(handleModelSelectionChanged()));

    connect(m_pointTableView,
        SIGNAL(rebuildModels(QList< CnetViz::AbstractTreeItem * >)),
        this,
        SLOT(rebuildModels(QList< CnetViz::AbstractTreeItem * >)));

    connect(m_pointTableView, SIGNAL(filterCountsChanged(int, int)),
        this, SLOT(handlePointTableFilterCountsChanged(int, int)));

    for (int i = 0; i < AbstractPointItem::COLS; i++) {
      QAction *act = new QAction(
        AbstractPointItem::getColumnName((AbstractPointItem::Column) i), this);
      act->setCheckable(true);
      connect(act, SIGNAL(toggled(bool)), this, SLOT(pointColToggled()));
      m_pointTableView->getHorizontalHeader()->addAction(act);
    }

    m_pointTableView->getHorizontalHeader()->setContextMenuPolicy(
      Qt::ActionsContextMenu);
  }


  void CnetEditorWidget::createMeasureTableView() {
    m_measureTableModel = new MeasureTableModel(m_pointModel);
    m_measureTableView = new TableView(m_measureTableModel, *m_settingsPath,
        "m_measureTableView");
    m_measureTableView->setWhatsThis("<html>Each row in the table is a control "
        "measure.  Each column in the table is an attribute of a control "
        "measure.<br/><br/>Rows with bold text are reference measures.  "
        "Cells that are gray are not editable.</html>");
    ASSERT(m_pointTableView);
    connect(m_pointTableView,
        SIGNAL(tableSelectionChanged(QList< AbstractTreeItem * >)),
        m_measureTableModel,
        SLOT(handleTreeSelectionChanged(QList<AbstractTreeItem *>)));

    connect(m_measureTableView,
        SIGNAL(tableSelectionChanged(QList< AbstractTreeItem * >)),
        m_pointTableModel,
        SLOT(handleTreeSelectionChanged(QList< AbstractTreeItem * >)));


    connect(m_measureTableView, SIGNAL(modelDataChanged()),
        this, SIGNAL(cnetModified()));
    connect(m_pointTreeView, SIGNAL(selectionChanged()),
        m_measureTableView, SLOT(handleModelSelectionChanged()));
    connect(m_measureTableView, SIGNAL(selectionChanged()),
        m_pointTreeView, SLOT(handleModelSelectionChanged()));
    connect(m_measureTableView,
        SIGNAL(rebuildModels(QList< CnetViz::AbstractTreeItem * >)),
        this,
        SLOT(rebuildModels(QList< CnetViz::AbstractTreeItem * >)));

    connect(m_measureTableView, SIGNAL(filterCountsChanged(int, int)),
        this, SLOT(handleMeasureTableFilterCountsChanged(int, int)));

    for (int i = 0; i < AbstractMeasureItem::COLS; i++) {
      QAction *act = new QAction(AbstractMeasureItem::getColumnName(
          (AbstractMeasureItem::Column) i), this);
      act->setCheckable(true);
      connect(act, SIGNAL(toggled(bool)), this, SLOT(measureColToggled()));
      m_measureTableView->getHorizontalHeader()->addAction(act);
    }

    m_measureTableView->getHorizontalHeader()->setContextMenuPolicy(
      Qt::ActionsContextMenu);
  }


  void CnetEditorWidget::rebuildModels() {
    rebuildModels(QList< AbstractTreeItem * >());
  }


  void CnetEditorWidget::pointColToggled() {
    QList< QAction * > actions =
      m_pointTableView->getHorizontalHeader()->actions();

    for (int i = 0; i < actions.size(); i++) {
      m_pointTableView->setColumnVisible(actions[i]->text(),
          actions[i]->isChecked());
    }
  }


  void CnetEditorWidget::measureColToggled() {
    QList< QAction * > actions =
      m_measureTableView->getHorizontalHeader()->actions();
    for (int i = 0; i < actions.size(); i++)
      m_measureTableView->setColumnVisible(actions[i]->text(),
          actions[i]->isChecked());
  }

  void CnetEditorWidget::handlePointTableFilterCountsChanged(
    int visibleRows, int totalRows) {
    handleTableFilterCountsChanged(visibleRows, totalRows, m_pointTableBox,
        "Control Point Table");
  }


  void CnetEditorWidget::handleMeasureTableFilterCountsChanged(
    int visibleRows, int totalRows) {
    handleTableFilterCountsChanged(visibleRows, totalRows, m_measureTableBox,
        "Control Measure Table");
  }


  void CnetEditorWidget::handleTableFilterCountsChanged(
    int visibleRows, int totalRows, QGroupBox *box, QString initialText) {
    if (box) {
      QString newTitle = initialText + " (";
      if (visibleRows > -1)
        newTitle += QString::number(visibleRows);
      else
        newTitle += "???";

      newTitle += " / " + QString::number(totalRows) + ")";

      box->setTitle(newTitle);
    }
  }


  void CnetEditorWidget::upgradeVersion() {
    if (*m_workingVersion == "") {
      *m_workingVersion = "0.1";
    }

    if (*m_workingVersion != VERSION)
      upgradeVersion();
  }


  void CnetEditorWidget::readSettings() {
    ASSERT(m_workingVersion);
    ASSERT(m_settingsPath);
    ASSERT(m_measureTableView);

    QSettings settings(*m_settingsPath, QSettings::NativeFormat);
    *m_workingVersion = settings.value("version", "").toString();

    m_mainSplitter->restoreState(settings.value("mainSplitter").toByteArray());

    QString key;

    QList< QAction * > actions =
      m_measureTableView->getHorizontalHeader()->actions();
    for (int i = 0; i < actions.size(); i++) {
      key = m_measureTableView->objectName() + " " +
          AbstractMeasureItem::getColumnName((AbstractMeasureItem::Column) i);
      key.replace(" ", "_");
      actions[i]->setChecked(settings.value(key, true).toBool());
    }

    actions = m_pointTableView->getHorizontalHeader()->actions();
    for (int i = 0; i < actions.size(); i++) {
      key = m_pointTableView->objectName() + " " +
          AbstractPointItem::getColumnName((AbstractPointItem::Column) i);
      key.replace(" ", "_");
      actions[i]->setChecked(settings.value(key, true).toBool());
    }

    // Restore sorting configuration settings.
    setMeasureTableSortingEnabled(
      settings.value("measureTableSortingEnabled", true).toBool());
    setMeasureTableSortLimit(
      settings.value("measureTableSortLimit", 500000).toInt());
    setPointTableSortingEnabled(
      settings.value("pointTableSortingEnabled", true).toBool());
    setPointTableSortLimit(
      settings.value("pointTableSortLimit", 100000).toInt());
  }


  void CnetEditorWidget::writeSettings() {
    ASSERT(m_mainSplitter);
    ASSERT(m_settingsPath);
    ASSERT(m_measureTableView);

    QSettings settings(*m_settingsPath, QSettings::NativeFormat);
    settings.setValue("version", VERSION);
    settings.setValue("mainSplitter", m_mainSplitter->saveState());

    QString key;

    QList< QAction * > actions =
      m_measureTableView->getHorizontalHeader()->actions();
    for (int i = 0; i < actions.size(); i++) {
      key = m_measureTableView->objectName() + " " +
          AbstractMeasureItem::getColumnName((AbstractMeasureItem::Column) i);
      key.replace(" ", "_");
      settings.setValue(key, actions[i]->isChecked());
    }

    actions = m_pointTableView->getHorizontalHeader()->actions();
    for (int i = 0; i < actions.size(); i++) {
      key = m_pointTableView->objectName() + " " +
          AbstractPointItem::getColumnName((AbstractPointItem::Column) i);
      key.replace(" ", "_");
      settings.setValue(key, actions[i]->isChecked());
    }

    // Write sorting configuration settings.
    settings.setValue("measureTableSortingEnabled",
        measureTableSortingEnabled());
    settings.setValue("measureTableSortLimit",
        measureTableSortLimit());
    settings.setValue("pointTableSortingEnabled",
        pointTableSortingEnabled());
    settings.setValue("pointTableSortLimit",
        pointTableSortLimit());
  }

  QWidget *CnetEditorWidget::pointTreeView() {
    return m_pointTreeView;
  }


  QWidget *CnetEditorWidget::serialTreeView() {
    return m_imageTreeView;
  }


  QWidget *CnetEditorWidget::connectionTreeView() {
    return m_connectionTreeView;
  }


  QWidget *CnetEditorWidget::pointFilterWidget() {
    return m_pointFilterWidget;
  }


  QWidget *CnetEditorWidget::serialFilterWidget() {
    return m_serialFilterWidget;
  }


  QWidget *CnetEditorWidget::connectionFilterWidget() {
    return m_connectionFilterWidget;
  }


  CnetViz::AbstractTableModel *CnetEditorWidget::measureTableModel() {
    return m_measureTableModel;
  }


  CnetViz::AbstractTableModel *CnetEditorWidget::pointTableModel() {
    return m_pointTableModel;
  }


  QMap< QAction *, QList< QString > > CnetEditorWidget::menuActions() {
    ASSERT(m_menuActions);
    return *m_menuActions;
  }


  QMap< QString, QList< QAction * > > CnetEditorWidget::toolBarActions() {
    ASSERT(m_toolBarActions);
    return *m_toolBarActions;
  }


  ControlNet *CnetEditorWidget::filteredNetwork() const {
    ControlNet *filteredCnet = new ControlNet(*m_controlNet);

    QList<AbstractTreeItem *> networkItems = m_pointModel->getItems(0, -1,
        AbstractTreeModel::MeasureItems | AbstractTreeModel::PointItems, true);

    // Iterate through our copy of the cnet, deleting anything that doesn't
    //   exactly match our networkItems.
    for (int pointIndex = filteredCnet->GetNumPoints() - 1;
        pointIndex >= 0;
        pointIndex--) {
      if (networkItems.isEmpty()) {
        ControlPoint *cp = filteredCnet->GetPoint(pointIndex);
        cp->SetEditLock(false);

        for (int measureIndex = 0;
            measureIndex < cp->GetNumMeasures();
            measureIndex++) {
          cp->GetMeasure(measureIndex)->SetEditLock(false);
        }

        filteredCnet->DeletePoint(cp);
      }
      else if (networkItems.last()->getPointerType() ==
          AbstractTreeItem::Point) {
        ControlPoint *networkItemsCp =
          (ControlPoint *)networkItems.last()->getPointer();
        ControlPoint *cp = filteredCnet->GetPoint(pointIndex);
        if (cp->GetId() != networkItemsCp->GetId()) {
          cp->SetEditLock(false);

          for (int measureIndex = 0;
              measureIndex < cp->GetNumMeasures();
              measureIndex++) {
            cp->GetMeasure(measureIndex)->SetEditLock(false);
          }

          filteredCnet->DeletePoint(cp);
        }
        else {
          networkItems.removeLast();
        }
      }
      else if (networkItems.last()->getPointerType() ==
          AbstractTreeItem::Measure) {
        ControlPoint *cp = filteredCnet->GetPoint(pointIndex);
        ControlMeasure *networkItemsCm =
          (ControlMeasure *)networkItems.last()->getPointer();

        if (cp->GetId() != networkItemsCm->Parent()->GetId()) {
          cp->SetEditLock(false);

          for (int measureIndex = 0;
              measureIndex < cp->GetNumMeasures();
              measureIndex++) {
            cp->GetMeasure(measureIndex)->SetEditLock(false);
          }

          filteredCnet->DeletePoint(cp);
        }
        else {
          // Our CP stays, figure out which CMs stay.
          for (int measureIndex = cp->GetNumMeasures() - 1;
              networkItemsCm && measureIndex >= 0;
              measureIndex--) {
            ControlMeasure *cm = cp->GetMeasure(measureIndex);
            if (cm->GetCubeSerialNumber() !=
                networkItemsCm->GetCubeSerialNumber()) {
              cm->SetEditLock(false);
              cp->Delete(cm);
            }
            else {
              networkItems.removeLast();
              networkItemsCm = NULL;

              if (networkItems.last()->getPointerType() ==
                  AbstractTreeItem::Measure) {
                networkItemsCm =
                  (ControlMeasure *)networkItems.last()->getPointer();
              }
            }
          }

          // We still need to verify the copied CP at this index... although
          //   nothing should go wrong, we know things do go wrong so do
          //   the verify instead of just tossing the last networkItems item.
          pointIndex++;
        }
      }
    }

    return filteredCnet;
  }


  bool CnetEditorWidget::measureTableSortingEnabled() const {
    return m_measureTableModel->sortingIsEnabled();
  }


  int CnetEditorWidget::measureTableSortLimit() const {
    return m_measureTableModel->sortLimit();
  }


  bool CnetEditorWidget::pointTableSortingEnabled() const {
    return m_pointTableModel->sortingIsEnabled();
  }


  int CnetEditorWidget::pointTableSortLimit() const {
    return m_pointTableModel->sortLimit();
  }


  void CnetEditorWidget::setMeasureTableSortingEnabled(bool enabled) {
    m_measureTableModel->setSortingEnabled(enabled);
  }


  void CnetEditorWidget::setMeasureTableSortLimit(int limit) {
    m_measureTableModel->setSortLimit(limit);
  }


  void CnetEditorWidget::setPointTableSortingEnabled(bool enabled) {
    m_pointTableModel->setSortingEnabled(enabled);
  }


  void CnetEditorWidget::setPointTableSortLimit(int limit) {
    m_pointTableModel->setSortLimit(limit);
  }


  void CnetEditorWidget::configSorting() {
    CnetEditorSortConfigDialog *dialog = new CnetEditorSortConfigDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
  }


  void CnetEditorWidget::setTablesFrozen(bool freezeTables) {
    if (freezeTables) {
      m_connectionModel->setFrozen(true);
      m_imageModel->setFrozen(true);
      m_pointModel->setFrozen(true);
    }
    else {
      m_pointModel->setFrozen(false);
      m_imageModel->setFrozen(false);
      m_connectionModel->setFrozen(false);
    }
  }
}

