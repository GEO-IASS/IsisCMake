#if !defined(Anisotropic2_h)
#define Anisotropic2_h
/**
 * @file
 * $Revision: 1.5 $
 * $Date: 2008/11/05 23:36:47 $
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

#include "AtmosModel.h"

namespace Isis {
  class Pvl;

  /**
   * @brief
   *
   *
   * @ingroup RadiometricAndPhotometricCorrection
   * @author 1998-12-21 Randy Kirk
   *
   * @internal
   *  @history 1998-12-21 Randy Kirk - USGS, Flagstaff - Original
   *          code.
   *  @history 2007-02-20 Janet Barrett - Imported from Isis2.
   *  @history 2007-08-15 Steven Lambright - Refactored code
   *  @history 2008-03-07 Janet Barrett - Moved code to set standard
   *                      conditions to the AtmosModel class
   *  @history 2008-06-18 Christopher Austin - Fixed documentation error
   *  @history 2008-11-05 Jeannie Walldren - Replaced references
   *            to NumericalMethods::r8expint() with
   *            AtmosModel::En(), NumericalMethods::G11Prime()
   *            with AtmosModel::G11Prime(), and
   *            NumericalMethods::r8ei() with AtmosModel::Ei().
   *            Added documentation from Isis2.
   *  @history 2011-08-17 Sharmila Prasad - Moved common HNORM to base AtmosModel
   *  @history 2011-12-19 Janet Barrett - Added code to estimate the
   *           shadow brightness value (transs). Also got rid of
   *           unnecessary check for identical photometric angle values
   *           between successive calls. This check should only be
   *           made in the photometric models.
   */
  class Anisotropic2 : public AtmosModel {
    public:
      Anisotropic2(Pvl &pvl, PhotoModel &pmodel);
      virtual ~Anisotropic2() {};

    protected:
      virtual void AtmosModelAlgorithm(double phase, double incidence,
                                       double emission);

    private:
      double p_wha2;
      double p_wham;
      double p_e1, p_e1_2, p_e2, p_e3, p_e4, p_e5;
      double p_em;
      double p_e;
      double p_f1m, p_f2m, p_f3m, p_f4m;
      double p_g12, p_g13, p_g14;
      double p_g32, p_g33, p_g34;
      double p_f1, p_f2, p_f3, p_f4;
      double p_g11p, p_g12p, p_g13p, p_g14p;
      double p_g32p, p_g33p, p_g34p;
      double p_x0_0, p_y0_0;
      double p_x0_1, p_y0_1;
      double p_delta_0, p_delta_1;
      double p_alpha0_0, p_alpha1_0;
      double p_beta0_0, p_beta1_0;
      double p_fac;
      double p_den;
      double p_p0, p_q0, p_p1, p_q1, p_q02p02, p_q12p12;
  };
};

#endif
