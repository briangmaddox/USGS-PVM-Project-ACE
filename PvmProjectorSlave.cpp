#ifndef PVMPROJECTORSLAVE_CPP
#define PVMPROJECTORSLAVE_CPP


#include "PvmProjectorSlave.h"

//********************************************************
PvmProjectorSlave::PvmProjectorSlave() : Projector(),
mastertid(0), mytid(0)
{
}

//********************************************************
PvmProjectorSlave::~PvmProjectorSlave()
{
}

//********************************************************
bool PvmProjectorSlave::connect() throw()
{
  double x, y;                          //temp projecting vars
  int _x, _y;                           //actual image xy
  long int currenty, xcounter;          //current line and counter
  unsigned char * scanline=NULL;        //output scanline
  const unsigned char * inscanline = NULL;    //input scaline
  int sppcounter;                       // spp counter
  int bufferid, len, tag, temptid;      //pvm buffer info
  PmeshLib::ProjectionMesh * pmesh = NULL;     //projection mesh
  
  try
  {
   
    mastertid = pvm_parent();           //get the parent node
    mytid = pvm_mytid();                //get my tid
  
    if (mastertid < 0)
    {
      return false;
    }
    
    
    pvm_initsend(PvmDataDefault);       //init the send buffer
    pvm_send(mastertid, SETUP_MSG);     //send a setup request
  
    bufferid = pvm_recv(-1,-1);         //receive any
    pvm_bufinfo(bufferid, &len, &tag, &temptid); //get info
  
    if (tag != SETUP_MSG)               //check for right tag
      return false;
  
    unpackSetup();                      //unpack the setup info
  
    if (pmesh)
    {
      delete pmesh;
      pmesh = NULL;
      pmesh = setupReversePmesh();        //setup the reverse pmesh
    }
  
    if (!(scanline = new (std::nothrow) unsigned char [newwidth*spp]))
      throw std::bad_alloc();
    

    //proccess messages
    while (tag != EXIT_MSG)
    {
      pvm_upklong(&currenty, 1, 1);
      
      for (xcounter = 0; xcounter < newwidth; xcounter++)
      {
        x = outRect.left + newscale.x * xcounter;
        y = outRect.top  - newscale.y * currenty;
        
        //now get the 
        //reverse projected value
        if (pmesh)
          pmesh->projectPoint(x, y);
        else
        {
          toprojection->projectToGeo(x, y, y, x);
          fromprojection->projectFromGeo(y, x, x, y);
        }
        _x = static_cast<long int>((x - inRect.left) / 
                                   (oldscale.x) + 0.5);
        _y = static_cast<long int>((inRect.top - y) / 
                                   (oldscale.y) + 0.5);
        
        if ((_x >= oldwidth) || (_x < 0) || (_y >= oldheight) 
            || (_y < 0))
        {
          for (sppcounter = 0; sppcounter < spp; sppcounter++)
            scanline[xcounter*spp + sppcounter] = 0;
        }
        else
        {
          inscanline = cache->getRawScanline(_y);
          for (sppcounter = 0; sppcounter < spp; sppcounter++)
            scanline[xcounter*spp + sppcounter ] = 
              inscanline[_x*spp + sppcounter];
        }
        
      }
      //pack the scanline and send it
      pvm_initsend(PvmDataDefault);
      pvm_pklong(&currenty, 1, 1);
      pvm_pkbyte(reinterpret_cast<char*>(scanline),newwidth*spp, 1);
      pvm_send(mastertid, WORK_MSG);
      
      bufferid = pvm_recv(-1,-1);         //receive any
      pvm_bufinfo(bufferid, &len, &tag, &temptid); //get info
    }
      
    delete [] scanline;
    delete pmesh;
    delete toprojection;
    toprojection = NULL;
    pmesh = NULL;
    return true;
  }
  catch(...)
  {
    delete pmesh;
    delete toprojection;
    toprojection = NULL;
    pmesh = NULL;
    delete scanline;
    return false;
  }
}

//*********************************************************
void PvmProjectorSlave::unpackSetup() throw()
{
  char tempbuffer[100];
  std::string inputfilename;
  

  try
  {
    pvm_upkstr(tempbuffer);
    inputfilename = tempbuffer;
   
    pvm_upklong(&newheight, 1, 1);
    pvm_upklong(&newwidth, 1, 1);
    pvm_upkdouble(&newscale.x, 1, 1);
    pvm_upkdouble(&newscale.y, 1, 1);
    pvm_upkdouble(&outRect.left, 1, 1);
    pvm_upkdouble(&outRect.top, 1, 1);
    

    //pack pmesh info
    pvm_upkint(&pmeshsize, 1, 1);
    pvm_upkint(&pmeshname, 1, 1);
    
    //pack the projection parameters
    pvm_upkint(reinterpret_cast<int *>(&Params.projtype),1,1);
    pvm_upkint(reinterpret_cast<int *>(&Params.datum), 1, 1);
    pvm_upkint(reinterpret_cast<int *>(&Params.unit), 1, 1);
    pvm_upkdouble(&Params.StdParallel1, 1, 1);
    pvm_upkdouble(&Params.StdParallel2, 1, 1);
    pvm_upkdouble(&Params.NatOriginLong, 1,1);
    pvm_upkdouble(&Params.NatOriginLat, 1, 1);
    pvm_upkdouble(&Params.FalseOriginLong, 1, 1);
    pvm_upkdouble(&Params.FalseOriginLat, 1, 1);
    pvm_upkdouble(&Params.FalseOriginEasting, 1, 1);
    pvm_upkdouble(&Params.FalseOriginNorthing, 1, 1);
    pvm_upkdouble(&Params.CenterLong, 1, 1);
    pvm_upkdouble(&Params.CenterLat, 1, 1);
    pvm_upkdouble(&Params.CenterEasting, 1, 1);
    pvm_upkdouble(&Params.CenterNorthing, 1, 1);
    pvm_upkdouble(&Params.ScaleAtNatOrigin, 1, 1);
    pvm_upkdouble(&Params.AzimuthAngle, 1, 1);
    pvm_upkdouble(&Params.StraightVertPoleLong, 1, 1);
    pvm_upkint(&Params.zone, 1, 1);
    pvm_upkdouble(&Params.FalseEasting, 1, 1);
    pvm_upkdouble(&Params.FalseNorthing, 1, 1);
   
    Projector::setInputFile(inputfilename);//setup cache, input image metrics, 
                                           //input projection
    
    toprojection = SetProjection(Params); //get the to projection
  }
  catch(...)
  {
    delete toprojection;
    toprojection = NULL;
  }
}

#endif




