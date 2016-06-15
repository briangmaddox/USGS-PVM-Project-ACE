#ifndef PROJECTOR_H
#define PROJECTOR_H

#include <iostream>
#include <vector>
#include <algorithm>
#include "ImageLib/DOQImageIFile.h"
#include "ImageLib/GeoTIFFImageOFile.h"
#include "ImageLib/GeoTIFFImageIFile.h"
#include "ImageLib/CacheManager.h"
#include "ProjectionIO/ProjectionReader.h"
#include "ProjectionIO/ProjectionWriter.h"
#include "ProjectionMesh/ProjectionMesh.h"
#include "DRect.h"
#include "ProjectorException.h" //needs to be implemented
#include "ProjectionParams.h"

//Reprojection object, converts one file to another
class Projector
{
public:
  //Constructors and Destructor
  Projector();
  Projector(std::string & ininfile, 
            ProjLib::Projection * inoutproject,
            const std::string & inoutfile = std::string("out.tif"));
  Projector(std::string & ininfile, 
            const ProjectionParams& inParams,
            const std::string & inoutfile = std::string("out.tif"));
  virtual ~Projector();

  //sets
  virtual void setInputFile(std::string & ininfile) throw(); 
  void setOutputProjection(ProjLib::Projection * inoutproject) throw();
  void setOutputProjection(const ProjectionParams& inParams) throw();
  void setOutputFileName(const std::string& inoutfile) throw();
  void setPmeshName(const int & inpmeshname) throw();
  void setPmeshSize(const long int & inpmeshsize) throw();
  void setOutputScale(const MathLib::Point & innewscale) throw(); 
  
  //This function allows user to have the same geographical output scale as input
  void setSameScale(bool insamescale) throw();
  
  //gets
  std::string getInputFile() const throw();
  ProjLib::Projection * getOutputProjection() const throw();
  ProjectionParams getOutputProjectionParams() const throw();
  int getPmeshName() const throw();
  int getPmeshSize() const throw();
  
  //main function which runs the projection
  virtual bool 
    project(void (*status)(long int, long int) = NULL) 
    throw(ProjectorException);

protected:
  //setup input file and input projections
  bool setupInput(std::string & ininfile) throw();
  
  //setup the output file
  bool setupOutput(std::string & inoutfile) throw();

  //setup the pmesh 
  PmeshLib::ProjectionMesh * setupForwardPmesh() throw();
  PmeshLib::ProjectionMesh * setupReversePmesh() throw();

  //getExtents function gets the new bounding rectangle for the new image
  bool getExtents(PmeshLib::ProjectionMesh * pmesh) throw();
  
  //get projectionparams from projection
  ProjectionParams getParams(ProjLib::Projection * proj) const throw();

  //set the projection from projectionParams
  ProjLib::Projection * 
    SetProjection(const ProjectionParams & inParams) throw();
  
  //get the datum from a string
  ProjLib::DATUM GetDatum(std::string indatum) throw();
  
  //get the unit from a 
  ProjLib::UNIT GetUnit(std::string inunit) throw();
  
  //convert from one scale to another
  MathLib::Point GetConvertedScale(double oldwidth, double oldheight, 
                                   DRect outRec) throw ();
  //convert to Packed Degree Minute seconds
  double ConvertToDMS(double degrees) const throw();

  //convert from DMS
  double ConvertFromDMS(double dms) const throw();
  
  //get minMax of two vecors
  void getMinMax(std::vector<double>& array, double& min, double& max)
    throw();

  //get SameScale converts linear units to perserve linear length (note:
  //if the input scale is angluar then the lat and long at equator are used
  MathLib::Point getSameScale(MathLib::Point inoldscale, ProjLib::Projection * in,
	  ProjLib::Projection * out) const throw();

  ProjIOLib::ProjectionReader reader;
  ProjIOLib::ProjectionWriter writer;
  Projection * fromprojection, * toprojection;
  USGSImageLib::ImageIFile * infile;
  USGSImageLib::ImageOFile* out;                //the output file  
  USGSImageLib::CacheManager* cache;            //cache

    
  //metrics
  long int oldheight, oldwidth, newheight, newwidth;
  DRect inRect, outRect;
  MathLib::Point oldscale, newscale;
  int pmeshsize;                                //pmesh metrics
  int pmeshname;
  int photo, spp, bps;
  std::string outfile;                          //outputfilename
  ProjectionParams Params;
  bool samescale;
};

#endif





