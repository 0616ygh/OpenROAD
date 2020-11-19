/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>
#include <set>

#include "opendb/db.h"
#include "sta/Liberty.hh"
#include "GRoute.h"

namespace ord {
class OpenRoad;
}

namespace odb {
class dbDatabase;
class dbTech;
class dbBlock;
class dbDatabase;
}  // namespace odb

namespace sta {
class dbSta;
class dbNetwork;
}

namespace FastRoute {

class FastRouteCore;
class AntennaRepair;
class Grid;
class Pin;
class Net;
class Netlist;
class RoutingTracks;
class RoutingLayer;
class SteinerTree;
class RoutePt;
struct NET;

struct RegionAdjustment
{
  odb::Rect region;
  int layer;
  float adjustment;
  
  RegionAdjustment(int minX, int minY, 
                   int maxX, int maxY,
                   int l, float adjst);
  odb::Rect getRegion() { return region; }
  int getLayer() { return layer; }
  float getAdjustment() { return adjustment; }
};

class GlobalRouter
{
 public:
  struct ADJUSTMENT_
  {
    int firstX;
    int firstY;
    int firstLayer;
    int finalX;
    int finalY;
    int finalLayer;
    int edgeCapacity;
  };

  struct ROUTE_
  {
    int gridCountX;
    int gridCountY;
    int numLayers;
    std::vector<int> verticalEdgesCapacities;
    std::vector<int> horizontalEdgesCapacities;
    std::vector<int> minWireWidths;
    std::vector<int> minWireSpacings;
    std::vector<int> viaSpacings;
    long gridOriginX;
    long gridOriginY;
    long tileWidth;
    long tileHeight;
    int blockPorosity;
    int numAdjustments;
    std::vector<ADJUSTMENT_> adjustments;
  };

  GlobalRouter() = default;
  ~GlobalRouter();
  void init(ord::OpenRoad* openroad);
  void init();
  void clear();

  void setAdjustment(const float adjustment);
  void setMinRoutingLayer(const int minLayer);
  void setMaxRoutingLayer(const int maxLayer);
  void setUnidirectionalRoute(const bool unidirRoute);
  void setAlpha(const float alpha);
  void setPitchesInTile(const int pitchesInTile);
  void setSeed(unsigned seed);
  unsigned getDbId();
  void addLayerAdjustment(int layer, float reductionPercentage);
  void addRegionAdjustment(int minX,
                           int minY,
                           int maxX,
                           int maxY,
                           int layer,
                           float reductionPercentage);
  void setLayerPitch(int layer, float pitch);
  void addAlphaForNet(char* netName, float alpha);
  void setVerbose(const int v);
  void setOverflowIterations(int iterations);
  void setGridOrigin(long x, long y);
  void setPDRevForHighFanout(int pdRevForHighFanout);
  void setAllowOverflow(bool allowOverflow);
  void setReportCongestion(char* congestFile);
  void setMacroExtension(int macroExtension);
  void printGrid();

  // flow functions
  void writeGuides(const char* fileName);
  void startFastRoute();
  void estimateRC();
  void runFastRoute(bool onlySignal = false);
  NetRouteMap& getRoutes() { return _routes; }
  bool haveRoutes() const { return !_routes.empty(); }

  // repair antenna public functions
  void repairAntennas(sta::LibertyPort* diodePort);
  void addDirtyNet(odb::dbNet* net);

  // congestion drive replace functions
  ROUTE_ getRoute();

  // estimate_rc functions
  void getLayerRC(unsigned layerId, float& r, float& c);
  void getCutLayerRes(unsigned belowLayerId, float& r);
  float dbuToMeters(unsigned dbu);

  // route clock nets public functions
  void routeClockNets();

protected:
  // Net functions
  int getNetCount() const;
  void reserveNets(size_t net_count);
  Net* addNet(odb::dbNet* db_net, std::vector<Net>* nets);
  int getMaxNetDegree();
  friend class AntennaRepair;

 private:
  void makeComponents();
  void deleteComponents();
  void clearFlow();
  void applyAdjustments();
  // main functions
  void initCoreGrid();
  void initRoutingLayers();
  void initRoutingTracks();
  void setCapacities();
  void setSpacingsAndMinWidths();
  void initializeNets(std::vector<Net>* nets);
  void computeGridAdjustments();
  void computeTrackAdjustments();
  void computeUserGlobalAdjustments();
  void computeUserLayerAdjustments();
  void computeRegionAdjustments(const odb::Rect& region,
                                int layer,
                                float reductionPercentage);
  void computeObstaclesAdjustments();
  void computeWirelength();
  std::vector<Pin*> getAllPorts();


  // aux functions
  void findPins(Net& net);
  void findPins(Net& net, std::vector<RoutePt>& pinsOnGrid);
  RoutingLayer getRoutingLayerByIndex(int index);
  RoutingTracks getRoutingTracksByIndex(int layer);
  void addGuidesForLocalNets(odb::dbNet* db_net, GRoute &route);
  void addGuidesForPinAccess(odb::dbNet* db_net, GRoute &route);
  void addRemainingGuides(NetRouteMap& routes, std::vector<Net> *nets);
  void connectPadPins(NetRouteMap& routes);
  void mergeBox(std::vector<odb::Rect>& guideBox);
  odb::Rect globalRoutingToBox(const GSegment& route);
  bool segmentsConnect(const GSegment& seg0,
                       const GSegment& seg1,
                       GSegment& newSeg,
                       const std::map<RoutePt, int>& segsAtPoint);
  void mergeSegments();
  void mergeSegments(GRoute& route);
  bool pinOverlapsWithSingleTrack(const Pin& pin, odb::Point& trackPosition);
  GSegment createFakePin(Pin pin, odb::Point& pinPosition, RoutingLayer layer);
  odb::Point findFakePinPosition(Pin &pin);
  void initAdjustments();
  void initPitches();
  odb::Point getRectMiddle(odb::Rect& rect);
  NetRouteMap findRouting(std::vector<Net> *nets);

  // check functions
  void checkPinPlacement();

  // antenna functions
  void addLocalConnections(NetRouteMap& routes);
  void mergeResults(NetRouteMap& routes);
  void getPreviousCapacities(int previousMinLayer, int previousMaxLayer);
  void restorePreviousCapacities(int previousMinLayer, int previousMaxLayer);
  void removeDirtyNetsUsage();

  // db functions
  void initGrid(int maxLayer);
  void initRoutingLayers(std::vector<RoutingLayer>& routingLayers);
  void initRoutingTracks(std::vector<RoutingTracks>& allRoutingTracks,
                         int maxLayer,
                         std::vector<float> layerPitches);
  void computeCapacities(int maxLayer, std::vector<float> layerPitches);
  void computeSpacingsAndMinWidth(int maxLayer);
  void initNetlist(std::vector<Net>* nets);
  void addNets(std::set<odb::dbNet*>& db_nets, std::vector<Net>* nets);
  Net* getNet(odb::dbNet* db_net);
  void initObstacles();
  void findLayerExtensions(std::vector<int>& layerExtensions);
  void findObstructions(odb::Rect& dieArea);
  void findInstancesObstacles(odb::Rect& dieArea, const std::vector<int>& layerExtensions);
  void findNetsObstacles(odb::Rect& dieArea);
  int computeMaxRoutingLayer();
  std::set<int> findTransitionLayers(int maxRoutingLayer);
  std::map<int, odb::dbTechVia*> getDefaultVias(int maxRoutingLayer);
  void makeItermPins(Net* net, odb::dbNet* db_net, odb::Rect& dieArea);
  void makeBtermPins(Net* net, odb::dbNet* db_net, odb::Rect& dieArea);
  void initClockNets();
  bool isClkTerm(odb::dbITerm *iterm, sta::dbNetwork *network);
  bool clockHasLeafITerm(odb::dbNet* db_net);
  void setSelectedMetal(int metal) { selectedMetal = metal; }

  ord::OpenRoad* _openroad;
  // Objects variables
  FastRouteCore* _fastRoute = nullptr;
  odb::Point* _gridOrigin = nullptr;
  NetRouteMap _routes;

  std::vector<Net> *_nets;
  std::vector<Net> *_clockNets;
  std::vector<Net> *_signalNets;
  std::vector<Net> *_antennaNets;
  std::map<odb::dbNet*, Net*> _db_net_map;
  Grid* _grid = nullptr;
  std::vector<RoutingLayer>* _routingLayers = nullptr;
  std::vector<RoutingTracks>* _allRoutingTracks = nullptr;

  // Flow variables
  std::string _congestFile;
  float _adjustment;
  int _minRoutingLayer;
  int _maxRoutingLayer;
  bool _unidirectionalRoute;
  int _fixLayer;
  const int _selectedMetal = 3;
  const float transitionLayerAdjust = 0.6;
  const int _gcellsOffset = 2;
  int _overflowIterations;
  int _pdRevForHighFanout;
  bool _allowOverflow;
  bool _reportCongest;
  std::vector<int> _vCapacities;
  std::vector<int> _hCapacities;
  unsigned _seed;
  int _macroExtension;

  // Layer adjustment variables
  std::vector<float> _adjustments;

  // Region adjustment variables
  std::vector<RegionAdjustment> _regionAdjustments;

  // Pitches variables
  std::vector<float> _layerPitches;

  // Clock net routing variables
  bool _pdRev;
  float _alpha;
  int _verbose;
  std::map<std::string, float> _netsAlpha;
  int _minLayerForClock = -1;
  int _maxLayerForClock = -2;

  // Antenna variables
  int*** oldHUsages;
  int*** oldVUsages;

  // temporary for congestion driven replace
  int _numAdjusts = 0;

  // Variables for PADs obstacles handling
  std::map<odb::dbNet*, std::vector<GSegment>> _padPinsConnections;

  // db variables
  sta::dbSta* _sta;
  int selectedMetal = 3;
  odb::dbDatabase* _db = nullptr;
  odb::dbBlock* _block;

  std::set<odb::dbNet*> _dirtyNets;
};

std::string getITermName(odb::dbITerm* iterm);
Net *
getNet(NET* net);

}  // namespace FastRoute
