#include "Isis.h"
#include "ProcessImportPds.h"

#include "UserInterface.h"
#include "FileName.h"

#include <QFile>

using namespace std;
using namespace Isis;

void IsisMain() {
  ProcessImportPds p, ptmp;
  Pvl label;
  UserInterface &ui = Application::GetUserInterface();

  QString labelFile = ui.GetFileName("FROM");
  QString imageFile("");
  if(ui.WasEntered("IMAGE")) {
    imageFile = ui.GetFileName("IMAGE");
  }

  // The Kaguya MI MAP files have an incorrect SAMPLE_PROJECTION_OFFSET
  // keyword value in their labels. The following code creates a temporary
  // detached PDS label with the correct (negated) keyword value.
  ptmp.SetPdsFile(labelFile, imageFile, label);
  PvlObject obj = label.findObject("IMAGE_MAP_PROJECTION");
  double soff = toDouble(obj.findKeyword("SAMPLE_PROJECTION_OFFSET")[0]);
  soff = -soff;
  label.findObject("IMAGE_MAP_PROJECTION").addKeyword(PvlKeyword("SAMPLE_PROJECTION_OFFSET",toString(soff)),Pvl::Replace);
  FileName tempFileName = FileName::createTempFile("TEMPORARYlabel.pvl").name();
  QString fn(tempFileName.expanded());
  label.write(fn);
  p.SetPdsFile(fn, labelFile, label);
  QFile::remove(fn);

  Cube *ocube = p.SetOutputCube("TO");

  // Get user entered special pixel ranges
  if(ui.GetBoolean("SETNULLRANGE")) {
    p.SetNull(ui.GetDouble("NULLMIN"), ui.GetDouble("NULLMAX"));
  }
  if(ui.GetBoolean("SETHRSRANGE")) {
    p.SetHRS(ui.GetDouble("HRSMIN"), ui.GetDouble("HRSMAX"));
  }
  if(ui.GetBoolean("SETHISRANGE")) {
    p.SetHIS(ui.GetDouble("HISMIN"), ui.GetDouble("HISMAX"));
  }
  if(ui.GetBoolean("SETLRSRANGE")) {
    p.SetLRS(ui.GetDouble("LRSMIN"), ui.GetDouble("LRSMAX"));
  }
  if(ui.GetBoolean("SETLISRANGE")) {
    p.SetLIS(ui.GetDouble("LISMIN"), ui.GetDouble("LISMAX"));
  }

  // Export the cube
  p.StartProcess();

  // Translate the mapping labels 
  Pvl otherLabels;
  p.TranslatePdsProjection(otherLabels);

  // Translate the remaining MI MAP labels
  PvlGroup dataDir(Preference::Preferences().findGroup("DataDirectory"));
  QString transDir = (QString) dataDir["Kaguya"] + "/translations/";

  FileName transFile(transDir + "mimapBandBin.trn");
  PvlTranslationManager bandBinXlater(label, transFile.expanded());
  bandBinXlater.Auto(otherLabels);

  transFile = transDir + "mimapInstrument.trn";
  PvlTranslationManager instXlater(label, transFile.expanded());
  instXlater.Auto(otherLabels);

  transFile = transDir + "mimapArchive.trn";
  PvlTranslationManager archiveXlater(label, transFile.expanded());
  archiveXlater.Auto(otherLabels);

  if(otherLabels.hasGroup("Mapping") &&
      (otherLabels.findGroup("Mapping").keywords() > 0)) {
    ocube->putGroup(otherLabels.findGroup("Mapping"));
  }
  if(otherLabels.hasGroup("Instrument") &&
      (otherLabels.findGroup("Instrument").keywords() > 0)) {
    ocube->putGroup(otherLabels.findGroup("Instrument"));
  }
  if(otherLabels.hasGroup("BandBin") &&
      (otherLabels.findGroup("BandBin").keywords() > 0)) {
    ocube->putGroup(otherLabels.findGroup("BandBin"));
  }
  if(otherLabels.hasGroup("Archive") &&
      (otherLabels.findGroup("Archive").keywords() > 0)) {
    ocube->putGroup(otherLabels.findGroup("Archive"));
  }

  p.EndProcess();

  return;
}

