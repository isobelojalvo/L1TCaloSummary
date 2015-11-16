#include <iostream>
#include <iomanip>
#include <vector>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

using namespace std;

#include "UCTSummaryCard.hh"

#include "UCTRegionExtended.hh"
#include "UCTObject.hh"

#include "L1Trigger/L1TCaloLayer1/src/UCTLayer1.hh"
#include "L1Trigger/L1TCaloLayer1/src/UCTCrate.hh"
#include "L1Trigger/L1TCaloLayer1/src/UCTCard.hh"
#include "L1Trigger/L1TCaloLayer1/src/UCTRegion.hh"

#include "L1Trigger/L1TCaloLayer1/src/UCTGeometry.hh"

UCTSummaryCard::UCTSummaryCard(const UCTLayer1* in) : uctLayer1(in) {
  extendedRegions.clear();
  for(int iEta = -NRegionsInCard; iEta <= NRegionsInCard; iEta++) {
    if(iEta == 0) continue;
    for(uint32_t iPhi = 1; iPhi <= MaxUCTRegionsPhi; iPhi++) {
      UCTRegionIndex regionIndex(iEta, iPhi);
      UCTRegionExtended* extendedRegion = new UCTRegionExtended(uctLayer1->getRegion(regionIndex));
      extendedRegions.push_back(extendedRegion);
    }
  }
  // FIXME: phi = 0 is probably not correct
  sinPhi[0] = 0;
  cosPhi[0] = 1;
  for(int iPhi = 1; iPhi <= 72; iPhi++) {
    sinPhi[iPhi] = sin((((double) iPhi / (double) 72) * 2 * 3.1415927) - 0.043633);
    cosPhi[iPhi] = cos((((double) iPhi / (double) 72) * 2 * 3.1415927) - 0.043633);
  }
}

UCTSummaryCard::~UCTSummaryCard() {
}

bool UCTSummaryCard::process() {
  // First initiate extended region processing
  uint32_t iReg = 0;
  for(int iEta = -NRegionsInCard; iEta <= NRegionsInCard; iEta++) {
    if(iEta == 0) break;
    for(uint32_t iPhi = 1; iPhi <= MaxUCTRegionsPhi; iPhi++) {
      UCTRegionExtended* extendedRegion = extendedRegions[iReg];
      // Set the normal region information and the process more
      UCTRegionIndex regionIndex(iEta, iPhi);
      extendedRegion->processMore(uctLayer1->getRegion(regionIndex));
      // Next region in list
      iReg++;
    }
  }
  // Then do the summary card processing
  uint32_t etValue = 0;
  uint32_t htValue = 0;
  int sumEx = 0;
  int sumEy = 0;
  int sumHx = 0;
  int sumHy = 0;
  for(int iEta = -NRegionsInCard; iEta <= NRegionsInCard; iEta++) {
    if(iEta == 0) break;
    for(uint32_t iPhi = 1; iPhi <= MaxUCTRegionsPhi; iPhi++) {
      UCTRegionIndex regionIndex(iEta, iPhi);
      processRegion(regionIndex);
      const UCTRegion* uctRegion = uctLayer1->getRegion(regionIndex);
      uint32_t et = uctRegion->et();
      int hitCaloPhi = uctRegion->hitCaloPhi();
      if(iEta == -NRegionsInCard) {
	sumEx += ((int) ((double) et) * cosPhi[hitCaloPhi]);
	sumEy += ((int) ((double) et) * sinPhi[hitCaloPhi]);
	etValue += et;
	if(et > 10) {
	  sumHx += ((int) ((double) et) * cosPhi[hitCaloPhi]);
	  sumHy += ((int) ((double) et) * sinPhi[hitCaloPhi]);
	  htValue += et;
	}
      }
    }
  }
  uint32_t metSquare = sumEx * sumEx + sumEy * sumEy;
  uint32_t metValue = sqrt((double) metSquare);
  double metPhi = (atan2(sumEy, sumEx) * 180. / 3.1415927) + 180.; // FIXME - phi=0 may not be correct
  int metIPhi = (int) ( 72. * (metPhi / 360.));
  uint32_t mhtSquare = sumHx * sumHx + sumHy * sumHy;
  uint32_t mhtValue = sqrt((double) mhtSquare);
  double mhtPhi = (atan2(sumHy, sumHx) * 180. / 3.1415927) + 180.; // FIXME - phi=0 may not be correct
  int mhtIPhi = (int) ( 72. * (mhtPhi / 360.));

  ET = new UCTObject(UCTObject::ET, etValue, 0, metIPhi, 0, 0, 0);
  HT = new UCTObject(UCTObject::HT, htValue, 0, mhtIPhi, 0, 0, 0);
  MET = new UCTObject(UCTObject::MET, metValue, 0, metIPhi, 0, 0, 0);
  MHT = new UCTObject(UCTObject::MHT, mhtValue, 0, mhtIPhi, 0, 0, 0); // FIXME - cheating for now - requires more work

  // Then sort the candidates for output usage
  emObjs.sort();
  isoEMObjs.sort();
  tauObjs.sort();
  isoTauObjs.sort();
  centralJetObjs.sort();
  forwardJetObjs.sort();
  // Cool we never fail :)
  return true;
}

bool UCTSummaryCard::processRegion(UCTRegionIndex center) {

  UCTGeometryExtended g;
  UCTRegionIndex nIndex = g.getUCTRegionNorth(center);
  UCTRegionIndex sIndex = g.getUCTRegionSouth(center);
  UCTRegionIndex eIndex = g.getUCTRegionEast(center);
  UCTRegionIndex wIndex = g.getUCTRegionWest(center);
  UCTRegionIndex neIndex = g.getUCTRegionNE(center);
  UCTRegionIndex nwIndex = g.getUCTRegionNW(center);
  UCTRegionIndex seIndex = g.getUCTRegionSE(center);
  UCTRegionIndex swIndex = g.getUCTRegionSW(center);

  UCTRegionExtended cRegion(uctLayer1->getRegion(center));
  UCTRegionExtended nRegion(uctLayer1->getRegion(nIndex));
  UCTRegionExtended sRegion(uctLayer1->getRegion(sIndex));
  UCTRegionExtended eRegion(uctLayer1->getRegion(eIndex));
  UCTRegionExtended wRegion(uctLayer1->getRegion(wIndex));
  UCTRegionExtended neRegion(uctLayer1->getRegion(neIndex));
  UCTRegionExtended nwRegion(uctLayer1->getRegion(nwIndex));
  UCTRegionExtended seRegion(uctLayer1->getRegion(seIndex));
  UCTRegionExtended swRegion(uctLayer1->getRegion(swIndex));

  uint32_t cET = cRegion.et();
  uint32_t nET = nRegion.et();
  uint32_t sET = sRegion.et();
  uint32_t eET = eRegion.et();
  uint32_t wET = wRegion.et();
  uint32_t neET = neRegion.et();
  uint32_t seET = seRegion.et();
  uint32_t nwET = nwRegion.et();
  uint32_t swET = swRegion.et();

  uint32_t et3x3 = cET + nET + nwET + wET + swET + sET + seET + eET + neET;

  uint32_t pileup = 0; // FIXME: This should be looked up for the region using the calculated PUM 

  uint32_t JetSeed = 10; // FIXME: This should be a configurable parameter

  // Jet - a 3x3 object with center greater than a seed and all neighbors

  if(cET >= nET && cET >= nwET && cET >= wET && cET >= swET &&
     cET >  sET && cET >  seET && cET >  eET && cET >  neET &&
     cET > JetSeed) {
    uint32_t jetET = et3x3 - pileup;
    centralJetObjs.push_back(new UCTObject(UCTObject::jet, jetET, cRegion.hitCaloEta(), cRegion.hitCaloPhi(), pileup, 0, et3x3));
  }

  // tau Object - a single region or a 2-region sum, where the neighbor with lower ET is located using matching hit calo towers

  if(cRegion.isTauLike()) {
    UCTTowerIndex cHitTower = cRegion.hitTowerIndex();
    uint32_t tauET = 0;
    uint32_t isolation = et3x3;
    if(g.isEdgeTower(cHitTower)) {
      tauET = cRegion.et();
    }
    else {
      UCTTowerIndex nHitTower = nRegion.hitTowerIndex();
      if(g.areNeighbors(cHitTower, nHitTower)) {
	tauET = cRegion.et() + nRegion.et();
      }
      else {
	UCTTowerIndex sHitTower = sRegion.hitTowerIndex();
	if(g.areNeighbors(cHitTower, sHitTower)) {
	  tauET = cRegion.et() + sRegion.et();
	}
	else {
	  UCTTowerIndex wHitTower = wRegion.hitTowerIndex();
	  if(g.areNeighbors(cHitTower, wHitTower)) {
	    tauET = cRegion.et() + wRegion.et();
	  }
	  else {
	    UCTTowerIndex eHitTower = eRegion.hitTowerIndex();
	    if(g.areNeighbors(cHitTower, eHitTower)) {
	      tauET = cRegion.et() + eRegion.et();
	    }
	  }
	}
      }
    }
    double IsolationFactor = 0.3; // FIXME: This should be a configurable parameter
    isolation = et3x3 - tauET - pileup;
    tauObjs.push_back(new UCTObject(UCTObject::tau, tauET, cRegion.hitCaloEta(), cRegion.hitCaloPhi(), pileup, isolation, et3x3));
    if(isolation < ((uint32_t) (IsolationFactor * (double) tauET))) {
      isoTauObjs.push_back(new UCTObject(UCTObject::isoTau, tauET, cRegion.hitCaloEta(), cRegion.hitCaloPhi(), pileup, isolation, et3x3));
    }
  }
  
  return true;
}

bool UCTSummaryCard::clearEvent() {
  return true;
}

void UCTSummaryCard::print() {
  if(cardSummary > 0)
    std::cout << "UCTSummaryCard: result = " << cardSummary << std::endl;
}
