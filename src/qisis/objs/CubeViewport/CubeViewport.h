#ifndef CubeViewport_h
#define CubeViewport_h

/**
 * @file
 * $Date: 2010/06/30 03:38:12 $
 * $Revision: 1.30 $
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are public
 *   domain. See individual third-party library and package descriptions for
 *   intellectual property information,user agreements, and related information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or implied,
 *   is made by the USGS as to the accuracy and functioning of such software
 *   and related material nor shall the fact of distribution constitute any such
 *   warranty, and no responsibility is assumed by the USGS in connection
 *   therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html in a browser or see
 *   the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */


// parent of this class
#include <QAbstractScrollArea>


class QPaintEvent;

namespace Isis {
  class Brick;
  class Camera;
  class Cube;
  class CubeDataThread;
  class Projection;
  class Pvl;
  class PvlKeyword;
  class Stretch;
  class Tool;
  class UniversalGroundMap;

  class ViewportBuffer;

  /**
   * @brief Widget to display Isis cubes for qt apps
   *
   * @ingroup Visualization Tools
   *
   * @author ????-??-?? Jeff Anderson
   *
   * @internal
   *  @history 2007-01-30  Tracie Sucharski,  Add information
   *                           message if spice not found.
   *  @history 2007-02-07  Tracie Sucharski,  Remove error message, decided it
   *                           was more of a hassle to click ok when displaying
   *                           many images without spice.
   *  @history 2007-03-20  Tracie Sucharski,  Add fitScaleMinDimension,
   *                           fitScaleWidth and fitScaleHeight methods.  Change
   *                           cursor to wait cursor in paintPixmap.
   *  @history 2007-04-13  Tracie Sucharski, Remove fitScaleMinDimension, turns
   *                           out it was not needed.
   *  @history 2007-09-11  Steven Lambright, Added double click signal
   *  @history 2007-12-11  Steven Lambright, Added 1x1xn cube auto stretch support
   *  @history 2008-06-19  Noah Hilt, Added a close event for saving and discarding
   *                           changes and a method to set the cube.
   *  @history 2008-12-04  Jeannie Walldren, Fixed a bug in computeStretch() method
   *                           for comparing precision difference on 32 bit Linux
   *                           system. Added try/catch in showCube() to set
   *                           p_cubeShown = false if this fails.
   *  @history 2009-03-09  Steven Lambright - Changed the way we do floating point
   *                           math in autoStretch to work better in order to allow
   *                           more cubes to open.
   *  @history 2009-03-27 Noah Hilt/Steven Lambright - Removed old rubber band
   *                          methods and variables. Added new ViewportBuffer to
   *                          read and store visible dn values in the viewport.
   *  @history 2009-10-23 Steven Lambright - Camera::SetBand is now called when
   *                          switching the band being shown.
   *  @history 2010-04-08 Steven Lambright and Eric Hyer -
   *                          Now supports ViewportBuffer using threaded cube I/O
   *  @history 2010-04-30 Steven Lambright - Bug fixes and better
   *                          support for threaded cube I/O
   *  @history 2010-05-20 Steven Lambright - Added memory of global
   *                          stretches
   *  @history 2010-05-24 Eric Hyer - Fixed bug where QPainter construction was
   *                          being attempted with a potentially null pixmap
   *  @history 2010-06-26 Eric Hyer - Moved MDI specific code to new child class
   *                          called MdiCubeViewport.  Fixed many include mistakes.
   *  @history 2010-07-12 Jeannie Walldren - Changed setScale() method's maximum
   *                          value from hard-coded 16 to the max of the viewport
   *                          width and height.  Added setScale() minimum value
   *                          of 1/min(samps,lines).  Added exceptions to
   *                          ViewportBuffer to help track errors.
   *  @history 2010-11-08 Eric Hyer -  Added better mouseMove signal
   *  @history 2010-12-01 Steven Lambright - The initial scale now uses fitScale
   *                          so that it is consistent with the zoom tool.
   *  @history 2011-03-30 Sharmila Prasad - Set the frame shadow and style to remove
   *                          border around the image
   *  @history 2011-03-31 Sharmila Prasad - Added band info to "whatsthis"
   *                          API to store the whatsthis info in a PVL format
   *  @history 2011-04-25 Steven Lambright - Fixed "center" and added more safety
   *                          checks.
   *  @history 2012-03-22 Steven Lambright and Jai Rideout - Fixed bug where
   *                          screenPixelsChanged was not correctly emitted
   *                          on resize.
   *  @history 2012-05-29 Steven Lambright - Changed destructor to clean up the cube when
   *                          necessary.
   *  @history 2012-06-28 Steven Lambright - Stretching gray no longer removes stretch
   *                          special pixel values from the RGB stretches. References #684.
   *  @history 2012-07-27 Tracie Sucharski - Added viewportClosed signal so that tools
   *                          can respond to the user closing a viewport rather than when the
   *                          application exits.
   *  @history 2013-11-04 Janet Barrett - Added the p_comboCount and p_comboIndex variables to
   *                          store the current state of the band bin combo box. Also added
   *                          comboCount(), comboIndex(), setComboCount(), and setComboIndex()
   *                          accessors and setters to allow the BandTool class to retain the
   *                          settings for each cube viewport. Fixes #1612.
   */
  class CubeViewport : public QAbstractScrollArea {
      Q_OBJECT

    public:
      CubeViewport(Cube *cube, CubeDataThread * cdt = 0, QWidget *parent = 0);
      virtual ~CubeViewport();


      /**
       * @author ????-??-?? Unknown
       *
       * @internal
       */
      class BandInfo {
        public:
          BandInfo();
          BandInfo(const BandInfo &other);
          ~BandInfo();
          const BandInfo &operator=(BandInfo other);
          Stretch getStretch() const;
          void setStretch(const Stretch &newStretch);
          int band;
        private:
          Stretch *stretch;
      };

      void setCube(Cube *cube);
      int cubeSamples() const;
      int cubeLines() const;
      int cubeBands() const;

      //! Is the viewport shown in 3-band color
      bool isColor() const {
        return p_color;
      };

      //! Is the viewport shown in gray / b&w
      bool isGray() const {
        return !p_color;
      };

      //! Return the gray band currently viewed
      int grayBand() const {
        return p_gray.band;
      };

      //! Return the red band currently viewed
      int redBand() const {
        return p_red.band;
      };

      //! Return the green band currently viewed
      int greenBand() const {
        return p_green.band;
      };

      //! Return the blue band currently viewed
      int blueBand() const {
        return p_blue.band;
      };

      //! Return the scale
      double scale() const {
        return p_scale;
      };

      //! Return if the cube is visible
      bool cubeShown() const {
        return p_cubeShown;
      };

      //! Return the BandBin combo box count
      int comboCount() const {
        return p_comboCount;
      };

      //! Return the BandBin combo box index
      int comboIndex() const {
        return p_comboIndex;
      }

      void cubeContentsChanged(QRect rect);

      double fitScale() const;
      double fitScaleWidth() const;
      double fitScaleHeight() const;

      void viewportToCube(int x, int y,
                          double &sample, double &line) const;
      void cubeToViewport(double sample, double line,
                          int &x, int &y) const;
      void contentsToCube(int x, int y,
                          double &sample, double &line) const;
      void cubeToContents(double sample, double line,
                          int &x, int &y) const;

      double redPixel(int sample, int line);
      double greenPixel(int sample, int line);
      double bluePixel(int sample, int line);
      double grayPixel(int sample, int line);

      Stretch grayStretch() const;
      Stretch redStretch() const;
      Stretch greenStretch() const;
      Stretch blueStretch() const;

      //! Return the cube associated with viewport
      Cube *cube() const {
        return p_cube;
      };

      //! Return the projection associated with cube (NULL implies none)
      Projection *projection() const {
        return p_projection;
      };

      //! Return the camera associated with the cube (NULL implies none)
      Camera *camera() const {
        return p_camera;
      };

      //! Return the universal ground map associated with the cube (NULL implies none)
      UniversalGroundMap *universalGroundMap() const {
        return p_groundMap;
      };

      void moveCursor(int x, int y);
      bool cursorInside() const;
      QPoint cursorPosition() const;
      void setCursorPosition(int x, int y);
      void setCaption();

      /**
       * Sets the background color
       *
       * @param color
       */
      void setBackground(QColor color) {
        p_bgColor = color;
      }

      /**
       * Sets the band bin combo box count
       *
       * @param count
       */
      void setComboCount(int count) {
        p_comboCount = count;
      }

      /**
       * Sets the band bin combo box index
       *
       * @param index
       */
      void setComboIndex(int index) {
        p_comboIndex = index;
      }

      /**
       * Returns the pixmap
       *
       *
       * @return QPixmap
       */
      QPixmap pixmap() {
        return p_pixmap;
      }

      /**
       * Returns the gray viewport buffer (Will be NULL if in RGB mode.)
       *
       *
       * @return ViewportBuffer*
       */
      ViewportBuffer *grayBuffer() {
        return p_grayBuffer;
      }

      /**
       * Returns the red viewport buffer (Will be NULL if in Gray mode.)
       *
       *
       * @return ViewportBuffer*
       */
      ViewportBuffer *redBuffer() {
        return p_redBuffer;
      }

      /**
       * Returns the green viewport buffer (Will be NULL if in Gray mode.)
       *
       *
       * @return ViewportBuffer*
       */
      ViewportBuffer *greenBuffer() {
        return p_greenBuffer;
      }

      /**
       * Returns the blue viewport buffer (Will be NULL if in Gray mode.)
       *
       *
       * @return ViewportBuffer*
       */
      ViewportBuffer *blueBuffer() {
        return p_blueBuffer;
      }

      void bufferUpdated(QRect rect);

      /**
       * This is called by internal viewport buffers when a stretch
       * action should be performed. The buffer passes itself as the
       * argument.
       *
       * @param buffer
       */
      virtual void restretch(ViewportBuffer *buffer) = 0;
      void paintPixmap();

      /**
       * Resets all remembered stretches
       */
      void forgetStretches();

      /**
       * Sets a stretch for all bands
       *
       * @param stretch
       */
      void setAllBandStretches(Stretch stretch);


      /**
       * @returns this CubeViewport's CubeDataThread
       */
      CubeDataThread *cubeDataThread() {
        return p_cubeData;
      }

      /**
       * @returns the CubeViewport's cube id
       */
      int cubeID() {
        return p_cubeId;
      }


      /**
       * Get All WhatsThis info - viewport, cube, area in PVL format
       *
       * @param pWhatsThisPvl - Pvl for all whatsthis info
       */
      void getAllWhatsThisInfo(Pvl & pWhatsThisPvl);

      /**
       * Get Band Filter name from the Isis cube label
       *
       * @param pFilterNameKey - FilterName keyword containing the
       *              corresponding keyword from the Isis Cube label
       */
      void getBandFilterName(PvlKeyword & pFilterNameKey);

      /**
       * Get Cube area corresponding to the viewport's dimension
       *
       * @param pdStartSample - Cube Start Sample
       * @param pdEndSample   - Cube End Sample
       * @param pdStartLine   - Cube Start Line
       * @param pdEndLine     - Cube End Line
       */
      void getCubeArea(double & pdStartSample, double & pdEndSample,
                                     double & pdStartLine, double & pdEndLine);

      bool confirmClose();

    signals:
      void viewportUpdated();//!< Emitted when viewport updated.
      void viewportClosed(CubeViewport *);//!< Emitted when viewport is closed.
      void mouseEnter();//!< Emitted when the mouse enters the viewport
      void mouseMove(QPoint);//!< Emitted when the mouse moves.
      void mouseMove(QPoint, Qt::MouseButton);//!< Emitted when the mouse moves.
      void mouseLeave();//!< Emitted when the mouse leaves the viewport.
      void mouseButtonPress(QPoint, Qt::MouseButton);//!< Emitted when mouse button pressed
      void mouseButtonRelease(QPoint, Qt::MouseButton);//!< Emitted when mouse button released
      void mouseDoubleClick(QPoint);//!< Emitted when double click happens
      void windowTitleChanged();//!< Emitted when window title changes
      void scaleChanged(); //!< Emitted when zoom factor changed just before the repaint event
      void saveChanges(CubeViewport *); //!< Emitted when changes should be saved
      void discardChanges(CubeViewport *); //!< Emitted when changes should be discarded
      void screenPixelsChanged(); //!< Emitted when cube pixels that should be on the screen change

      /**
       * Emitted with current progress (0 to 100) when working
       */
      void progressChanged(int);

      /**
       * Emitted when a brick is no longer needed, should only be sent
       * to cube data thread
       */
      void doneWithData(int, const Isis::Brick *);


    public slots:
      QSize sizeHint() const;
      void setScale(double scale);
      void setScale(double scale, double sample, double line);
      void setScale(double scale, int x, int y);
      void center(int x, int y);
      void center(double sample, double line);

      virtual void viewRGB(int redBand, int greenBand, int blueBand);
      virtual void viewGray(int band);

      void stretchGray(const QString &string);
      void stretchRed(const QString &string);
      void stretchGreen(const QString &string);
      void stretchBlue(const QString &string);

      void stretchGray(const Stretch &stretch);
      void stretchRed(const Stretch &stretch);
      void stretchGreen(const Stretch &stretch);
      void stretchBlue(const Stretch &stretch);

      void stretchKnownGlobal();

      void cubeChanged(bool changed);
      void showEvent(QShowEvent *);

      void scrollBy(int dx, int dy);

      void changeCursor(QCursor cursor);

      void onProgressTimer();
      void enableProgress();


    protected:
      void scrollContentsBy(int dx, int dy);
      virtual void resizeEvent(QResizeEvent *e);
      virtual bool eventFilter(QObject *o, QEvent *e);
      virtual void keyPressEvent(QKeyEvent *e);
      virtual void paintEvent(QPaintEvent *e);



    protected slots:
      virtual void cubeDataChanged(int cubeId, const Isis::Brick *);


    private:

      void paintPixmap(QRect rect);
      void shiftPixmap(int dx, int dy);

      void updateScrollBars(int x, int y);
      void paintPixmapRects();

      //void computeStretch(Brick *brick, int band,
      //                    int ssamp, int esamp,
      //                    int sline, int eline, int linerate,
      //                    Stretch &stretch);



    protected: // data
      QPixmap p_pixmap;//!< The qpixmap.

      //! Stretches for each previously stretched band
      QVector< Stretch * > * p_knownStretches;

      //! Global stretches for each stretched band
      QVector< Stretch * > * p_globalStretches;


    private: // data
      ViewportBuffer *p_grayBuffer;  //!< Viewport Buffer to manage gray band
      ViewportBuffer *p_redBuffer;  //!< Viewport Buffer to manage red band
      ViewportBuffer *p_greenBuffer;  //!< Viewport Buffer to manage green band
      ViewportBuffer *p_blueBuffer;  //!< Viewport Buffer to manage blue band

      QColor p_bgColor; //!< The color to paint the background of the viewport

      Cube *p_cube;  //!< The cube associated with the viewport.
      Camera *p_camera;  //!< The camera from the cube.
      Projection *p_projection;  //!< The projection from the cube.
      UniversalGroundMap *p_groundMap;  //!< The universal ground map from the cube.

      //! Activated to update progress bar
      QTimer *p_progressTimer;

      double p_scale;//!< The scale number.

      bool p_color;//!< Is the viewport in color?
      BandInfo p_gray;//!< Gray band info
      BandInfo p_red;//!< Red band info
      BandInfo p_green;//!< Green band info
      BandInfo p_blue;//!< Blue band info

      int p_comboCount;//!< Number of elements in band bin combo box
      int p_comboIndex;//!< Current element chosen from combo box

      Brick *p_redBrick;  //!< Bricks for every color.
      Brick *p_grnBrick;  //!< Bricks for every color.
      Brick *p_bluBrick;  //!< Bricks for every color.
      Brick *p_gryBrick;  //!< Bricks for every color.
      Brick *p_pntBrick;  //!< Bricks for every color.
      bool p_saveEnabled; //!< Has the cube changed?
      bool p_cubeShown;//!< Is the cube visible?
      QImage *p_image;  //!< The qimage.
      bool p_paintPixmap;//!< Paint the pixmap?
      bool p_updatingBuffers; //!< Changing RGB and need to not repaint pixmap?

      QString p_whatsThisText;//!< The text for What's this.
      QString p_cubeWhatsThisText;//!< The text for the cube's What's this.
      QString p_viewportWhatsThisText;//!< The text for the viewport's what's this.
      void updateWhatsThis();

      //! A list of rects that the viewport buffers have requested painted
      QList< QRect * > *p_pixmapPaintRects;

      CubeDataThread *p_cubeData;  //!< Does all the cube I/O
      int p_cubeId; //!< Cube ID given from cube data thread for I/O

      /**
       *  if true then this owns the CubeDataThread,
       *  and should thus delete it
       */
      bool p_thisOwnsCubeData;
  };
}

#endif
