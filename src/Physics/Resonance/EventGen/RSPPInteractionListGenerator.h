//____________________________________________________________________________
/*!

\class    genie::RSPPInteractionListGenerator

\brief    Creates a list of all the interactions that can be generated by the
          SPP thread (generates exclusive inelastic 1 pion reactions proceeding
          through resonance neutrinoproduction).
          Concrete implementations of the InteractionListGeneratorI interface.

\author   Costas Andreopoulos <costas.andreopoulos \at stfc.ac.uk>
          University of Liverpool & STFC Rutherford Appleton Lab

\created  May 13, 2005

\cpright  Copyright (c) 2003-2017, GENIE Neutrino MC Generator Collaboration
          For the full text of the license visit http://copyright.genie-mc.org
          or see $GENIE/LICENSE
*/
//____________________________________________________________________________

#ifndef _RSPP_INTERACTION_LIST_GENERATOR_H_
#define _RSPP_INTERACTION_LIST_GENERATOR_H_

#include "Framework/EventGen/InteractionListGeneratorI.h"
#include "Interaction/SppChannel.h"

namespace genie {

class RSPPInteractionListGenerator : public InteractionListGeneratorI {

public :
  RSPPInteractionListGenerator();
  RSPPInteractionListGenerator(string config);
 ~RSPPInteractionListGenerator();

  // implement the InteractionListGeneratorI interface
  InteractionList * CreateInteractionList(const InitialState & init) const;

  // overload the Algorithm::Configure() methods to load private data
  // members from configuration options
  void Configure(const Registry & config);
  void Configure(string config);

private:

  void AddFinalStateInfo (Interaction * i, SppChannel_t chan) const;
  void LoadConfigData(void);

  bool          fIsCC;
  bool          fIsNC;
};

}      // genie namespace
#endif // _RSPP_INTERACTION_LIST_GENERATOR_H_
