#ifndef PVMPROJECTORSLAVE_H
#define PVMPROJECTORSLAVE_H



#include "Projector.h"
#include "MessageTags.h"
#include <pvm3.h>

//Slave pvm projector
class PvmProjectorSlave : public Projector
{
 public:
  //Constructor and Destructor
  PvmProjectorSlave();
  virtual ~PvmProjectorSlave();

  //connect function attempts to connect to the master 
  //and runs the projection
  bool connect() throw();

 protected:
  //unpackSetup function unpacks setup info from the master
  void unpackSetup() throw();

  int mastertid, mytid;  //pvm name
};

#endif
