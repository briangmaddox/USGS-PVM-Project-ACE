#ifndef PVMPROJECTOR_H_
#define PVMPROJECTOR_H_



#include "ProjectorStitcher.h"
#include "Projector.h"
#include "MessageTags.h"
#include <pvm3.h>

//The master pvm projector
class PvmProjector : public Projector
{
 public:
  //Constructor and Destructor
  PvmProjector();
  virtual ~PvmProjector();
  
  //allows user to set the number of slaves spawned
  void setNumberOfSlaves(const unsigned int & innumslaves) throw();
  unsigned int getNumberOfSlaves() const throw();
  
  //overloaded to save the filename
  virtual void setInputFile(std::string & ininfile) throw();
  
  //main function which runs the projection
  virtual bool 
    project(void (*status)(long int, long int) = NULL) 
    throw(ProjectorException);
  
protected:
  
  //sendSlaveSetup function does all the pvm packing to send to the slave
  //for setup
  bool sendSlaveSetup() throw();

  //unpackScanline unpacks a scanline from the slave and passes it to the
  //stitcher
  void unpackScanline(ProjectorStitcher & stitcher) throw();
  unsigned int numofslaves;          //the number of slaves
  int * childtid;                    //the child ids
  std::string inputfilename;
};

#endif
  
