#ifndef CNetSuiteMainWindow_H
#define CNetSuiteMainWindow_H
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

#include <QMainWindow>
#include <QPointer>
#include <QProgressBar>
#include <QMdiSubWindow>

namespace Isis {
  class AbstractProjectItemView;
  class Directory;
  class Project;

  /**
   * The main window for the cnetsuite appication. This handles most of the top-level GUI aspects of the program.
   *
   * @author 2012-??-?? Steven Lambright and Stuart Sides
   *
   * @internal
   *   @history 2012-07-27 Kimberly Oyama and Steven Lambright - Removed progress and warnings
   *                           tab widgets. They are now dock widgets.
   *   @history 2012-08-28 Tracie Sucharski - The Directory no longer takes a container it its
   *                           constructor.
   *   @history 2012-09-17 Steven Lambright - Dock widgets now delete themselves on close. This
   *                          gives the user the correct options when proceeding in the interface,
   *                          but undo/redo are not implemented (it needs to eventually be
   *                          encapsulated in a work order). The undo/redo didn't work correctly
   *                          anyways before setting this flag, so it's an improvement. Example
   *                          change: If I close Footprint View 1, I'm no longer asked if I want
   *                          to view images in footprint view 1.
   *   @history 2015-10-05 Jeffrey Covington - Replaced the ProjectTreeWidget
   *                          with a ProjectItemTreeView. Added the
   *                          eventFilter() method for intercepting some
   *                          events from views.
   *   @history 2016-01-04 Jeffrey Covington - Added a QMdiArea as the central widget
   *                          of the main window. The menus and toolbars are now solely
   *                          handled by the main window. Menus, context menus, and
   *                          toolbars are populated with actions recieved from the Directory
   *                          the active view, and the main window. Both WorkOrders and
   *                          regular QActions can be used in menus and toolbars. Views can
   *                          now be detached from the main window into their own independent
   *                          window with internalized menus and toolbars.
   *   @history 2016-10-20 Tracie Sucharski - Clean up included headers that are commented out,
   *                          updated for Qt5, comment call to saveState for window which caused
   *                          errors.  TODO:  Determine problem with saveState call.
   *  @history 2016-11-09  Tyler Wilson - Move a segment of code in the constructor from the beginning
   *                          to the end.  This code loads a project from the command line instead of the 
   *                          GUI, and it wasn't outputting warnings/errors to the warnings/error tab
   *                          when the project was loaded because it was being called before the GUI
   *                          was created.  Fixes #4488.  References #4526, ##4487.
   *   @history 2016-11-09 Ian Humphrey - Modified readSettings() and writeSettings() to take in
   *                           Project pointers to be used to properly read and write settings
   *                           for the CNetSuiteMainWindow. Note that when running cnetsuite without
   *                           opening a Project, the config file cnetsuite_Project.config is used.
   *                           Otherwise, when a project is open, the config file
   *                           cnetsuite_ProjectName will be used to restore window geom.
   *                           The m_permToolBar, m_activeToolBar, and m_toolPad now have object
   *                           names set, so the saveState() call within writeSettings() now works.
   *                           Fixes #4358.
   */
  class CNetSuiteMainWindow : public QMainWindow {
      Q_OBJECT
    public:
      explicit CNetSuiteMainWindow(QWidget *parent = 0);
      ~CNetSuiteMainWindow();

    public slots:
      void addView(QWidget *newWidget);
      void setActiveView(AbstractProjectItemView *view);
      void updateMenuActions();
      void updateToolBarActions();
      void readSettings(Project *);

    protected:
      void closeEvent(QCloseEvent *event);
      bool eventFilter(QObject *watched, QEvent *event);

    private slots:
      void configureThreadLimit();
      void enterWhatsThisMode();
      void onSubWindowActivated(QMdiSubWindow *);

      void toggleViewMode();
      void setTabbedViewMode();
      void setSubWindowViewMode();

      void detachActiveView();
      void reattachView();

    private:
      Q_DISABLE_COPY(CNetSuiteMainWindow);

      void applyMaxThreadCount();
      void createMenus();
      void writeSettings(const Project *project) const;

      void initializeActions();

    private:
      /**
       * The directory stores all of the work orders that this program is capable of doing. This
       *   drives most of the functionality.
       */
      QPointer<Directory> m_directory;

      QDockWidget *m_projectDock;

      /**
       * This is the "goal" or "estimated" maximum number of active threads running in this program
       *   at once. For now, the GUI consumes 1 thread and QtConcurrent
       *   (QThreadPool::globalInstance) consumes the remaining threads. Anything <= 1 means that we
       *   should perform a best-guess for best perfomance.
       */
      int m_maxThreadCount;

      QToolBar *m_permToolBar; //!< The toolbar for actions that rarely need to be changed.
      QToolBar *m_activeToolBar; //<! The toolbar for the actions of the current tool.
      QToolBar *m_toolPad; //<! The toolbar for the actions that activate tools.

      QMenu *m_fileMenu; //!< Menu for the file actions
      QMenu *m_projectMenu; //!< Menu for the project actions
      QMenu *m_editMenu; //!< Menu for edit actions
      QMenu *m_viewMenu; //!< Menu for view and window actions
      QMenu *m_settingsMenu; //!< Menu for settings actions
      QMenu *m_helpMenu; //!< Menu for help actions

      QList<QAction *> m_fileMenuActions; //!< Internal list of file actions
      QList<QAction *> m_projectMenuActions;//!< Internal list of project actions
      QList<QAction *> m_editMenuActions;//!< Internal list of edit actions
      QList<QAction *> m_viewMenuActions;//!< Internal list of view actions
      QList<QAction *> m_settingsMenuActions;//!< Internal list of settings actions
      QList<QAction *> m_helpMenuActions;//!< Internal list of help actions

      QList<QAction *> m_permToolBarActions;//!< Internal list of permanent toolbar actions
      QList<QAction *> m_activeToolBarActions;//!< Internal list of active toolbar actions
      QList<QAction *> m_toolPadActions;//!< Internal list of toolpad actions

      QAction *m_cascadeViewsAction; //!< Action that cascades the mdi area
      QAction *m_tileViewsAction; //!< Action that tiles the mdi area

      AbstractProjectItemView *m_activeView; //!< The active view
  };
}

#endif // CNetSuiteMainWindow_H
