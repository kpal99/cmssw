// -*- C++ -*-
//
// Package:    Test/WriteL1TriggerObjectsTxt
// Class:      WriteL1TriggerObjectsTxt
//
/**\class WriteL1TriggerObjectsTxt WriteL1TriggerObjectsTxt.cc Test/WriteL1TriggerObjectsTxt/plugins/WriteL1TriggerObjectsTxt.cc

 Description: [one line class summary]

 Implementation:
     [Notes on implementation]
*/
//
// Original Author:  Aleko Khukhunaishvili
//         Created:  Fri, 21 Jul 2017 08:25:18 GMT
//
//

// system include files
#include <memory>
#include <fstream>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "CalibFormats/HcalObjects/interface/HcalCalibrations.h"
#include "CalibFormats/HcalObjects/interface/HcalDbService.h"
#include "CalibFormats/HcalObjects/interface/HcalDbRecord.h"
#include "CondFormats/HcalObjects/interface/HcalL1TriggerObjects.h"
#include "CondFormats/HcalObjects/interface/HcalL1TriggerObject.h"
#include "CalibCalorimetry/HcalAlgos/interface/HcalDbASCIIIO.h"
#include "Geometry/CaloTopology/interface/HcalTopology.h"

class WriteL1TriggerObjectsTxt : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
  explicit WriteL1TriggerObjectsTxt(const edm::ParameterSet&);
  ~WriteL1TriggerObjectsTxt() override;

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  void analyze(const edm::Event&, const edm::EventSetup&) override;

  template <typename T>
  void fillL1TrgObjsCollection(std::unique_ptr<HcalL1TriggerObjects>& l1TrgObjsCol,
                               const HcalDbService* conditions,
                               const HcalTopology* topo,
                               T cell);

  std::string tagName_;
  edm::ESGetToken<HcalDbService, HcalDbRecord> tok_dbservice_;
};

WriteL1TriggerObjectsTxt::WriteL1TriggerObjectsTxt(const edm::ParameterSet& iConfig)
    : tagName_(iConfig.getParameter<std::string>("TagName")),
      tok_dbservice_(esConsumes<HcalDbService, HcalDbRecord>()) {}

WriteL1TriggerObjectsTxt::~WriteL1TriggerObjectsTxt() {}

template <typename T>
void WriteL1TriggerObjectsTxt::fillL1TrgObjsCollection(std::unique_ptr<HcalL1TriggerObjects>& l1TrgObjsCol,
                                                       const HcalDbService* conditions,
                                                       const HcalTopology* topo,
                                                       T cell) {
  const HcalCalibrations calibrations = conditions->getHcalCalibrations(cell);

  float gain = 0.0;
  float ped = 0.0;

  for (auto i : {0, 1, 2, 3}) {
    gain += calibrations.LUTrespcorrgain(i);
    ped += calibrations.effpedestal(i);
  }

  gain /= 4.;
  ped /= 4.;

  const HcalChannelStatus* channelStatus = conditions->getHcalChannelStatus(cell);
  uint32_t status = channelStatus->getValue();
  HcalL1TriggerObject l1object(cell, ped, gain, status);
  l1TrgObjsCol->setTopo(topo);
  l1TrgObjsCol->addValues(l1object);
}

void WriteL1TriggerObjectsTxt::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
  using namespace edm;

  const HcalDbService* conditions = &iSetup.getData(tok_dbservice_);

  const HcalLutMetadata* metadata = conditions->getHcalLutMetadata();
  const HcalTopology* topo = metadata->topo();

  std::unique_ptr<HcalL1TriggerObjects> HcalL1TrigObjCol(new HcalL1TriggerObjects);

  for (const auto& id : metadata->getAllChannels()) {
    if (id.det() == DetId::Hcal and topo->valid(id)) {
      HcalDetId cell(id);
      HcalSubdetector subdet = cell.subdet();
      if (subdet != HcalBarrel and subdet != HcalEndcap and subdet != HcalForward)
        continue;

      fillL1TrgObjsCollection(HcalL1TrigObjCol, conditions, topo, cell);

    } else if (id.det() == DetId::Calo && id.subdetId() == HcalZDCDetId::SubdetectorId) {
      HcalZDCDetId cell(id.rawId());

      if (cell.section() != HcalZDCDetId::EM && cell.section() != HcalZDCDetId::HAD &&
          cell.section() != HcalZDCDetId::LUM)
        continue;

      fillL1TrgObjsCollection(HcalL1TrigObjCol, conditions, topo, cell);
    }
  }

  HcalL1TrigObjCol->setTagString(tagName_);
  HcalL1TrigObjCol->setAlgoString("TP algo determined by HcalTPChannelParameter auxi params");
  std::string outfilename = "Gen_L1TriggerObjects_";
  outfilename += tagName_;
  outfilename += ".txt";
  std::ofstream of(outfilename.c_str());
  HcalDbASCIIIO::dumpObject(of, *HcalL1TrigObjCol);
}

void WriteL1TriggerObjectsTxt::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);
}

DEFINE_FWK_MODULE(WriteL1TriggerObjectsTxt);
