#ifndef JigsawWorkOrder_H
#define JigsawWorkOrder_H
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
#include "WorkOrder.h"

namespace Isis {
  /**
   * This work order allows the user to run a bundle adjustment (jigsaw).
   *
   * @author 2014-04-03 Ken Edmundson
   *
   * @internal
   *   @history 2014-06-04 Jeannie Backer - Fixed JigsawWorkOrder error.
   *   @history 2015-09-05 Ken Edmundson - Added preliminary target body functionality to IPCE.
   *   @history 2016-06-06 Makayla Shepherd - Updated documentation. Fixes #3993.
   */
  class JigsawWorkOrder : public WorkOrder {
      Q_OBJECT
    public:
      JigsawWorkOrder(Project *project);
      JigsawWorkOrder(const JigsawWorkOrder &other);
      ~JigsawWorkOrder();

      virtual JigsawWorkOrder *clone() const;

      virtual bool isExecutable();
      bool execute();

    protected:
      bool dependsOn(WorkOrder *other) const;
      void syncRedo();
      void syncUndo();

    private:
      JigsawWorkOrder &operator=(const JigsawWorkOrder &rhs);
  };
}
#endif

