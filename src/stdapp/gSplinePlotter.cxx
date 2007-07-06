//____________________________________________________________________________
/*!

\program gsplt

\brief   GENIE utility program to produce cross section plots from the XML
         cross section spline data that GENIE MC drivers can load and / or
         create by themselves at job initialization.

         Syntax :
           gsplt -f xml_file -p nu_pdg -t target_pdg [-e emax] [-o root_file]

         Options :
           []  denotes an optional argument
           -f  the input XML file containing the cross section spline data
           -p  the neutrino pdg code
           -t  the target pdg code (format: 1aaazzz000)
           -e  the maximum energy (in generated plots -- use it to zoom at low E)
           -o  if an output ROOT file is specified then the cross section graphs 
               will be saved there as well 
               Note 1: these graphs can be used to instantiate splines in bare 
               root sessions -- effectively, they provide you with cross section 
               functions --.
               Note 2: the input ROOT file will not be recreated if it already 
               exists. The graphs are saved in a TDirectory named after the 
               neutrino+target names. That allows you to save all graphs in a 
               single root file (with multiple directories)

         Example:

           gsplt -f ~/mydata/mysplines.xml -p 14 -t 1056026000 

           will load the cross section splines from the XML file mysplines.xml,
           then will select the cross section splines that are relevant to 
           numu+Fe56 and will generate cross section plots.
           The generated cross section plots will be saved in a postscript
           document named "xsec-splines-nu_mu-Fe56.ps"

         Notes:
         - To create the cross sections splines in XML format (for some target
           list or input geometry and for some input neutrino list) run the gmkspl
           GENIE application (see $GENIE/src/stdapp/gMakeSplines.cxx)

\author  Costas Andreopoulos <C.V.Andreopoulos@rl.ac.uk>
         CCLRC, Rutherford Appleton Laboratory

\created December 15, 2005

\cpright Copyright (c) 2003-2007, GENIE Neutrino MC Generator Collaboration
         All rights reserved.
         For the licensing terms see $GENIE/USER_LICENSE.
*/
//____________________________________________________________________________

#include <cassert>
#include <string>
#include <sstream>
#include <vector>

#include <TSystem.h>
#include <TFile.h>
#include <TTree.h>
#include <TDirectory.h>
#include <TPostScript.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TGraph.h>
#include <TPaveText.h>
#include <TString.h>
#include <TH1F.h>

#include "BaryonResonance/BaryonResonance.h"
#include "BaryonResonance/BaryonResUtils.h"
#include "Conventions/XmlParserStatus.h"
#include "Conventions/Units.h"
#include "EVGCore/InteractionList.h"
#include "EVGDrivers/GEVGDriver.h"
#include "Interaction/Interaction.h"
#include "Messenger/Messenger.h"
#include "Numerical/Spline.h"
#include "PDG/PDGCodes.h"
#include "PDG/PDGUtils.h"
#include "PDG/PDGLibrary.h"
#include "Utils/XSecSplineList.h"
#include "Utils/StringUtils.h"
#include "Utils/CmdLineArgParserUtils.h"
#include "Utils/CmdLineArgParserException.h"

using std::string;
using std::vector;
using std::ostringstream;

using namespace genie;
using namespace genie::utils;

//Prototypes:
void       LoadSplines          (void);
GEVGDriver GetEventGenDriver    (void);
void       SaveToPsFile         (void);
void       SaveGraphsToRootFile (void);
void       SaveNtupleToRootFile (void);
void       GetCommandLineArgs   (int argc, char ** argv);
void       PrintSyntax          (void);

//User-specified options:
string gOptXMLFilename;  // input XML filename
string gOptROOTFilename; // output ROOT filename
double gOptNuEnergy;     // Ev(max)
int    gOptNuPdgCode;    // neutrino PDG code
int    gOptTgtPdgCode;   // target PDG code

//Globals & constants
double gEmin;
double gEmax;
const int    kNP       = 300;
const int    kNSplineP = 1000;
const int    kPsType   = 111;  // ps type: portrait
const double kEmin     = 0.01; // minimum energy in plots (GeV)

//____________________________________________________________________________
int main(int argc, char ** argv)
{
  //-- parse command line arguments
  GetCommandLineArgs(argc,argv);

  //-- load the x-section splines xml file specified by the user
  LoadSplines();

  //-- save the cross section plots in a postscript file
  SaveToPsFile();

  //-- save the cross section graphs at a root file
  //   (these graphs can then be used to create splines
  SaveGraphsToRootFile();

  return 0;
}
//____________________________________________________________________________
void LoadSplines(void)
{
// load the cross section splines specified at the cmd line

  XSecSplineList * splist = XSecSplineList::Instance();
  XmlParserStatus_t ist = splist->LoadFromXml(gOptXMLFilename);
  assert(ist == kXmlOK);
}
//____________________________________________________________________________
GEVGDriver GetEventGenDriver(void)
{
// create an event genartion driver configured for the specified initial state 
// (so that cross section splines will be accessed through that driver as in 
// event generation mode)

  InitialState init_state(gOptTgtPdgCode, gOptNuPdgCode);
           
  GEVGDriver evg_driver;
  evg_driver.Configure(init_state);
  evg_driver.CreateSplines();
  evg_driver.CreateXSecSumSpline (100, gEmin, gEmax);

  return evg_driver;
}
//____________________________________________________________________________
void SaveToPsFile(void)
{
  //-- get the event generation driver
  GEVGDriver evg_driver = GetEventGenDriver();

  //-- define some marker styles / colors
  const unsigned int kNMarkers = 5;
  const unsigned int kNColors  = 6;
  unsigned int markers[kNMarkers] = {20, 28, 29, 27, 3};
  unsigned int colors [kNColors]  = {1, 2, 4, 6, 8, 28};

  //-- create a postscript document for saving cross section plpots

  TCanvas * c = new TCanvas("c","",20,20,500,850);
  c->SetBorderMode(0);
  c->SetFillColor(0);
  TLegend * legend = new TLegend(0.01,0.01,0.99,0.99);
  legend->SetFillColor(0);
  legend->SetBorderSize(0);

  //-- get pdglibrary for mapping pdg codes to names   
  PDGLibrary * pdglib = PDGLibrary::Instance();
  ostringstream filename;
  filename << "xsec-splines-" 
          <<  pdglib->Find(gOptNuPdgCode)->GetName()  << "-"
          <<  pdglib->Find(gOptTgtPdgCode)->GetName() << ".ps";
  TPostScript * ps = new TPostScript(filename.str().c_str(), kPsType);

  //-- get the list of interactions that can be simulated by the driver
  const InteractionList * ilist = evg_driver.Interactions();
  unsigned int nspl = ilist->size();

  //-- book enough space for xsec plots (last one is the sum)
  TGraph * gr[nspl+1];

  //-- loop over all the simulated interactions & create the cross section graphs
  InteractionList::const_iterator ilistiter = ilist->begin();
  unsigned int i=0;
  for(; ilistiter != ilist->end(); ++ilistiter) {
    
    const Interaction * interaction = *ilistiter;
    LOG("gsplt", pINFO) 
       << "Current interaction: " << interaction->AsString();

    //-- access the cross section spline
    const Spline * spl = evg_driver.XSecSpline(interaction);
    if(!spl) {
      LOG("gsplt", pWARN) << "Can't get spline for: " << interaction->AsString();
      exit(2);
    }

    //-- set graph color/style
    int icol = TMath::Min( i % kNColors,  kNColors-1  );
    int isty = TMath::Min( i / kNMarkers, kNMarkers-1 );
    int col = colors[icol];
    int sty = markers[isty];

    LOG("gsplt", pINFO) << "color = " << col << ", marker = " << sty;

    //-- export Spline as TGraph / set color & stype
    gr[i] = spl->GetAsTGraph(kNP,true,true,1.,1./units::cm2);
    gr[i]->SetLineColor(col);
    gr[i]->SetMarkerColor(col);
    gr[i]->SetMarkerStyle(sty);
    gr[i]->SetMarkerSize(0.5);

    i++;
  }

  //-- now, get the sum...
  const Spline * splsum = evg_driver.XSecSumSpline();
  if(!splsum) {
      LOG("gsplt", pWARN) << "Can't get the cross section sum spline";
      exit(2);
  }
  gr[nspl] = splsum->GetAsTGraph(kNP,true,true,1.,1./units::cm2);

  //-- figure out the minimum / maximum xsec in plotted range
  double XSmax = -9999;
  double XSmin =  9999;
  double x=0, y=0;
  for(int j=0; j<kNP; j++) {
    gr[nspl]->GetPoint(j,x,y);
    XSmax = TMath::Max(XSmax,y);
  }
  XSmin = XSmax/100000.;
  XSmax = XSmax*1.2;

  LOG("gsplt", pINFO) << "Drawing frame: E    = (" << gEmin  << ", " << gEmax << ")";
  LOG("gsplt", pINFO) << "Drawing frame: XSec = (" << XSmin  << ", " << XSmax << ")";

  //-- ps output: add the 1st page with _all_ xsec spline plots
  //
  //c->Draw();
  TH1F * h = (TH1F*) c->DrawFrame(gEmin, XSmin, gEmax, XSmax);
  for(unsigned int i = 0; i <= nspl; i++) if(gr[i]) gr[i]->Draw("LP");
  h->GetXaxis()->SetTitle("Ev (GeV)");
  h->GetYaxis()->SetTitle("#sigma_{nuclear}/Ev (cm^{2}/GeV)");
  c->SetLogx();
  c->SetLogy();
  c->SetGridx();
  c->SetGridy();
  c->Update();

  //-- plot QEL xsecs only
  //
  h = (TH1F*) c->DrawFrame(gEmin, XSmin, gEmax, XSmax);
  i=0;
  for(ilistiter = ilist->begin(); ilistiter != ilist->end(); ++ilistiter) {    
    const Interaction * interaction = *ilistiter;
    if(interaction->ProcInfo().IsQuasiElastic()) {
        gr[i]->Draw("LP");
        TString spltitle(interaction->AsString());
        spltitle = spltitle.ReplaceAll(";",1," ",1);
        legend->AddEntry(gr[i], spltitle.Data(),"LP");
    }
    i++;
  }
  legend->SetHeader("QEL Cross Sections");
  gr[nspl]->Draw("LP");
  legend->AddEntry(gr[nspl], "sum","LP");
  h->GetXaxis()->SetTitle("Ev (GeV)");
  h->GetYaxis()->SetTitle("#sigma_{nuclear}/Ev (cm^{2}/GeV)");
  c->SetLogx();
  c->SetLogy();
  c->SetGridx();
  c->SetGridy();
  c->Update();
  c->Clear();
  c->Range(0,0,1,1);
  legend->Draw();
  c->Update();

  //-- plot RES xsecs only
  //
  h = (TH1F*) c->DrawFrame(gEmin, XSmin, gEmax, XSmax);
  legend->Clear();
  i=0;
  for(ilistiter = ilist->begin(); ilistiter != ilist->end(); ++ilistiter) {    
    const Interaction * interaction = *ilistiter;
    if(interaction->ProcInfo().IsResonant()) {
        gr[i]->Draw("LP");
        TString spltitle(interaction->AsString());
        spltitle = spltitle.ReplaceAll(";",1," ",1);
        legend->AddEntry(gr[i], spltitle.Data(),"LP");
    }
    i++;
  }
  legend->SetHeader("RES Cross Sections");
  gr[nspl]->Draw("LP");
  legend->AddEntry(gr[nspl], "sum","LP");
  h->GetXaxis()->SetTitle("Ev (GeV)");
  h->GetYaxis()->SetTitle("#sigma_{nuclear}/Ev (cm^{2}/GeV)");
  c->SetLogx();
  c->SetLogy();
  c->SetGridx();
  c->SetGridy();
  c->Update();
  c->Clear();
  c->Range(0,0,1,1);
  legend->Draw();
  c->Update();

  //-- plot DIS xsecs only
  //
  h = (TH1F*) c->DrawFrame(gEmin, XSmin, gEmax, XSmax);
  legend->Clear();
  i=0;
  for(ilistiter = ilist->begin(); ilistiter != ilist->end(); ++ilistiter) {    
    const Interaction * interaction = *ilistiter;
    if(interaction->ProcInfo().IsDeepInelastic()) {
        gr[i]->Draw("LP");
        TString spltitle(interaction->AsString());
        spltitle = spltitle.ReplaceAll(";",1," ",1);
        legend->AddEntry(gr[i], spltitle.Data(),"LP");
    }
    i++;
  }
  legend->SetHeader("DIS Cross Sections");
  gr[nspl]->Draw("LP");
  legend->AddEntry(gr[nspl], "sum","LP");
  h->GetXaxis()->SetTitle("Ev (GeV)");
  h->GetYaxis()->SetTitle("#sigma_{nuclear}/Ev (cm^{2}/GeV)");
  c->SetLogx();
  c->SetLogy();
  c->SetGridx();
  c->SetGridy();
  c->Update();
  c->Clear();
  c->Range(0,0,1,1);
  legend->Draw();
  c->Update();

  //-- plot COH xsecs only
  //
  h = (TH1F*) c->DrawFrame(gEmin, XSmin, gEmax, XSmax);
  legend->Clear();
  i=0;
  for(ilistiter = ilist->begin(); ilistiter != ilist->end(); ++ilistiter) {    
    const Interaction * interaction = *ilistiter;
    if(interaction->ProcInfo().IsCoherent()) {
        gr[i]->Draw("LP");
        TString spltitle(interaction->AsString());
        spltitle = spltitle.ReplaceAll(";",1," ",1);
        legend->AddEntry(gr[i], spltitle.Data(),"LP");
    }
    i++;
  }
  legend->SetHeader("COH Cross Sections");
  gr[nspl]->Draw("LP");
  legend->AddEntry(gr[nspl], "sum","LP");
  h->GetXaxis()->SetTitle("Ev (GeV)");
  h->GetYaxis()->SetTitle("#sigma_{nuclear}/Ev (cm^{2}/GeV)");
  c->SetLogx();
  c->SetLogy();
  c->SetGridx();
  c->SetGridy();
  c->Update();
  c->Clear();
  c->Range(0,0,1,1);
  legend->Draw();
  c->Update();

  //-- plot ve xsecs only
  //
  h = (TH1F*) c->DrawFrame(gEmin, XSmin, gEmax, XSmax);
  legend->Clear();
  i=0;
  for(ilistiter = ilist->begin(); ilistiter != ilist->end(); ++ilistiter) {    
    const Interaction * interaction = *ilistiter;
    if(interaction->ProcInfo().IsInverseMuDecay() ||
       interaction->ProcInfo().IsNuElectronElastic()) {
        gr[i]->Draw("LP");
        TString spltitle(interaction->AsString());
        spltitle = spltitle.ReplaceAll(";",1," ",1);
        legend->AddEntry(gr[i], spltitle.Data(),"LP");
    }
    i++;
  }
  legend->SetHeader("IMD and ve Elastic Cross Sections");
  gr[nspl]->Draw("LP");
  legend->AddEntry(gr[nspl], "sum","LP");
  h->GetXaxis()->SetTitle("Ev (GeV)");
  h->GetYaxis()->SetTitle("#sigma_{nuclear}/Ev (cm^{2}/GeV)");
  c->SetLogx();
  c->SetLogy();
  c->SetGridx();
  c->SetGridy();
  c->Update();
  c->Clear();
  c->Range(0,0,1,1);
  legend->Draw();
  c->Update();

  //-- close the postscript document 
  ps->Close();

  //-- clean-up
  for(unsigned int j=0; j<=nspl; j++) { if(gr[j]) delete gr[j]; }
  delete c;
  delete ps;
}
//____________________________________________________________________________
void SaveGraphsToRootFile(void)
{
  //-- get the event generation driver
  GEVGDriver evg_driver = GetEventGenDriver();

  //-- get the list of interactions that can be simulated by the driver
  const InteractionList * ilist = evg_driver.Interactions();

  //-- check whether the splines will be saved in a ROOT file - if not, exit now
  bool save_in_root = gOptROOTFilename.size()>0;
  if(!save_in_root) return;
  
  //-- get pdglibrary for mapping pdg codes to names
  PDGLibrary * pdglib = PDGLibrary::Instance();

  //-- check whether the requested filename exists
  //   if yes, then open the file in 'update' mode 
  bool exists = !(gSystem->AccessPathName(gOptROOTFilename.c_str()));

  TFile * froot = 0;
  if(exists) froot = new TFile(gOptROOTFilename.c_str(), "UPDATE");
  else       froot = new TFile(gOptROOTFilename.c_str(), "RECREATE");
  assert(froot);

  //-- create directory 
  ostringstream dptr;
  dptr << pdglib->Find(gOptNuPdgCode)->GetName() << "_" 
       << pdglib->Find(gOptTgtPdgCode)->GetName();

  ostringstream dtitle;
  dtitle << "Cross sections for: "
         << pdglib->Find(gOptNuPdgCode)->GetName() << "+" 
         << pdglib->Find(gOptTgtPdgCode)->GetName();

  LOG("gsplt", pINFO) 
           << "Will store graphs in root directory = " << dptr.str();
  TDirectory * topdir = 
         dynamic_cast<TDirectory *> (froot->Get(dptr.str().c_str()));
  if(topdir) {
     LOG("gsplt", pINFO) 
       << "Directory: " << dptr.str() << " already exists!! Exiting";
     froot->Close();
     delete froot;
     return;
  }

  topdir = froot->mkdir(dptr.str().c_str(),dtitle.str().c_str());
  topdir->cd();

  double   de = (gEmax-gEmin)/(kNSplineP-1);
  double * e  = new double[kNSplineP];
  for(int i=0; i<kNSplineP; i++) {  e[i]  = gEmin + i*de; }

  double * xs = new double[kNSplineP];

  InteractionList::const_iterator ilistiter = ilist->begin();

  for(; ilistiter != ilist->end(); ++ilistiter) {    

    const Interaction * interaction = *ilistiter;

    const ProcessInfo &  proc = interaction->ProcInfo();
    const XclsTag &      xcls = interaction->ExclTag();
    const InitialState & init = interaction->InitState();
    const Target &       tgt  = init.Tgt();

    // graph title
    ostringstream title;
   
    if      (proc.IsQuasiElastic()     ) { title << "qel"; }
    else if (proc.IsResonant()         ) { title << "res"; }
    else if (proc.IsDeepInelastic()    ) { title << "dis"; }
    else if (proc.IsCoherent(  )       ) { title << "coh"; }
    else if (proc.IsInverseMuDecay()   ) { title << "imd"; }
    else if (proc.IsNuElectronElastic()) { title << "ve";  }
    else                                 { continue;       }

    if      (proc.IsWeakCC()) { title << "_cc"; }
    else if (proc.IsWeakNC()) { title << "_nc"; }
    else                      { continue;       }

    if(tgt.HitNucIsSet()) {
      int hitnuc = tgt.HitNucPdg();
      if      ( pdg::IsProton (hitnuc) ) { title << "_p"; }
      else if ( pdg::IsNeutron(hitnuc) ) { title << "_n"; }

      if(tgt.HitQrkIsSet()) {
        int  qrkpdg = tgt.HitQrkPdg();
        bool insea  = tgt.HitSeaQrk();

        if      ( pdg::IsUQuark(qrkpdg)     ) { title << "_u";    }
        else if ( pdg::IsDQuark(qrkpdg)     ) { title << "_d";    }
        else if ( pdg::IsSQuark(qrkpdg)     ) { title << "_s";    }
        else if ( pdg::IsCQuark(qrkpdg)     ) { title << "_c";    }
        else if ( pdg::IsAntiUQuark(qrkpdg) ) { title << "_ubar"; }
        else if ( pdg::IsAntiDQuark(qrkpdg) ) { title << "_dbar"; }
        else if ( pdg::IsAntiSQuark(qrkpdg) ) { title << "_sbar"; }
        else if ( pdg::IsAntiCQuark(qrkpdg) ) { title << "_cbar"; }

        if(insea) { title << "sea"; }
        else      { title << "val"; }
      }
    }
    if(proc.IsResonant()) { 
       Resonance_t res = xcls.Resonance();
       string resname = res::AsString(res);
       resname = str::FilterString(")", resname);
       resname = str::FilterString("(", resname);
       title << "_" << resname.substr(3,4) << resname.substr(0,3);
    }

    if(xcls.IsCharmEvent()) {
        title << "_charm";
        if(!xcls.IsInclusiveCharm()) { title << xcls.CharmHadronPdg(); }
    }

    const Spline * spl = evg_driver.XSecSpline(interaction);
    for(int i=0; i<kNSplineP; i++) {
      xs[i] = spl->Evaluate(e[i]) * (1E+38/units::cm2);
    }

    TGraph * gr = new TGraph(kNSplineP, e, xs);
    gr->SetName(title.str().c_str());
    gr->SetTitle("GENIE cross section graph");

    topdir->Add(gr);
  }

  // add-up all res channels
  //
  double * xsresccp = new double[kNSplineP];
  double * xsresccn = new double[kNSplineP];
  double * xsresncp = new double[kNSplineP];
  double * xsresncn = new double[kNSplineP];
  for(int i=0; i<kNSplineP; i++) {
    xsresccp[i] = 0;
    xsresccn[i] = 0;
    xsresncp[i] = 0;
    xsresncn[i] = 0;
  }

  for(ilistiter = ilist->begin(); ilistiter != ilist->end(); ++ilistiter) {    
    const Interaction * interaction = *ilistiter;
    const ProcessInfo &  proc = interaction->ProcInfo();
    const InitialState & init = interaction->InitState();
    const Target &       tgt  = init.Tgt();

    const Spline * spl = evg_driver.XSecSpline(interaction);
 
    if (proc.IsResonant() && proc.IsWeakCC() && pdg::IsProton(tgt.HitNucPdg())) {
      for(int i=0; i<kNSplineP; i++) { 
          xsresccp[i] += (spl->Evaluate(e[i]) * (1E+38/units::cm2)); 
      }
    }
    if (proc.IsResonant() && proc.IsWeakCC() && pdg::IsNeutron(tgt.HitNucPdg())) {
      for(int i=0; i<kNSplineP; i++) { 
          xsresccn[i] += (spl->Evaluate(e[i]) * (1E+38/units::cm2)); 
      }
    }
    if (proc.IsResonant() && proc.IsWeakNC() && pdg::IsProton(tgt.HitNucPdg())) {
      for(int i=0; i<kNSplineP; i++) { 
          xsresncp[i] += (spl->Evaluate(e[i]) * (1E+38/units::cm2)); 
      }
    }
    if (proc.IsResonant() && proc.IsWeakNC() && pdg::IsNeutron(tgt.HitNucPdg())) {
      for(int i=0; i<kNSplineP; i++) { 
          xsresncn[i] += (spl->Evaluate(e[i]) * (1E+38/units::cm2)); 
      }
    }
  }

  TGraph * gr_resccp = new TGraph(kNSplineP, e, xsresccp);
  gr_resccp->SetName("res_cc_p");
  gr_resccp->SetTitle("GENIE cross section graph");
  topdir->Add(gr_resccp);
  TGraph * gr_resccn = new TGraph(kNSplineP, e, xsresccn);
  gr_resccn->SetName("res_cc_n");
  gr_resccn->SetTitle("GENIE cross section graph");
  topdir->Add(gr_resccn);
  TGraph * gr_resncp = new TGraph(kNSplineP, e, xsresncp);
  gr_resncp->SetName("res_nc_p");
  gr_resncp->SetTitle("GENIE cross section graph");
  topdir->Add(gr_resncp);
  TGraph * gr_resncn = new TGraph(kNSplineP, e, xsresncn);
  gr_resncn->SetName("res_nc_n");
  gr_resncn->SetTitle("GENIE cross section graph");
  topdir->Add(gr_resncn);

  // add-up all dis channels
  //
  double * xsdisccp = new double[kNSplineP];
  double * xsdisccn = new double[kNSplineP];
  double * xsdisncp = new double[kNSplineP];
  double * xsdisncn = new double[kNSplineP];
  for(int i=0; i<kNSplineP; i++) {
    xsdisccp[i] = 0;
    xsdisccn[i] = 0;
    xsdisncp[i] = 0;
    xsdisncn[i] = 0;
  }
  for(ilistiter = ilist->begin(); ilistiter != ilist->end(); ++ilistiter) {    
    const Interaction * interaction = *ilistiter;
    const ProcessInfo &  proc = interaction->ProcInfo();
    const XclsTag &      xcls = interaction->ExclTag();
    const InitialState & init = interaction->InitState();
    const Target &       tgt  = init.Tgt();

    const Spline * spl = evg_driver.XSecSpline(interaction);

    if(xcls.IsCharmEvent()) continue;
 
    if (proc.IsDeepInelastic() && proc.IsWeakCC() && pdg::IsProton(tgt.HitNucPdg())) {
      for(int i=0; i<kNSplineP; i++) { 
          xsdisccp[i] += (spl->Evaluate(e[i]) * (1E+38/units::cm2)); 
      }
    }
    if (proc.IsDeepInelastic() && proc.IsWeakCC() && pdg::IsNeutron(tgt.HitNucPdg())) {
      for(int i=0; i<kNSplineP; i++) { 
          xsdisccn[i] += (spl->Evaluate(e[i]) * (1E+38/units::cm2)); 
      }
    }
    if (proc.IsDeepInelastic() && proc.IsWeakNC() && pdg::IsProton(tgt.HitNucPdg())) {
      for(int i=0; i<kNSplineP; i++) { 
          xsdisncp[i] += (spl->Evaluate(e[i]) * (1E+38/units::cm2)); 
      }
    }
    if (proc.IsDeepInelastic() && proc.IsWeakNC() && pdg::IsNeutron(tgt.HitNucPdg())) {
      for(int i=0; i<kNSplineP; i++) { 
          xsdisncn[i] += (spl->Evaluate(e[i]) * (1E+38/units::cm2)); 
      }
    }
  }
  TGraph * gr_disccp = new TGraph(kNSplineP, e, xsdisccp);
  gr_disccp->SetName("dis_cc_p");
  gr_disccp->SetTitle("GENIE cross section graph");
  topdir->Add(gr_disccp);
  TGraph * gr_disccn = new TGraph(kNSplineP, e, xsdisccn);
  gr_disccn->SetName("dis_cc_n");
  gr_disccn->SetTitle("GENIE cross section graph");
  topdir->Add(gr_disccn);
  TGraph * gr_disncp = new TGraph(kNSplineP, e, xsdisncp);
  gr_disncp->SetName("dis_nc_p");
  gr_disncp->SetTitle("GENIE cross section graph");
  topdir->Add(gr_disncp);
  TGraph * gr_disncn = new TGraph(kNSplineP, e, xsdisncn);
  gr_disncn->SetName("dis_nc_n");
  gr_disncn->SetTitle("GENIE cross section graph");
  topdir->Add(gr_disncn);


  // add-up all charm dis channels
  //
  for(int i=0; i<kNSplineP; i++) {
    xsdisccp[i] = 0;
    xsdisccn[i] = 0;
    xsdisncp[i] = 0;
    xsdisncn[i] = 0;
  }
  for(ilistiter = ilist->begin(); ilistiter != ilist->end(); ++ilistiter) {    
    const Interaction * interaction = *ilistiter;
    const ProcessInfo &  proc = interaction->ProcInfo();
    const XclsTag &      xcls = interaction->ExclTag();
    const InitialState & init = interaction->InitState();
    const Target &       tgt  = init.Tgt();

    const Spline * spl = evg_driver.XSecSpline(interaction);

    if(!xcls.IsCharmEvent()) continue;
 
    if (proc.IsDeepInelastic() && proc.IsWeakCC() && pdg::IsProton(tgt.HitNucPdg())) {
      for(int i=0; i<kNSplineP; i++) { 
          xsdisccp[i] += (spl->Evaluate(e[i]) * (1E+38/units::cm2)); 
      }
    }
    if (proc.IsDeepInelastic() && proc.IsWeakCC() && pdg::IsNeutron(tgt.HitNucPdg())) {
      for(int i=0; i<kNSplineP; i++) { 
          xsdisccn[i] += (spl->Evaluate(e[i]) * (1E+38/units::cm2)); 
      }
    }
    if (proc.IsDeepInelastic() && proc.IsWeakNC() && pdg::IsProton(tgt.HitNucPdg())) {
      for(int i=0; i<kNSplineP; i++) { 
          xsdisncp[i] += (spl->Evaluate(e[i]) * (1E+38/units::cm2)); 
      }
    }
    if (proc.IsDeepInelastic() && proc.IsWeakNC() && pdg::IsNeutron(tgt.HitNucPdg())) {
      for(int i=0; i<kNSplineP; i++) { 
          xsdisncn[i] += (spl->Evaluate(e[i]) * (1E+38/units::cm2)); 
      }
    }
  }
  TGraph * gr_disccp_charm = new TGraph(kNSplineP, e, xsdisccp);
  gr_disccp_charm->SetName("dis_cc_p_charm");
  gr_disccp_charm->SetTitle("GENIE cross section graph");
  topdir->Add(gr_disccp_charm);
  TGraph * gr_disccn_charm = new TGraph(kNSplineP, e, xsdisccn);
  gr_disccn_charm->SetName("dis_cc_n_charm");
  gr_disccn_charm->SetTitle("GENIE cross section graph");
  topdir->Add(gr_disccn_charm);
  TGraph * gr_disncp_charm = new TGraph(kNSplineP, e, xsdisncp);
  gr_disncp_charm->SetName("dis_nc_p_charm");
  gr_disncp_charm->SetTitle("GENIE cross section graph");
  topdir->Add(gr_disncp_charm);
  TGraph * gr_disncn_charm = new TGraph(kNSplineP, e, xsdisncn);
  gr_disncn_charm->SetName("dis_nc_n_charm");
  gr_disncn_charm->SetTitle("GENIE cross section graph");
  topdir->Add(gr_disncn_charm);

  delete [] e;
  delete [] xs;
  delete [] xsresccp;
  delete [] xsresccn;
  delete [] xsresncp;
  delete [] xsresncn; 
  delete [] xsdisccp;
  delete [] xsdisccn;
  delete [] xsdisncp;
  delete [] xsdisncn; 

  topdir->Write();

  if(froot) {
    froot->Close();
    delete froot;
  }
}
//____________________________________________________________________________
void SaveNtupleToRootFile(void)
{
/*
  //-- create cross section ntuple
  //
  double brXSec;
  double brEv;
  bool   brIsQel;
  bool   brIsRes;
  bool   brIsDis;
  bool   brIsCoh;
  bool   brIsImd;
  bool   brIsNuEEl;
  bool   brIsCC;
  bool   brIsNC;
  int    brNucleon;
  int    brQrk;
  bool   brIsSeaQrk;
  int    brRes;
  bool   brCharm;
  TTree * xsecnt = new TTree("xsecnt",dtitle.str().c_str());
  xsecnt->Branch("xsec",  &brXSec,     "xsec/D");
  xsecnt->Branch("e",     &brEv,       "e/D");
  xsecnt->Branch("qel",   &brIsQel,    "qel/O");
  xsecnt->Branch("res",   &brIsRes,    "res/O");
  xsecnt->Branch("dis",   &brIsDis,    "dis/O");
  xsecnt->Branch("coh",   &brIsCoh,    "coh/O");
  xsecnt->Branch("imd",   &brIsImd,    "imd/O");
  xsecnt->Branch("veel",  &brIsNuEEl,  "veel/O");
  xsecnt->Branch("cc",    &brIsCC,     "cc/O");
  xsecnt->Branch("nc",    &brIsNC,     "nc/O");
  xsecnt->Branch("nuc",   &brNucleon,  "nuc/I");
  xsecnt->Branch("qrk",   &brQrk,      "qrk/I");
  xsecnt->Branch("sea",   &brIsSeaQrk, "sea/O");
  xsecnt->Branch("res",   &brRes,      "res/I");
  xsecnt->Branch("charm", &brCharm,    "charm/O");
*/
}
//____________________________________________________________________________
void GetCommandLineArgs(int argc, char ** argv)
{
  LOG("gsplt", pINFO) << "Parsing commad line arguments";

  //input XML file name:
  try {
    LOG("gsplt", pINFO) << "Reading input XML filename";
    gOptXMLFilename = genie::utils::clap::CmdLineArgAsString(argc,argv,'f');
  } catch(exceptions::CmdLineArgParserException e) {
    if(!e.ArgumentFound()) {
      LOG("gsplt", pFATAL) << "Unspecified input XML file!";
      PrintSyntax();
      exit(1);
    }
  }
  // neutrino PDG code:
  try {
    LOG("gevgen", pINFO) << "Reading neutrino PDG code";
    gOptNuPdgCode = genie::utils::clap::CmdLineArgAsInt(argc,argv,'p');
  } catch(exceptions::CmdLineArgParserException e) {
    if(!e.ArgumentFound()) {
      LOG("gevgen", pFATAL) << "Unspecified neutrino PDG code - Exiting";
      PrintSyntax();
      exit(1);
    }
  }
  // target PDG code:
  try {
    LOG("gevgen", pINFO) << "Reading target PDG code";
    gOptTgtPdgCode = genie::utils::clap::CmdLineArgAsInt(argc,argv,'t');
  } catch(exceptions::CmdLineArgParserException e) {
    if(!e.ArgumentFound()) {
      LOG("gevgen", pFATAL) << "Unspecified target PDG code - Exiting";
      PrintSyntax();
      exit(1);
    }
  } 
  //max neutrino energy
  try {
    LOG("gsplt", pINFO) << "Reading neutrino energy";
    gOptNuEnergy = genie::utils::clap::CmdLineArgAsDouble(argc,argv,'e');
  } catch(exceptions::CmdLineArgParserException e) {
    if(!e.ArgumentFound()) {
      LOG("gsplt", pDEBUG)  << "Unspecified Emax - Setting to 100 GeV";
      gOptNuEnergy = 100;
    }
  }
  //output ROOT file name:
  try {
    LOG("gsplt", pINFO) << "Reading output ROOT filename";
    gOptROOTFilename = genie::utils::clap::CmdLineArgAsString(argc,argv,'o');
  } catch(exceptions::CmdLineArgParserException e) {
    if(!e.ArgumentFound()) {
      LOG("gsplt", pDEBUG)  
                      << "Unspecified ROOT file. Splines will not be saved.";
      gOptROOTFilename = "";
    }
  }

  gEmin  = kEmin;
  gEmax  = gOptNuEnergy;
  assert(gEmin<gEmax);

  // print the options you got from command line arguments
  LOG("gsplt", pINFO) << "Command line arguments:";
  LOG("gsplt", pINFO) << "  Input XML file    = " << gOptXMLFilename;
  LOG("gsplt", pINFO) << "  Neutrino PDG code = " << gOptNuPdgCode;
  LOG("gsplt", pINFO) << "  Target PDG code   = " << gOptTgtPdgCode;
  LOG("gsplt", pINFO) << "  Max neutrino E    = " << gOptNuEnergy;
}
//____________________________________________________________________________
void PrintSyntax(void)
{
  LOG("gsplt", pNOTICE)
      << "\n\n" << "Syntax:" << "\n"
      << "   gsplt -f xml_file -p neutrino_pdg -t target_pdg"
      << " [-e emax] [-o output_root_file]";
}
//____________________________________________________________________________
