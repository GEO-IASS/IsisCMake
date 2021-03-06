/**
 * @file
 * $Revision: 1.19 $
 * $Date: 2010/03/22 19:44:53 $
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are
 *   public domain. See individual third-party library and package descriptions
 *   for intellectual property information, user agreements, and related
 *   information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or
 *   implied, is made by the USGS as to the accuracy and functioning of such
 *   software and related material nor shall the fact of distribution
 *   constitute any such warranty, and no responsibility is assumed by the
 *   USGS in connection therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html
 *   in a browser or see the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */
#include "CNetSuiteMainWindow.h"

#include <QObject>
#include <QtWidgets>
#include <QSettings>
#include <QTreeView>

#include "AbstractProjectItemView.h"
#include "Directory.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "MosaicSceneWidget.h"
#include "ProgressWidget.h"
#include "Project.h"
#include "ProjectItemModel.h"
#include "ProjectItemTreeView.h"
#include "SensorInfoWidget.h"
#include "TargetInfoWidget.h"

namespace Isis {
  /**
   * Construct the main window. This will create a Directory, the menus, and the dock areas.
   *
   * @param parent The Qt-relationship parent widget (usually NULL in this case)
   *
   * @internal
   *   @history 2016-11-09 Tyler Wilson - Moved the if-block which loads a project from the
   *                             command line from the start of the constructor to the end
   *                             because if there were warnings and errors, they were not
   *                             being output to the Warnings widget since the project is loaded
   *                             before the GUI is constructed.  Fixes #4488
   *   @history 2016-11-09 Ian Humphrey - Added default readSettings() call to load initial
   *                           default project window state. References #4358.
   */
  CNetSuiteMainWindow::CNetSuiteMainWindow(QWidget *parent) :
      QMainWindow(parent) {
    m_maxThreadCount = -1;

    QMdiArea *centralWidget = new QMdiArea;
    centralWidget->setActivationOrder(QMdiArea::StackingOrder);

    connect(centralWidget, SIGNAL( subWindowActivated(QMdiSubWindow *) ),
            this, SLOT( onSubWindowActivated(QMdiSubWindow *) ) );

    setCentralWidget(centralWidget);
    setDockNestingEnabled(true);

    m_activeView = NULL;

    try {
      m_directory = new Directory(this);
      connect(m_directory, SIGNAL( newWidgetAvailable(QWidget *) ),
              this, SLOT( addView(QWidget *) ) );
      connect(m_directory->project(), SIGNAL(projectLoaded(Project *)),
              this, SLOT(readSettings(Project *)));
    }
    catch (IException &e) {
      throw IException(e, IException::Programmer,
          "Could not create Directory.", _FILEINFO_);
    }

    QStringList args = QCoreApplication::arguments();
/**
    if (args.count() == 2) {    
      qDebug() << args.last();
      m_directory->project()->open(args.last());
    }
*/
    m_projectDock = new QDockWidget("Project", this, Qt::SubWindow);
    m_projectDock->setObjectName("projectDock");
    m_projectDock->setFeatures(QDockWidget::DockWidgetMovable |
                              QDockWidget::DockWidgetFloatable);
    m_projectDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    ProjectItemTreeView *projectTreeView = m_directory->addProjectItemTreeView();
    projectTreeView->setInternalModel( m_directory->model() );
    projectTreeView->treeView()->expandAll();
    projectTreeView->installEventFilter(this);
    m_projectDock->setWidget(projectTreeView);

    addDockWidget(Qt::LeftDockWidgetArea, m_projectDock, Qt::Horizontal);

    QDockWidget *warningsDock = new QDockWidget("Warnings", this, Qt::SubWindow);
    warningsDock->setObjectName("warningsDock");
    warningsDock->setFeatures(QDockWidget::DockWidgetClosable |
                         QDockWidget::DockWidgetMovable |
                         QDockWidget::DockWidgetFloatable);
    warningsDock->setWhatsThis(tr("This shows notices and warnings from all operations "
                          "on the current project."));
    warningsDock->setAllowedAreas(Qt::BottomDockWidgetArea);
    m_directory->setWarningContainer(warningsDock);
    addDockWidget(Qt::BottomDockWidgetArea, warningsDock);

    QDockWidget *historyDock = new QDockWidget("History", this, Qt::SubWindow);
    historyDock->setObjectName("historyDock");
    historyDock->setFeatures(QDockWidget::DockWidgetClosable |
                         QDockWidget::DockWidgetMovable |
                         QDockWidget::DockWidgetFloatable);
    historyDock->setWhatsThis(tr("This shows all operations performed on the current project."));
    historyDock->setAllowedAreas(Qt::BottomDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, historyDock);
    m_directory->setHistoryContainer(historyDock);
    tabifyDockWidget(warningsDock, historyDock);

    QDockWidget *progressDock = new QDockWidget("Progress", this, Qt::SubWindow);
    progressDock->setObjectName("progressDock");
    progressDock->setFeatures(QDockWidget::DockWidgetClosable |
                         QDockWidget::DockWidgetMovable |
                         QDockWidget::DockWidgetFloatable);
    progressDock->setAllowedAreas(Qt::BottomDockWidgetArea);
    //m_directory->setProgressContainer(progressDock);
    addDockWidget(Qt::BottomDockWidgetArea, progressDock);
    tabifyDockWidget(historyDock, progressDock);

    warningsDock->raise();

    // Read settings from the default project, "Project"
    readSettings(m_directory->project());

    setTabPosition(Qt::TopDockWidgetArea, QTabWidget::North);
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);

    statusBar()->showMessage("Ready");
    statusBar()->addWidget(m_directory->project()->progress());

    foreach (QProgressBar *progressBar, m_directory->progressBars()) {
      statusBar()->addWidget(progressBar);
    }

    createMenus();
    initializeActions();
    updateMenuActions();

    m_permToolBar = new QToolBar(this);
    m_activeToolBar = new QToolBar(this);
    m_toolPad = new QToolBar(this);

    m_permToolBar->setObjectName("PermanentToolBar");
    m_activeToolBar->setObjectName("ActiveToolBar");
    m_toolPad->setObjectName("ToolPad");

    addToolBar(m_permToolBar);
    addToolBar(m_activeToolBar);
    addToolBar(m_toolPad);
    updateToolBarActions();

    setTabbedViewMode();
    centralWidget->setTabsMovable(true);
    centralWidget->setTabsClosable(true);

    // ken testing
    setActiveView(projectTreeView);
    // ken testing

    if (args.count() == 2) {
      m_directory->project()->open(args.last());
    }
  }


  /**
   * 
   *
   * @param[in] newWidget (QWidget *)
   */
  void CNetSuiteMainWindow::addView(QWidget *newWidget) {
    if ( qobject_cast<SensorInfoWidget *>(newWidget) ||
         qobject_cast<TargetInfoWidget *>(newWidget) ) {
      QDockWidget *dock = new QDockWidget( newWidget->windowTitle() );
      dock->setAttribute(Qt::WA_DeleteOnClose, true);
      dock->setWidget(newWidget);
      splitDockWidget(m_projectDock, dock, Qt::Vertical);
    }
    else {
      if ( QMdiArea *mdiArea = qobject_cast<QMdiArea *>( centralWidget() ) ) {
        mdiArea->addSubWindow(newWidget);
        newWidget->show();
      }
    }
  }


  /**
   * Cleans up the directory.
   */
  CNetSuiteMainWindow::~CNetSuiteMainWindow() {
    m_directory->deleteLater();
  }


  /**
   * Sets the active view and updates the toolbars and menus.
   *
   * @param[in] view (AbstractProjectItemView *) The active view.
   */
  void CNetSuiteMainWindow::setActiveView(AbstractProjectItemView *view) {
    m_activeView = view;
    updateMenuActions();
    updateToolBarActions();
  }


  /**
   * Clears all the menus, then populates the menus with QActions from
   * several sources. The QActions come from an internal list of
   * QActions, the Directory, and the active view.
   */
  void CNetSuiteMainWindow::updateMenuActions() {

    m_fileMenu->clear();
    foreach ( QAction *action, m_directory->fileMenuActions() ) {
      m_fileMenu->addAction(action);
    }
    m_fileMenu->addSeparator();
    if (m_activeView) {
      foreach ( QAction *action, m_activeView->fileMenuActions() ) {
        m_fileMenu->addAction(action);
      }
    }
    m_fileMenu->addSeparator();
    foreach ( QAction *action, m_fileMenuActions ) {
      m_fileMenu->addAction(action);
    }

    m_projectMenu->clear();
    foreach ( QAction *action, m_directory->projectMenuActions() ) {
      m_projectMenu->addAction(action);
    }
    m_projectMenu->addSeparator();
    if (m_activeView) {
      foreach ( QAction *action, m_activeView->projectMenuActions() ) {
        m_projectMenu->addAction(action);
      }
    }
    m_projectMenu->addSeparator();
    foreach ( QAction *action, m_projectMenuActions ) {
      m_projectMenu->addAction(action);
    }

    m_editMenu->clear();
    foreach ( QAction *action, m_directory->editMenuActions() ) {
      m_editMenu->addAction(action);
    }
    m_editMenu->addSeparator();
    if (m_activeView) {
      foreach ( QAction *action, m_activeView->editMenuActions() ) {
        m_editMenu->addAction(action);
      }
    }
    m_editMenu->addSeparator();
    foreach ( QAction *action, m_editMenuActions ) {
      m_editMenu->addAction(action);
    }

    m_viewMenu->clear();
    foreach ( QAction *action, m_directory->viewMenuActions() ) {
      m_viewMenu->addAction(action);
    }
    m_viewMenu->addSeparator();
    foreach ( QAction *action, m_viewMenuActions ) {
      m_viewMenu->addAction(action);
    }
    m_viewMenu->addSeparator();
    if (m_activeView) {
      foreach ( QAction *action, m_activeView->viewMenuActions() ) {
        m_viewMenu->addAction(action);
      }
    }

    m_settingsMenu->clear();
    foreach ( QAction *action, m_directory->settingsMenuActions() ) {
      m_settingsMenu->addAction(action);
    }
    m_settingsMenu->addSeparator();
    if (m_activeView) {
      foreach ( QAction *action, m_activeView->settingsMenuActions() ) {
        m_settingsMenu->addAction(action);
      }
    }
    m_settingsMenu->addSeparator();
    foreach ( QAction *action, m_settingsMenuActions ) {
      m_settingsMenu->addAction(action);
    }

    m_helpMenu->clear();
    foreach ( QAction *action, m_directory->helpMenuActions() ) {
      m_helpMenu->addAction(action);
    }
    m_helpMenu->addSeparator();
    if (m_activeView) {
      foreach ( QAction *action, m_activeView->helpMenuActions() ) {
        m_helpMenu->addAction(action);
      }
    }
    m_helpMenu->addSeparator();
    foreach ( QAction *action, m_helpMenuActions ) {
      m_helpMenu->addAction(action);
    }
  }


  /**
   * Clears the tool bars and repopulates them with QActions from
   * several sources. Actions are taken from an internal list of
   * QActions, the Directory, and the active view.
   */
  void CNetSuiteMainWindow::updateToolBarActions() {
    m_permToolBar->clear();
    foreach( QAction *action, m_directory->permToolBarActions() ) {
      m_permToolBar->addAction(action);
    }
    foreach(QAction *action, m_permToolBarActions) {
      m_permToolBar->addAction(action);
    }
    m_permToolBar->addSeparator();
    if (m_activeView) {
      foreach ( QAction *action, m_activeView->permToolBarActions() ) {
        m_permToolBar->addAction(action);
      }
    }

    m_activeToolBar->clear();
    if (m_activeView) {
      foreach ( QAction *action, m_activeView->activeToolBarActions() ) {
        m_activeToolBar->addAction(action);
      }
    }

    m_toolPad->clear();
    if (m_activeView) {
      foreach ( QAction *action, m_activeView->toolPadActions() ) {
        m_toolPad->addAction(action);
      }
    }
  }


  /**
   * Filters out events from views so they can be handled by the main
   * window. Filters out DragEnter Drop and ContextMenu events from
   * views.
   *
   * @param[in] watched (QObject *) The object being filtered.
   * @param[in] event (QEvent *) The event that may be filtered.
   */
  bool CNetSuiteMainWindow::eventFilter(QObject *watched, QEvent *event) {
    if ( AbstractProjectItemView *view = qobject_cast<AbstractProjectItemView *>(watched) ) {
      if (event->type() == QEvent::DragEnter) {
        return true;
      }
      else if (event->type() == QEvent::Drop) {
        return true;
      }
      else if (event->type() == QEvent::ContextMenu) {
        QMenu contextMenu;

        QList<QAction *> viewActions = view->contextMenuActions();

        if ( !viewActions.isEmpty() ) {
          foreach (QAction *action, viewActions) {
            if (action) {
              contextMenu.addAction(action);
            }
            else {
              contextMenu.addSeparator();
            }
          }
          contextMenu.addSeparator();
        }

        QList<QAction *> workOrders = m_directory->supportedActions( view->currentItem() );

        if ( !workOrders.isEmpty() ) {
          foreach (QAction *action, workOrders) {
            contextMenu.addAction(action);
          }
          contextMenu.addSeparator();
        }

        contextMenu.exec( static_cast<QContextMenuEvent *>(event)->globalPos() );

        return true;
      }
    }
 
    return QMainWindow::eventFilter(watched, event);
  }


  /**
   * This method takes the max thread count setting and asks
   * QtConcurrent to respect it.
   */
  void CNetSuiteMainWindow::applyMaxThreadCount() {
    if (m_maxThreadCount <= 1) {
      // Allow QtConcurrent to use every core and starve the GUI thread
      QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());
    }
    else {
      // subtract 1 to account for the GUI thread
      QThreadPool::globalInstance()->setMaxThreadCount(m_maxThreadCount - 1);
    }
  }


  /**
   * Initializes the internal lists of actions of the main window for
   * use in the menus and toolbars.
   */
  void CNetSuiteMainWindow::initializeActions() {
    QAction *exitAction = new QAction("E&xit", this);
    exitAction->setIcon( QIcon::fromTheme("window-close") );
    connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));
    m_fileMenuActions.append(exitAction);
    m_permToolBarActions.append(exitAction);

    QAction *undoAction = m_directory->undoAction();
    undoAction->setShortcut(Qt::Key_Z | Qt::CTRL);

    QAction *redoAction = m_directory->redoAction();
    redoAction->setShortcut(Qt::Key_Z | Qt::CTRL | Qt::SHIFT);

    m_editMenuActions.append(undoAction);
    m_editMenuActions.append(redoAction);


    QAction *viewModeAction = new QAction("Toggle View Mode", this);
    connect(viewModeAction, SIGNAL( triggered() ),
            this, SLOT( toggleViewMode() ) );
    m_viewMenuActions.append(viewModeAction);

    m_cascadeViewsAction = new QAction("Cascade Views", this);
    connect(m_cascadeViewsAction, SIGNAL( triggered() ),
            centralWidget(), SLOT( cascadeSubWindows() ) );
    m_viewMenuActions.append(m_cascadeViewsAction);

    m_tileViewsAction = new QAction("Tile Views", this);
    connect(m_tileViewsAction, SIGNAL( triggered() ),
            centralWidget(), SLOT( tileSubWindows() ) );
    m_viewMenuActions.append(m_tileViewsAction);

    QAction *detachActiveViewAction = new QAction("Detach Active View", this);
    connect(detachActiveViewAction, SIGNAL( triggered() ),
            this, SLOT( detachActiveView() ) );
    m_viewMenuActions.append(detachActiveViewAction);

    QAction *threadLimitAction = new QAction("Set Thread &Limit", this);
    connect(threadLimitAction, SIGNAL(triggered()),
            this, SLOT(configureThreadLimit()));

    m_settingsMenuActions.append(m_directory->project()->userPreferenceActions());
    m_settingsMenuActions.append(threadLimitAction);

    QAction *activateWhatsThisAct = new QAction("&What's This", this);
    activateWhatsThisAct->setShortcut(Qt::SHIFT | Qt::Key_F1);
    activateWhatsThisAct->setIcon(
        QPixmap(FileName("$base/icons/contexthelp.png").expanded()));
    activateWhatsThisAct->setToolTip("Activate What's This and click on parts "
        "this program to see more information about them");
    connect(activateWhatsThisAct, SIGNAL(triggered()), this, SLOT(enterWhatsThisMode()));

    m_helpMenuActions.append(activateWhatsThisAct);
  }


  /**
   * Creates the main menus of the menu bar.
   */
  void CNetSuiteMainWindow::createMenus() {
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->setObjectName("fileMenu");

    m_projectMenu = menuBar()->addMenu(tr("&Project"));
    m_projectMenu->setObjectName("projectMenu");

    m_editMenu = menuBar()->addMenu(tr("&Edit"));
    m_editMenu->setObjectName("editMenu");

    m_viewMenu = menuBar()->addMenu("&View");
    m_viewMenu->setObjectName("viewMenu");

    m_settingsMenu = menuBar()->addMenu("&Settings");
    m_settingsMenu->setObjectName("settingsMenu");

    m_helpMenu = menuBar()->addMenu("&Help");
    m_helpMenu->setObjectName("helpMenu");
  }


  /**
   * Write the window positioning and state information out to a
   * config file. This allows us to restore the settings when we
   * create another main window (the next time this program is run).
   * 
   * The state will be saved according to the currently loaded project and its name.
   *
   * When no project is loaded (i.e. the default "Project" is open), the config file used is
   * $HOME/.Isis/$APPNAME/$APPNAME_Project.config.
   * When a project, ProjectName, is loaded, the config file used is
   * $HOME/.Isis/$APPNAME/$APPNAME_ProjectName.config.
   * 
   * @param[in] project Pointer to the project that is currently loaded (default is "Project")
   *
   * @internal
   *   @history 2016-11-09 Ian Humphrey - Settings are now written according to the loaded project.
   *                           References #4358.
   */
  void CNetSuiteMainWindow::writeSettings(const Project *project) const {
    // Ensure that we are not using a NULL pointer   
    if (!project) { 
      QString msg = "Cannot write settings with a NULL Project pointer.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    QString appName = QApplication::applicationName();
    QSettings settings(
        FileName("$HOME/.Isis/" + appName + "/" + appName + "_" + project->name() + ".config")
          .expanded(),
        QSettings::NativeFormat);

    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("size", size());
    settings.setValue("pos", pos());

    settings.setValue("maxThreadCount", m_maxThreadCount);
  }


  /**
   * Read the window positioning and state information from the config file.
   *
   * When running cnetsuite without opening a project, the config file read is
   * $HOME/.Isis/$APPNAME/$APPNAME_Project.config
   * Otherwise, when running cnetsuite and opening a project (ProjectName), the config file read is
   * $HOME/.Isis/$APPNAME/$APPNAME_ProjectName.config
   *
   * @param[in] project (Project *) The project that was loaded.
   *
   * @internal
   *   @history Ian Humphrey - Settings are now read on a project name basis. References #4358.
   */
  void CNetSuiteMainWindow::readSettings(Project *project) {
    // Ensure that the Project pointer is not NULL
    if (!project) {
      QString msg = "Cannot read settings with a NULL Project pointer.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    if (project->name() == "Project") {
      setWindowTitle("cnetsuite");
    }
    else {
      setWindowTitle( project->name() );
    }
    QString appName = QApplication::applicationName();
    QSettings settings(
        FileName("$HOME/.Isis/" + appName + "/" + appName + "_" + project->name() + ".config")
          .expanded(),
        QSettings::NativeFormat);

    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

    // The geom/state isn't enough for main windows to correctly remember
    //   their position and size, so let's restore those on top of
    //   the geom and state.
    if (!settings.value("pos").toPoint().isNull())
      move(settings.value("pos").toPoint());

    resize(settings.value("size", QSize(800, 600)).toSize());
    m_maxThreadCount = settings.value("maxThreadCount", m_maxThreadCount).toInt();
    applyMaxThreadCount();
  }


  /**
   * Handle the close event by writing the window positioning and
   * state information before forwarding the event to the QMainWindow.
   */
  void CNetSuiteMainWindow::closeEvent(QCloseEvent *event) {
    writeSettings(m_directory->project());
    QMainWindow::closeEvent(event);
  }


  /**
   * Ask the user how many threads to use in this program. This
   * includes the GUI thread.
   */
  void CNetSuiteMainWindow::configureThreadLimit() {
    bool ok = false;

    QStringList options;

    int current = 0;
    options << tr("Use all available");

    for(int i = 1; i < 24; i++) {
      QString option = tr("Use %1 threads").arg(i + 1);

      options << option;
      if(m_maxThreadCount == i + 1)
        current = i;
    }

    QString res = QInputDialog::getItem(NULL, tr("Concurrency"),
        tr("Set the number of threads to use"),
        options, current, false, &ok);

    if (ok) {
      m_maxThreadCount = options.indexOf(res) + 1;

      if (m_maxThreadCount <= 1)
        m_maxThreadCount = -1;

      applyMaxThreadCount();
    }
  }


  /**
   * Activate the What's This? cursor. This is useful for he What's
   * This? action in the help menu.
   */
  void CNetSuiteMainWindow::enterWhatsThisMode() {
    QWhatsThis::enterWhatsThisMode();
  }


  /**
   * Slot to connect to the subWindowActivated signal from the central
   * QMdiArea. Updates the active view to the active sub window, or
   * sets it to null if the active window is not an AbstractProjectItemView.
   *
   * @param[in] window (QMdiSubWindow *) The active sub window.
   */
  void CNetSuiteMainWindow::onSubWindowActivated(QMdiSubWindow * window) {
    if (window) {
      setActiveView( qobject_cast<AbstractProjectItemView *>( window->widget() ) );
    }
    else {
      setActiveView(0);
    }
  }


  /**
   * Toggles the view mode of the central QMdiArea between tabbed and
   * sub window mode.
   */
  void CNetSuiteMainWindow::toggleViewMode() {
    QMdiArea *mdiArea = qobject_cast<QMdiArea *>( centralWidget() );
    if (mdiArea->viewMode() == QMdiArea::SubWindowView) {
      setTabbedViewMode();
    }
    else {
      setSubWindowViewMode();
    }
  }


  /**
   * Sets the QMdiArea in the central widget to the tabbed view mode
   * and updates the appropriate actions.
   */
  void CNetSuiteMainWindow::setTabbedViewMode() {
    QMdiArea *mdiArea = qobject_cast<QMdiArea *>( centralWidget() );
    mdiArea->setViewMode(QMdiArea::TabbedView);
    m_cascadeViewsAction->setEnabled(false);
    m_tileViewsAction->setEnabled(false);
  }


  /**
   * Sets the QMdiArea in the central widget to the sub window view
   * mode and updates the appropriate actions.
   */
  void CNetSuiteMainWindow::setSubWindowViewMode() {
    QMdiArea *mdiArea = qobject_cast<QMdiArea *>( centralWidget() );
    mdiArea->setViewMode(QMdiArea::SubWindowView);
    m_cascadeViewsAction->setEnabled(true);
    m_tileViewsAction->setEnabled(true);
  }


  /**
   * Moves the active view from the mdi area to its own independent
   * window. The view, its toolbars, and menu actions, are removed
   * from the main window and placed in an independent
   * QMainWindow. The new window contains the view as well as its
   * toolbars and menu actions. A detached view will not be set as the
   * active view when it is activated.
   */
  void CNetSuiteMainWindow::detachActiveView() {
    AbstractProjectItemView *view = m_activeView;

    if (!m_activeView) {
      return;
    }

    QMdiArea *mdiArea = qobject_cast<QMdiArea *>( centralWidget() );
    if (mdiArea) {
      mdiArea->removeSubWindow(view);
      mdiArea->closeActiveSubWindow();
    }

    QMainWindow *newWindow = new QMainWindow(this, Qt::Window);
    newWindow->setCentralWidget(view);
    newWindow->setWindowTitle( view->windowTitle() );

    if ( !view->permToolBarActions().isEmpty() ) {
      QToolBar *permToolBar = new QToolBar(newWindow);
      foreach ( QAction *action, view->permToolBarActions() ) {
        permToolBar->addAction(action);
      }
      newWindow->addToolBar(permToolBar);
    }

    if ( !view->activeToolBarActions().isEmpty() ) {
      QToolBar *activeToolBar = new QToolBar(newWindow);
      foreach ( QAction *action, view->activeToolBarActions() ) {
        activeToolBar->addAction(action);
      }
      newWindow->addToolBar(activeToolBar);
    }

    if ( !view->toolPadActions().isEmpty() ) {
      QToolBar *toolPad = new QToolBar(newWindow);
      foreach ( QAction *action, view->toolPadActions() ) {
        toolPad->addAction(action);
      }
      newWindow->addToolBar(Qt::RightToolBarArea, toolPad);
    }

    QMenuBar *menuBar = new QMenuBar(newWindow);
    newWindow->setMenuBar(menuBar);

    if ( !view->fileMenuActions().isEmpty() ) {
      QMenu *fileMenu = new QMenu("&File", newWindow);
      foreach ( QAction *action, view->fileMenuActions() ) {
        fileMenu->addAction(action);
      }
      menuBar->addMenu(fileMenu);
    }

    if ( !view->projectMenuActions().isEmpty() ) {
      QMenu *projectMenu = new QMenu("&Project", newWindow);
      foreach ( QAction *action, view->projectMenuActions() ) {
        projectMenu->addAction(action);
      }
      menuBar->addMenu(projectMenu);
    }

    if ( !view->editMenuActions().isEmpty() ) {
      QMenu *editMenu = new QMenu("&Edit", newWindow);
      foreach ( QAction *action, view->editMenuActions() ) {
        editMenu->addAction(action);
      }
      menuBar->addMenu(editMenu);
    }

    QAction *reattachAction = new QAction("Reattach View", newWindow);
    connect( reattachAction, SIGNAL( triggered() ),
             this, SLOT( reattachView() ) );

    QMenu *viewMenu = new QMenu("&View", newWindow);

    viewMenu->addAction(reattachAction);

    if ( !view->viewMenuActions().isEmpty() ) {
      foreach ( QAction *action, view->viewMenuActions() ) {
        viewMenu->addAction(action);
      }
    }
    menuBar->addMenu(viewMenu);

    if ( !view->settingsMenuActions().isEmpty() ) {
      QMenu *settingsMenu = new QMenu("&Settings", newWindow);
      foreach ( QAction *action, view->settingsMenuActions() ) {
        settingsMenu->addAction(action);
      }
      menuBar->addMenu(settingsMenu);
    }

    if ( !view->helpMenuActions().isEmpty() ) {
      QMenu *helpMenu = new QMenu("&Help", newWindow);
      foreach ( QAction *action, view->helpMenuActions() ) {
        helpMenu->addAction(action);
      }
      menuBar->addMenu(helpMenu);
    }
    newWindow->show();

  }


  /**
   * Reattaches a detached view. This slot can only be called by a QAction
   * from a QMainWindow that contains the view. The view is added to
   * the main window and the window that previously contained it is
   * deleted.
   */
  void CNetSuiteMainWindow::reattachView() {
    QAction *reattachAction = qobject_cast<QAction *>( sender() );
    if (!reattachAction) {
      return;
    }

    QMainWindow *viewWindow = qobject_cast<QMainWindow *>( reattachAction->parent() );
    if (!viewWindow) {
      return;
    }

    AbstractProjectItemView *view = qobject_cast<AbstractProjectItemView *>( viewWindow->centralWidget() );
    if (!view) {
      return;
    }

    view->setParent(this);
    viewWindow->deleteLater();

    addView(view);
  }

}
