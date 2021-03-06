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
#include "ImportShapesWorkOrder.h"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrentMap>

#include "Cube.h"
#include "CubeAttribute.h"
#include "FileName.h"
#include "Project.h"
#include "SaveProjectWorkOrder.h"
#include "TextFile.h"

namespace Isis {

  ImportShapesWorkOrder::ImportShapesWorkOrder(Project *project) :
      WorkOrder(project) {
    m_newShapes = NULL;

    QAction::setText(tr("Import &Shape Models..."));
    QUndoCommand::setText(tr("Import Shape Models"));
    setModifiesDiskState(true);
  }


  ImportShapesWorkOrder::ImportShapesWorkOrder(const ImportShapesWorkOrder &other) :
      WorkOrder(other) {
    m_newShapes = NULL;
  }


  ImportShapesWorkOrder::~ImportShapesWorkOrder() {
    delete m_newShapes;
    m_newShapes = NULL;
  }


  ImportShapesWorkOrder *ImportShapesWorkOrder::clone() const {
    return new ImportShapesWorkOrder(*this);
  }


  bool ImportShapesWorkOrder::execute() {
    WorkOrder::execute();

    QStringList fileNames = QFileDialog::getOpenFileNames(
        qobject_cast<QWidget *>(parent()),
        tr("Import Shape Model Images"), "",
        tr("Isis cubes and list files (*.cub *.lis);;All Files (*)"));

    QStringList stateToSave;

    if (!fileNames.isEmpty()) {
      foreach (FileName fileName, fileNames) {
        if (fileName.extension() == "lis") {
          TextFile listFile(fileName.expanded());
          QString lineOfListFile;

          while (listFile.GetLine(lineOfListFile)) {
            stateToSave.append(lineOfListFile);
          }
        }
        else {
          stateToSave.append(fileName.original());
        }
      }
    }

    QMessageBox::StandardButton copyImagesAnswer = QMessageBox::No;
    if (!fileNames.isEmpty()) {
      copyImagesAnswer = QMessageBox::question(qobject_cast<QWidget *>(parent()),
               tr("Copy Shape Model Cubes into Project"),
               tr("Should images (DN data) be copied into project?"),
               QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
               QMessageBox::Yes);
    }

    bool copyDnData = (copyImagesAnswer == QMessageBox::Yes);

    stateToSave.prepend(copyDnData? "copy" : "nocopy");

    if (fileNames.count() > 1) {
      QUndoCommand::setText(tr("Import %1 Shape Model Images").arg(stateToSave.count() - 1));
    }
    else if (fileNames.count() == 1) {
      QUndoCommand::setText(tr("Import %1").arg(fileNames.first()));
    }

    setInternalData(stateToSave);

    bool doImport = fileNames.count() > 0 && copyImagesAnswer != QMessageBox::Cancel;

    return doImport;
  }



  void ImportShapesWorkOrder::asyncUndo() {
    project()->waitForShapeReaderFinished();
    project()->shapes().last()->deleteFromDisk(project());
  }


  void ImportShapesWorkOrder::postSyncUndo() {
    QPointer<ShapeList> shapesWeAdded = project()->shapes().last();

    foreach (Shape *shape, *shapesWeAdded) {
      delete shape;
    }
    delete shapesWeAdded;
  }


  void ImportShapesWorkOrder::asyncRedo() {
    if (internalData().count() > 0) {
      importConfirmedShapes(internalData().mid(1), (internalData()[0] == "copy"));
    }
  }


  void ImportShapesWorkOrder::postSyncRedo() {
    if (!m_newShapes->isEmpty()) {
      project()->addShapes(*m_newShapes);

      delete m_newShapes;
      m_newShapes = NULL;
    }

    if (m_warning != "") {
      project()->warn(m_warning);
    }
  }


  ImportShapesWorkOrder::OriginalFileToProjectCubeFunctor::OriginalFileToProjectCubeFunctor(
      QThread *guiThread, QDir destinationFolder, bool copyDnData) : m_errors(new IException),
      m_numErrors(new int(0)) {
    m_destinationFolder = destinationFolder;
    m_copyDnData = copyDnData;
    m_guiThread = guiThread;
  }


  ImportShapesWorkOrder::OriginalFileToProjectCubeFunctor::OriginalFileToProjectCubeFunctor(
      const OriginalFileToProjectCubeFunctor &other) : m_errors(other.m_errors),
      m_numErrors(other.m_numErrors) {
    m_destinationFolder = other.m_destinationFolder;
    m_copyDnData = other.m_copyDnData;
    m_guiThread = other.m_guiThread;
  }


  ImportShapesWorkOrder::OriginalFileToProjectCubeFunctor::~OriginalFileToProjectCubeFunctor() {
    m_destinationFolder = QDir();
    m_copyDnData = false;
    m_guiThread = NULL;
  }


  Cube *ImportShapesWorkOrder::OriginalFileToProjectCubeFunctor::operator()(
      const FileName &original) {
    Cube *result = NULL;

    if (*m_numErrors < 20) {
      try {
        QString destination = QFileInfo(m_destinationFolder, original.name())
                                .absoluteFilePath();
        Cube *input = new Cube(original, "r");

        if (m_copyDnData) {
          Cube *copiedCube = input->copy(destination, CubeAttributeOutput());
          delete input;
          input = copiedCube;
        }

        FileName externalLabelFile(destination);
        externalLabelFile = externalLabelFile.setExtension("ecub");

        Cube *projectShape = input->copy(externalLabelFile, CubeAttributeOutput("+External"));

        if (m_copyDnData) {
          // Make sure the external label has a fully relative path to the DN data
          projectShape->relocateDnData(FileName(destination).name());
        }

        delete input;

        result = projectShape;
      }
      catch (IException &e) {
        m_errorsLock.lock();

        m_errors->append(e);
        (*m_numErrors)++;

        m_errorsLock.unlock();
      }
    }

    return result;
  }


  IException ImportShapesWorkOrder::OriginalFileToProjectCubeFunctor::errors() const {
    IException result;

    result.append(*m_errors);

    if (*m_numErrors >= 20) {
      result.append(
          IException(IException::Unknown,
                     tr("Aborted import shapes due to a high number of errors"),
                     _FILEINFO_));
    }
    return result;
  }


  /**
   * Creates a project shape folder and copies the shape cubes into it. This will create the *.ecub 
   * and .cub files inside of the project. 
   * This can be thought of as:
   * <pre>
   *   mkdir project/Shapes/import1
   *   cp in1.cub in2.cub project/Shapes/import1
   * </pre>
   *
   * This should be called in a non-GUI thread
   *
   * @param confirmedShapes This is a list of shape model cube file names outside of the
   *                   project folder
   * @param copyDnData If this is true, this will create both the *.cub and *.ecub files in the
   *                   project. Otherwise, only the external label files (*.ecub) will be created
   *                   inside of the project.
   */
  void ImportShapesWorkOrder::importConfirmedShapes(QStringList confirmedShapes,
                                                             bool copyDnData) {
    if (!confirmedShapes.isEmpty()) {
      QDir folder = project()->addShapeFolder("import");

      setProgressRange(0, confirmedShapes.count());

      m_newShapes = new ShapeList;
      m_newShapes->reserve(confirmedShapes.count());

      QStringList confirmedShapesFileNames;
      QStringList confirmedShapesIds;

      foreach (QString confirmedShape, confirmedShapes) {
        QStringList fileNameAndId = confirmedShape.split(",");

        confirmedShapesFileNames.append(fileNameAndId.first());

        if (fileNameAndId.count() == 2) {
          confirmedShapesIds.append(fileNameAndId.last());
        }
        else {
          confirmedShapesIds.append(QString());
        }
      }

      OriginalFileToProjectCubeFunctor functor(thread(), folder, copyDnData);
      QFuture<Cube *> future = QtConcurrent::mapped(confirmedShapesFileNames, functor);

      QStringList newInternalData;
      newInternalData.append(internalData().first());

      QThreadPool::globalInstance()->releaseThread();
      for (int i = 0; i < confirmedShapes.count(); i++) {
        setProgressValue(i);

        Cube *cube = future.resultAt(i);

        if (cube) {
          Shape *newShape = new Shape(future.resultAt(i));

          if (confirmedShapesIds[i].isEmpty()) {
            confirmedShapesIds[i] = newShape->id();
          }
          else {
            newShape->setId(confirmedShapesIds[i]);
          }

          QStringList ShapeInternalData;
          ShapeInternalData.append(confirmedShapesFileNames[i]);
          ShapeInternalData.append(confirmedShapesIds[i]);

          newInternalData.append(ShapeInternalData.join(","));

          m_newShapes->append(newShape);

          newShape->moveToThread(thread());
          newShape->displayProperties()->moveToThread(thread());

          newShape->closeCube();
        }
      }
      QThreadPool::globalInstance()->reserveThread();

      m_warning = functor.errors().toString();

      m_newShapes->moveToThread(thread());

      setInternalData(newInternalData);
    }
  }
}
