#include "TargetInfoWidget.h"
#include "ui_TargetInfoWidget.h"

#include <QPixmap>

#include "Directory.h"
#include "SpiceRotation.h"
#include "TargetBody.h"
#include "TargetBodyDisplayProperties.h"

namespace Isis {

  /**
   * Constructor.  Sets up the widget based on the target.
   * 
   * @param target The target whose information will be displayed
   * @param directory Currently unused
   * @param parent The parent widget
   */
  TargetInfoWidget::TargetInfoWidget(TargetBody* target, Directory *directory,
                                     QWidget *parent) : m_ui(new Ui::TargetInfoWidget) {
    m_ui->setupUi(this);

    m_target = target;

    QString name = m_target->displayProperties()->displayName();


    // Ken TODO - set up map between target display names and icon/image names
    QPixmap image;
    if (name.compare("MOON") == 0) {
      image.load(":moon-large");
      setWindowIcon(QIcon(":moon"));
    }
    else if (name.compare("Enceladus") == 0) {
      image.load(":enceladus-saturn");
      setWindowIcon(QIcon(":enceladus"));
    }
    else if (name.compare("Europa") == 0) {
      image.load(":europa-large");
      setWindowIcon(QIcon(":europa"));
    }
    else if (name.compare("Mars") == 0) {
      image.load(":mars-large");
      setWindowIcon(QIcon(":mars"));
    }
    else if (name.compare("Titan") == 0) {
      image.load(":titan-large");
      setWindowIcon(QIcon(":titan"));
    }

    m_ui->bodySystemlabel->setText(tr("System: %1").arg(m_target->naifPlanetSystemName()));

    setMinimumWidth(m_ui->tabWidget->minimumWidth()+20);

    m_ui->targetImage->setPixmap(image);

    setFrameStyle(QFrame::Panel | QFrame::Sunken);
    setLineWidth(2);

    m_ui->tabWidget->setCurrentIndex(0);

    if (target->frameType() == Isis::SpiceRotation::BPC) {
      m_ui->poleRightAscensionLabel->hide();
      m_ui->poleDeclinationLabel->hide();
      m_ui->polePMOffsetLabel->hide();
    }
    else {
      m_ui->poleRightAscensionLabel->setText(formatPoleRaString());
      m_ui->poleDeclinationLabel->setText(formatPoleDecString());
      m_ui->polePMOffsetLabel->setText(formatPmString());
    }

    m_ui->aRadiiLabel->setText(tr("%1").arg(m_target->radiusA().kilometers()));
    m_ui->bRadiiLabel->setText(tr("%1").arg(m_target->radiusB().kilometers()));
    m_ui->cRadiiLabel->setText(tr("%1").arg(m_target->radiusC().kilometers()));
    m_ui->meanRadiiLabel->setText(tr("%1").arg(m_target->meanRadius().kilometers()));
  }


  /**
   * Destructor
   */
  TargetInfoWidget::~TargetInfoWidget() {
    delete m_ui;
  }


  /**
   * Make the poleRightAscensionLabel text using information from the target.
   * 
   * @return @b QString The poleRightAscensionLabel text
   */
  QString TargetInfoWidget::formatPoleRaString() {
    std::vector<Angle> poleRaCoefs = m_target->poleRaCoefs();
    std::vector<double> poleRaNutPrecCoefs = m_target->poleRaNutPrecCoefs();

    const QChar degChar(0260);
    QString poleRaString = "";
    QString coefLetter = m_target->naifPlanetSystemName().at(0);

    if (poleRaCoefs[1].degrees() < 0.0 ) {
      poleRaString.append(tr("%1%3 - %2T").arg(poleRaCoefs[0].degrees()).arg(-poleRaCoefs[1]
          .degrees()).arg(degChar));
    }
    else {
      poleRaString.append(tr("%1%3 + %2T").arg(poleRaCoefs[0].degrees()).arg(poleRaCoefs[1]
          .degrees()).arg(degChar));
    }

    QString tmp;
    int nCoefs = poleRaNutPrecCoefs.size();;
    for (int i = 0; i < nCoefs; i++) {
      if (poleRaNutPrecCoefs[i] < 0.0 ) {
        tmp.append(tr(" - %1%2%3").arg(-poleRaNutPrecCoefs[i]).arg("sin %1").arg(coefLetter)
                                  .arg(i+1));
      }
      else if (poleRaNutPrecCoefs[i] > 0.0 ) {
        tmp.append(tr(" + %1%2%3").arg(poleRaNutPrecCoefs[i]).arg("sin %1").arg(coefLetter)
                                  .arg(i+1));
      }
    }

    poleRaString.append(tmp);

    return poleRaString;
  }


  /**
   * Make the poleDeclinationLabel text using information from the target.
   * 
   * @return @b QString The poleDeclinationLabel text
   */
  QString TargetInfoWidget::formatPoleDecString() {
    std::vector<Angle> poleDecCoefs = m_target->poleDecCoefs();
    std::vector<double> poleDecNutPrecCoefs = m_target->poleDecNutPrecCoefs();

    const QChar degChar(0260);
    QString poleDecString = "";
    QString coefLetter = m_target->naifPlanetSystemName().at(0);

    if (poleDecCoefs[1].degrees() < 0.0 ) {
      poleDecString.append(tr("%1%3 - %2T").arg(poleDecCoefs[0].degrees()).arg(-poleDecCoefs[1]
          .degrees()).arg(degChar));
    }
    else {
      poleDecString.append(tr("%1%3 + %2T").arg(poleDecCoefs[0].degrees()).arg(poleDecCoefs[1]
          .degrees()).arg(degChar));
    }

    QString tmp;
    int nCoefs = poleDecNutPrecCoefs.size();;
    for (int i = 0; i < nCoefs; i++) {
      if (poleDecNutPrecCoefs[i] < 0.0 ) {
        tmp.append(tr(" - %1%2%3").arg(-poleDecNutPrecCoefs[i]).arg("cos %1").arg(coefLetter)
                                  .arg(i+1));
      }
      else if (poleDecNutPrecCoefs[i] > 0.0 ) {
        tmp.append(tr(" + %1%2%3").arg(poleDecNutPrecCoefs[i]).arg("cos %1").arg(coefLetter)
                                  .arg(i+1));
      }
    }

    poleDecString.append(tmp);

    return poleDecString;
  }


  /**
   * Make the polePMOffsetLabel text using information from the target.
   * 
   * @return @b QString The polePMOffsetLabel text
   */
  QString TargetInfoWidget::formatPmString() {
    std::vector<Angle> pmCoefs = m_target->pmCoefs();
    std::vector<double> pmNutPrecCoefs = m_target->pmNutPrecCoefs();

    const QChar degChar(0260);
    QString pmString = "";
    QString coefLetter = m_target->naifPlanetSystemName().at(0);

    if (pmCoefs[1].degrees() < 0.0 ) {
      pmString.append(tr("%1%3 - %2d").arg(pmCoefs[0].degrees()).arg(-pmCoefs[1].degrees())
          .arg(degChar));
    }
    else if (pmCoefs[1].degrees() > 0.0 ) {
      pmString.append(tr("%1%3 + %2d").arg(pmCoefs[0].degrees()).arg(pmCoefs[1].degrees())
          .arg(degChar));
    }

    if (pmCoefs[2].degrees() < 0.0 ) {
      pmString.append(tr(" - %2d^2").arg(-pmCoefs[2].degrees()));
    }
    else if (pmCoefs[2].degrees() > 0.0 ) {
      pmString.append(tr(" + %2d^2").arg(pmCoefs[2].degrees()));
    }


    QString tmp;
    int nCoefs = pmNutPrecCoefs.size();;
    for (int i = 0; i < nCoefs; i++) {
      if (pmNutPrecCoefs[i] < 0.0 ) {
        tmp.append(tr(" - %1%2%3").arg(-pmNutPrecCoefs[i]).arg("sin %1").arg(coefLetter).arg(i+1));
      }
      else if (pmNutPrecCoefs[i] > 0.0 ) {
        tmp.append(tr(" + %1%2%3").arg(pmNutPrecCoefs[i]).arg("sin %1").arg(coefLetter).arg(i+1));
      }
    }

    pmString.append(tmp);

    return pmString;
  }
}
