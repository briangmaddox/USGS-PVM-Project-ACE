#ifndef PROJECTIONPARAMS_H
#define PROJECTIONPARAMS_H

#include "ProjectionLib/ProjectionTypes.h"


class ProjectionParams
{
public:
  //Constructor
  ProjectionParams();

  //public data
  ProjLib::PROJSYS projtype;
  ProjLib::DATUM datum;
  ProjLib::UNIT unit;
  double StdParallel1;
  double StdParallel2;
  double NatOriginLong;
  double NatOriginLat;
  double FalseOriginLong;
  double FalseOriginLat;
  double FalseOriginEasting;
  double FalseOriginNorthing;
  double CenterLong;
  double CenterLat;
  double CenterEasting;
  double CenterNorthing;
  double ScaleAtNatOrigin;
  double AzimuthAngle;
  double StraightVertPoleLong;
  int zone;
  double FalseEasting;
  double FalseNorthing;
};

#endif
