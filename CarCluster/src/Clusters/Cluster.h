// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#ifndef CLUSTER_GENERIC
#define CLUSTER_GENERIC

#include <Arduino.h>
#include "../Games/GameSimulation.h"

class Cluster {
  public:
  virtual void updateWithGame(GameState& game) = 0;
  //virtual static ClusterConfiguration clusterConfigForUserConfig(UserConfiguration& userConfig) = 0;
};

#endif