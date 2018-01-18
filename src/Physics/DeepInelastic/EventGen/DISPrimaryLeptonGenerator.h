//____________________________________________________________________________
/*!

\class    genie::DISPrimaryLeptonGenerator

\brief    Generates the final state primary lepton in v DIS interactions.
          Is a concrete implementation of the EventRecordVisitorI interface.

\author   Costas Andreopoulos <costas.andreopoulos \at stfc.ac.uk>
          University of Liverpool & STFC Rutherford Appleton Lab

\created  October 03, 2004

\cpright  Copyright (c) 2003-2017, GENIE Neutrino MC Generator Collaboration
          For the full text of the license visit http://copyright.genie-mc.org
          or see $GENIE/LICENSE
*/
//____________________________________________________________________________

#ifndef _DIS_PRIMARY_LEPTON_GENERATOR_H_
#define _DIS_PRIMARY_LEPTON_GENERATOR_H_

#include "Physics/Common/PrimaryLeptonGenerator.h"

namespace genie {

class DISPrimaryLeptonGenerator : public PrimaryLeptonGenerator {

public :
  DISPrimaryLeptonGenerator();
  DISPrimaryLeptonGenerator(string config);
  ~DISPrimaryLeptonGenerator();

  // implement the EventRecordVisitorI interface
  void ProcessEventRecord(GHepRecord * event_rec) const;
};

}      // genie namespace
#endif // _DIS_PRIMARY_LEPTON_GENERATOR_H_
