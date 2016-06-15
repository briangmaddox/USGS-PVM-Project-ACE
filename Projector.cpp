#ifndef PROJECTOR_CPP
#define PROJECTOR_CPP

#ifdef _WIN32
#pragma warning( disable : 4291 ) // Disable VC warning messages for
                                  // new(nothrow)
#endif


#include "Projector.h"
#include "ImageLib/RGBPalette.h"
//*********************************************************************
Projector::Projector() : fromprojection(NULL), toprojection(NULL),
infile(NULL), out(NULL), cache(NULL), 
oldheight(0), oldwidth(0), newheight(0), newwidth(0),
pmeshsize(4), pmeshname(0), outfile("out.tif"), samescale(false)
{
  oldscale.x = newscale.x = 0;
  oldscale.y = newscale.y = 0;
  
}
  
//***********************************************************************
Projector::Projector(std::string & ininfile, 
            ProjLib::Projection * inoutproject,
            const std::string & inoutfile) : fromprojection(NULL), 
            toprojection(NULL), infile(NULL), out(NULL), cache(NULL), 
            oldheight(0), 
            oldwidth(0), newheight(0), newwidth(0),
            pmeshsize(4), pmeshname(0), outfile("out.tif"), samescale(false)
{
  oldscale.x = newscale.x = 0;                //initialize scale
  oldscale.y = newscale.y = 0;

  setInputFile(ininfile);                     //open the input file
  toprojection = inoutproject;                //set the to project
  Params = getParams(toprojection);           //get the Params

  outfile = inoutfile;                        //get the output filename
  
}
  
//********************************************************************
Projector::Projector(std::string & ininfile, 
            const ProjectionParams&  inParams,
            const std::string & inoutfile) : fromprojection(NULL), 
            toprojection(NULL), infile(NULL), out(NULL), cache(NULL), 
            oldheight(0), 
            oldwidth(0), newheight(0), newwidth(0),
            pmeshsize(4), pmeshname(0), outfile("out.tif"), samescale(false)
{
  oldscale.x = newscale.x = 0;                //initialize scale
  oldscale.y = newscale.y = 0;
  
  setInputFile(ininfile);                     //open the input file
  Params = inParams;                          //get the Params
  toprojection = SetProjection(Params);       //get the 
  outfile = inoutfile;                        //get the output filename
  
}

//***********************************************************************
Projector::~Projector()
{
  delete cache;
  delete toprojection;
  delete infile;
}
  

//**********************************************************************
void Projector::setInputFile(std::string & ininfile ) throw()
{
  setupInput(ininfile); //setup the input file and params
}

//*************************************************************************
void Projector::
 setOutputProjection(ProjLib::Projection * inoutproject) throw()
{
  toprojection = inoutproject;
  Params = getParams(inoutproject);  //fill in the projection params struct
}

//*************************************************************************
void Projector::setOutputProjection(const ProjectionParams& inParams) throw()
{
  Params = inParams;
  toprojection = SetProjection(Params);  //create the projection from the 
                                         //params
}

//************************************************************************
void Projector::setOutputFileName(const std::string& inoutfile) throw()
{
  outfile = inoutfile; //get a different output filename
}

//************************************************************************
void Projector::setPmeshName(const int& inpmeshname) throw()
{
  pmeshname = inpmeshname; //set the pmesh name
}

//************************************************************************
void Projector::setPmeshSize(const long int & inpmeshsize) throw()
{
  pmeshsize = inpmeshsize;  //set the pmesh size
}

//***********************************************************************
void Projector::setOutputScale(const MathLib::Point & innewscale) throw()
{
  newscale = innewscale;   //set a new scale instead of apsect perserving
}


//***********************************************************************
void Projector::setSameScale(bool insamescale) throw()
{
  samescale = insamescale;
}


//**********************************************************************
ProjLib::Projection * Projector::getOutputProjection() const throw()
{
  return toprojection;    //give up the output projection
}

//**********************************************************************
ProjectionParams Projector::getOutputProjectionParams() const throw()
{
  return Params;         //give up the output Params
}

//**********************************************************************
int Projector::getPmeshName() const throw()
{
  return pmeshname;      //give the pmesh type
}

//**************************************************************
int Projector::getPmeshSize() const throw()
{
  return pmeshsize;     //return the square of the number of nodes
}
  
//**************************************************************
bool Projector::project(void (*status)(long int, long int)) 
throw(ProjectorException)
{
  double x, y;                                 //temp variables
  long int _x, _y;                             //for world to pixel translation
  unsigned char* scanline = NULL;              //output scanline
  const unsigned char* inscanline = NULL;      //input scanline
  int sppcounter;                              //for colored images
  PmeshLib::ProjectionMesh * pmesh = NULL;     //projection mesh
  long int xcounter, ycounter;                 //counters for each direction
  try
  {
    if (!fromprojection || !toprojection)      //check for projections
      return false;
    
    
    pmesh = setupForwardPmesh();               //setup the forward mesh

    if (!getExtents(pmesh))                    //try to get the etxents
      return false;
    
    if (!setupOutput(outfile))                 //setup the output file
      return false;
      
    if (pmesh)                                 //check for existing pmesh
    {
      delete pmesh;                             
      pmesh = setupReversePmesh();             //setup the reverse mesh
    }
    
    
    if (!(scanline = new (std::nothrow) unsigned char [newwidth*spp]))
      throw std::bad_alloc();
                                              
                                               //now start adding pixels
    for (ycounter =0; ycounter < newheight; ycounter++)
    {
      if (status)                              //check for output status func
        status(ycounter, newheight);
      
      
      for (xcounter = 0; xcounter < newwidth; xcounter++)
      {   
                                               //get the new x and y pixel
        x = outRect.left + newscale.x * xcounter;
        y = outRect.top  - newscale.y * ycounter;
        
        
        if (pmesh)                             //now get the old pixel
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
        /*if (_x >= oldwidth)
          _x = _x - 1;
          else
          if (_x < 0)
          _x = 0;
          if (_y >= oldheight)
          _y = _y -1;
          else
          if (_y < 0)
          _y = 0;*/
        if ((_x >= oldwidth) || (_x < 0) || (_y >= oldheight) 
            || (_y < 0))
        {
          for (sppcounter = 0; sppcounter < spp; sppcounter++)
            scanline[xcounter*spp + sppcounter] = 0; //out of bounds pixel
        }
        else
        {
          inscanline = cache->getRawScanline(_y);    //get a pointer to cache
          for (sppcounter = 0; sppcounter < spp; sppcounter++)
            scanline[xcounter*spp + sppcounter ] = 
              inscanline[_x*spp + sppcounter];       //copy pixels
          
        }
        
      }
      
      out->putRawScanline(ycounter, scanline);       //write out scanlines
    }
    
    writer.removeImage(0);                           //flush the output file
    out = NULL;
    delete [] scanline;                              //delete the scanline
    delete pmesh;                                    //delete the pmesh
    return true;                                     //done successfully
  }
  catch(...)
  {
    delete [] scanline;                              //delete the scanline
    delete pmesh;                                    //delete the pmesh
    return false;                                    //something bad happened
  }      
}

//**********************************************************
bool Projector::setupInput(std::string & ininfile) throw()
{
  double scale[3] = {0};
  double tp[6] = {0};
  short unsigned int tpnum;
  float xscale = 0.0;
  USGSImageLib::GeoTIFFImageIFile* ingeo;
  USGSImageLib::DOQImageIFile * indoq;
  
  
  try
  {
    //check for prexisting input file
    if (infile)
    {
      delete infile;
      infile = NULL;
    }
    
    //try to open the file as a doq
    if (!(infile = new(std::nothrow)USGSImageLib::DOQImageIFile(ininfile)))
      throw std::bad_alloc();
    
    if (!infile->good())
    {
      delete infile;
      
      if (!(infile = new(std::nothrow)
            USGSImageLib::GeoTIFFImageIFile(ininfile, false)))
        throw std::bad_alloc();
      
      if (!infile->good())
      {
        throw ProjectorException(PROJECTOR_ERROR_BADINPUT);
      }
      
      //check for preexisting fromprojection
      if (fromprojection)
      {
        reader.removeProjection(0);
      }
      
      
      fromprojection = reader.createProjection
        (dynamic_cast<USGSImageLib::GeoTIFFImageIFile*>(infile));
      
      if (!fromprojection)
      {
        throw ProjectorException(PROJECTOR_ERROR_BADINPUT);
      }
      
      //get the scale and such
      ingeo = dynamic_cast<USGSImageLib::GeoTIFFImageIFile *>(infile);
      ingeo->getPixelScale(scale);
      oldscale.x = scale[0];
      oldscale.y = scale[1];
      ingeo->getTiePoints(tp, tpnum);
      inRect.left = tp[3];
      inRect.top  = tp[4];
      
    }
    else
    {
      fromprojection = reader.createProjection
        (dynamic_cast<USGSImageLib::DOQImageIFile*>(infile));
      
      if (!fromprojection)
      {
        throw ProjectorException(PROJECTOR_ERROR_BADINPUT);
      }
      
      indoq = dynamic_cast<USGSImageLib::DOQImageIFile * > (infile);
      
      indoq->getHorizontalResolution(xscale);
      oldscale.x = xscale;
      oldscale.y = xscale;
      indoq->getXOrigin(inRect.left);
      indoq->getYOrigin(inRect.top);
      
    }
    
    //get some image metrics
    infile->getHeight(oldheight);
    infile->getWidth(oldwidth);
    infile->getSamplesPerPixel(spp);
    infile->getBitsPerSample(bps);
    infile->getPhotometric(photo);    
    
    //check for preexisting cache
    if (cache)
      delete cache;
    cache = NULL;
    
    //create the cache for the file
    if (!(cache = new(std::nothrow) USGSImageLib::CacheManager
          (infile, /*oldheight*0.20*/1000, true)))
      throw std::bad_alloc();
    if (!cache->good())
    {
      //can't open the cache so delete it and set default cache
      delete cache;
      if (!(cache = new (std::nothrow) USGSImageLib::CacheManager
            (infile, 1, false)))
        throw std::bad_alloc();
    }
    
      
    inRect.right = inRect.left + oldscale.x * oldwidth;
    inRect.bottom = inRect.top - oldscale.y * oldheight;
    
    return true;
  }
  catch(...)
  {
    if (infile)
      delete infile;
    if (cache)
      delete cache;
    return false;
  }
}

//***********************************************************************
bool Projector::setupOutput(std::string & inoutfile) throw()
{
  double tp[6] = {0};
  double res[3] = {0};
  
  try
  {
    //perform some checks
    if (!newwidth || !newheight || !infile)
      return false;
    
    if (out)                //check for preexisting output file
    {
      writer.removeImage(0); //keep only one geotiff at a time
      out = NULL;
    }
    //Create the output file
    tp[3] = outRect.left;
    tp[4] = outRect.top;
    res[0] = newscale.x;
    res[1] = newscale.y;
    res[2] = 0;
    out = writer.create(toprojection, outfile, newwidth, 
                        newheight, photo,tp, res);
    
    if (!out)
    {
      std::cout << "Unable to open the output file" << std::endl;
          return 0;
    }
    
    //check for palette
    if (photo == PHOTO_PALETTE)
    {
      USGSImageLib::RGBPalette temppal;
      infile->getPalette(&temppal);
      out->setPalette(&temppal);
    }
    
    dynamic_cast<USGSImageLib::TIFFImageOFile*>(out)->setCompression
        (COMPRESSION_PACKBITS);
    out->setSamplesPerPixel(spp);
    out->setBitsPerSample(bps);
    return true;
  }
  catch(...)
  {
    return false;
  }
}



//**********************************************************************
PmeshLib::ProjectionMesh * Projector::setupForwardPmesh() throw()
{
  PmeshLib::ProjectionMesh * ret = NULL;          //return pmesh
  
  try
  {
    if (pmeshname != 0)
    {
      if(!(ret = new (std::nothrow) PmeshLib::ProjectionMesh))
        throw std::bad_alloc();
      
      //setup the foward mesh
      ret->setSourceMeshBounds(inRect.left, inRect.bottom, 
                               inRect.right, inRect.top);
      ret->setMeshSize(pmeshsize,pmeshsize); 
      ret->setInterpolator(pmeshname);
      //now project all the points onto the mesh
      ret->calculateMesh((*fromprojection), (*toprojection));
    }
    return ret;
  }
  catch(...)
  {
    delete ret;
    ret = NULL;
    return ret;
  }
}

//*********************************************************************
PmeshLib::ProjectionMesh * Projector::setupReversePmesh() throw()
{
  PmeshLib::ProjectionMesh * ret = NULL;          //return pmesh
  
  try
  {
    if (pmeshname != 0)
    {
      if(!(ret = new (std::nothrow) PmeshLib::ProjectionMesh))
        throw std::bad_alloc();
      
      //setup the reverse mesh
      ret->setSourceMeshBounds(outRect.left, outRect.bottom, 
                               outRect.right, outRect.top);
      ret->setMeshSize(pmeshsize,pmeshsize); 
      ret->setInterpolator(pmeshname);
      //now project all the points onto the mesh
      ret->calculateMesh((*toprojection), (*fromprojection));
    }
    return ret;
    
  }
  catch(...)
  {
    delete ret;
    ret = NULL;
    return ret;
  }
}



//**********************************************************************
bool Projector::getExtents(PmeshLib::ProjectionMesh * pmesh) throw()
{
  std::vector<double> xarr, yarr;
  long int xcounter, ycounter; 
  double tempx, tempy;

  try
  {
    //do some checks
    if (!infile)
      return false;
    
    if (!toprojection)
      return false;
    
    //make room in the vector for the edges
    xarr.resize(2*(oldwidth+oldheight));
    yarr.resize(2*(oldheight + oldwidth));
    
    //now we should be good to go
    for (xcounter = 0; xcounter < oldwidth; xcounter++)
    {
      //reproject the top line
      tempx = inRect.left + oldscale.x*xcounter;
      tempy = inRect.top;
      if (pmesh)
        pmesh->projectPoint(tempx, tempy);
      else
      {
        fromprojection->projectToGeo(tempx, tempy, tempy, tempx);
        toprojection->projectFromGeo(tempy, tempx, tempx, tempy);
      }
      xarr[xcounter] = tempx;
      yarr[xcounter] = tempy;
      
      //reproject the bottom line
      tempx = inRect.left + oldscale.x*xcounter;
      tempy = inRect.bottom;
      if (pmesh)
        pmesh->projectPoint(tempx, tempy);
      else
      {
        fromprojection->projectToGeo(tempx, tempy, tempy, tempx);
        toprojection->projectFromGeo(tempy, tempx, tempx, tempy);
      }
      xarr[xcounter + oldwidth] = tempx;
      yarr[xcounter + oldwidth] = tempy;
    }
    
    //now do the height
    for (ycounter = 0; ycounter < oldheight; ycounter++)
    {
      //reproject the left line
      tempy = inRect.top - oldscale.y * ycounter;
      tempx = inRect.left;
      if (pmesh)
        pmesh->projectPoint(tempx, tempy);
      else
      {
        fromprojection->projectToGeo(tempx, tempy, tempy, tempx);
        toprojection->projectFromGeo(tempy, tempx, tempx, tempy);
      }
      xarr[ycounter + 2*oldwidth] = tempx;
      yarr[ycounter + 2*oldwidth] = tempy;
      
      //reproject the right line
      tempy = inRect.top - oldscale.y * ycounter;
      tempx = inRect.right;
      if (pmesh)
        pmesh->projectPoint(tempx, tempy);
      else
      {
        fromprojection->projectToGeo(tempx, tempy, tempy, tempx);
        toprojection->projectFromGeo(tempy, tempx, tempx, tempy);
      }
      xarr[ycounter + 2*oldwidth + oldheight] = tempx;
      yarr[ycounter + 2*oldwidth + oldheight] = tempy;
    }
    
    
    //get the min and max of the coordinates
    getMinMax(xarr, outRect.left, outRect.right);
    getMinMax(yarr, outRect.bottom, outRect.top);
    
    
    if (samescale)
      newscale = getSameScale(oldscale, fromprojection, toprojection);
    
    //get the new scale
    if (!newscale.x || !newscale.y)
      newscale = GetConvertedScale(oldwidth,oldheight,outRect);
    
    //get the new pixel width and height
    newwidth  = static_cast<long int>
      ((outRect.right - outRect.left)/(newscale.x) + 0.5);
    newheight = static_cast<long int>
      ((outRect.top - outRect.bottom)/(newscale.y) + 0.5);
    return true;
  }
  catch(...)
  {
    return false;
  }
}


//*************************************************************************
ProjectionParams Projector::
getParams(ProjLib::Projection * proj) const throw()
{
  ProjectionParams ret; //the return structure
  
  try
  {
    
    //first get the projsys and datum
    ret.projtype = proj->getProjectionSystem();
    ret.datum = proj->getDatum();
    ret.unit =  proj->getUnit();
    switch(ret.projtype)
    {
    case UTM:
      ret.zone = dynamic_cast<UTMProjection *>(proj)->getZone();
      break;
    case SPCS: 
      ret.zone = dynamic_cast<StatePlaneProjection *>(proj)->getZone();
      break;
    case ALBERS:
      {
        AlbersConicProjection* albers = 
          dynamic_cast<AlbersConicProjection*>(proj);
        ret.StdParallel1 = 
          ConvertFromDMS(albers->getFirstStandardParallel());
        ret.StdParallel2 = 
          ConvertFromDMS(albers->getSecondStandardParallel());
        ret.CenterLong = ConvertFromDMS(albers->getCentralMeridian());
        ret.NatOriginLat = ConvertFromDMS(albers->getOriginLatitude());
        ret.FalseEasting = albers->getFalseEasting();
        ret.FalseNorthing = albers->getFalseNorthing();
        break;
      }
    case AZMEQD:
    case GNOMON:
    case LAMAZ:
    case ORTHO:
    case STEREO:
      {
        AzimuthalProjection* azim =
          dynamic_cast<AzimuthalProjection *>(proj);
        ret.CenterLong = ConvertFromDMS(azim->getCentralLongitude());
        ret.CenterLat = ConvertFromDMS(azim->getCentralLatitude());
        ret.FalseEasting = azim->getFalseEasting();
        ret.FalseNorthing = azim->getFalseNorthing();
        break;
      }
    case MILLER:
    case ROBIN:
    case SNSOID:
      {
        PseudocylindricalProjection* pc = 
          dynamic_cast<PseudocylindricalProjection*>(proj);
        ret.CenterLong = ConvertFromDMS(pc->getCentralMeridian());
        ret.FalseEasting = pc->getFalseEasting();
        ret.FalseNorthing = pc->getFalseNorthing();
        break;
      }
    case EQUIDC:
      {
        EquidistantConicProjection* edc =
          dynamic_cast<EquidistantConicProjection*>(proj);
        ret.CenterLat = ConvertFromDMS(edc->getStandardParallel());
        ret.CenterLong = ConvertFromDMS(edc->getCentralMeridian());
        ret.NatOriginLat = ConvertFromDMS(edc->getOriginLatitude());
        ret.FalseEasting = edc->getFalseEasting();
        ret.FalseNorthing = edc->getFalseNorthing();
        ret.StdParallel1 = ConvertFromDMS(edc->getFirstStandardParallel());
        ret.StdParallel2 = 
          ConvertFromDMS(edc->getSecondStandardParallel());
        break;
      }
    case EQRECT:
      {
        EquirectangularProjection* eqrect =
          dynamic_cast<EquirectangularProjection*>(proj);
        ret.CenterLat = ConvertFromDMS(eqrect->getTrueScaleLatitude());
        ret.CenterLong = ConvertFromDMS(eqrect->getCentralMeridian());
        ret.FalseEasting = eqrect->getFalseEasting();
        ret.FalseNorthing = eqrect->getFalseNorthing();
        break;
      }
    case HOM:
      {
        HotineObliqueMercatorProjection* homerc =
          dynamic_cast<HotineObliqueMercatorProjection*>(proj);
        
        // Make sure this is a type B HOM projection
        if ( 0.0 != homerc->getLatitude1()  
             || 0.0 != homerc->getLatitude2() 
             || 0.0 != homerc->getLongitude1() 
             || 0.0 != homerc->getLongitude2() 
             || 0.0 != homerc->getAzimuthPoint())
          return ret;
        
        ret.ScaleAtNatOrigin = homerc->getProjectionCenterScaleFactor();
        ret.AzimuthAngle = homerc->getAzimuthAngle();
        ret.CenterLong = ConvertFromDMS(homerc->getCentralMeridian());
        ret.CenterLat = ConvertFromDMS(homerc->getOriginLatitude());
        ret.FalseEasting = homerc->getFalseEasting();
        ret.FalseNorthing = homerc->getFalseNorthing();
        break;
      }
    case LAMCC:
      {
        LambertConformalConicProjection* lamcc =
          dynamic_cast<LambertConformalConicProjection*>(proj);
        ret.StdParallel1 = 
          ConvertFromDMS(lamcc->getFirstStandardParallel());
        ret.StdParallel2 = 
          ConvertFromDMS(lamcc->getSecondStandardParallel());
        ret.NatOriginLong = ConvertFromDMS(lamcc->getCentralMeridian());
        ret.FalseOriginLat = ConvertFromDMS(lamcc->getOriginLatitude());
        ret.FalseEasting = lamcc->getFalseEasting();
        ret.FalseNorthing = lamcc->getFalseNorthing();
        break;
      }
    case MERCAT:
      {
        MercatorProjection* merc =
          dynamic_cast<MercatorProjection*>(proj);
        ret.NatOriginLong = ConvertFromDMS(merc->getCentralMeridian());
        ret.NatOriginLat = ConvertFromDMS(merc->getOriginLatitude());
        ret.CenterEasting = merc->getFalseEasting();
        ret.CenterNorthing = merc->getFalseNorthing();
        break;
      }
    case POLYC:
      {
        PolyconicProjection* polycon =
          dynamic_cast<PolyconicProjection*>(proj);
        ret.CenterLong = ConvertFromDMS(polycon->getCentralMeridian());
        ret.CenterLat = ConvertFromDMS(polycon->getOriginLatitude());
        ret.FalseEasting = polycon->getFalseEasting();
        ret.FalseNorthing = polycon->getFalseNorthing();
        break;
      }
    case PS:
      {
        PolarStereographicProjection* polar = 
          dynamic_cast<PolarStereographicProjection*>(proj); 
        ret.StraightVertPoleLong = 
          ConvertFromDMS(polar->getLongitudePole());
        ret.NatOriginLat = ConvertFromDMS(polar->getTrueScaleLatitude());
        ret.FalseEasting = polar->getFalseEasting();
        ret.FalseNorthing = polar->getFalseNorthing();
        break;
      }
    case ALASKA:
    case TM:
      {
        TransverseMercatorProjection* trans = 
          dynamic_cast<TransverseMercatorProjection*>(proj);
        ret.ScaleAtNatOrigin = trans->getCentralMeridianScaleFactor();
        ret.CenterLong = ConvertFromDMS(trans->getCentralMeridian());
        ret.NatOriginLat = ConvertFromDMS(trans->getOriginLatitude());
        ret.FalseEasting = trans->getFalseEasting();
        ret.FalseNorthing = trans->getFalseNorthing();
        break;
      }
    case VGRINT:
      {
        VanDerGrintenProjection* vandg =
          dynamic_cast<VanDerGrintenProjection*>(proj);
        ret.CenterLat = ConvertFromDMS(vandg->getOriginLatitude());
        ret.CenterLong =ConvertFromDMS(vandg->getCentralMeridian());
        ret.FalseEasting = vandg->getFalseEasting();
        ret.FalseNorthing = vandg->getFalseNorthing();
        break;
      }
	case GOOD: //good has no real params
    default:
      return ret;
    }
    return ret;
  }
  catch(...)
  {
    return ret;
  }
  
}

//*********************************************************
ProjLib::Projection * Projector::
    SetProjection(const ProjectionParams & inParams) throw()
{
  Projection * proj;
  
  
  try
    {
      if (inParams.projtype == ProjLib::GEO)
      {
        if (inParams.unit != ProjLib::ARC_DEGREES)
          return NULL;
        
        if (!(proj = new (std::nothrow) GeographicProjection
              (inParams.datum, inParams.unit)))
          throw std::bad_alloc();
      }
      
      if (inParams.projtype == ProjLib::UTM)
      {
        if (!(proj = new (std::nothrow) UTMProjection(inParams.zone, 
                                                      inParams.datum, 
                                                      inParams.unit)))
          throw std::bad_alloc();
      }
      
      if (inParams.projtype == ProjLib::SPCS)
      {
        StatePlaneProjection::setNAD83ParameterFilename
          (std::string("nad83sp"));
        //create the state plane projection
        //here I am assuming that the zone will be 
        //a correct state plane zone
        if (!(proj = new (std::nothrow) StatePlaneProjection
              (inParams.zone, inParams.datum, inParams.unit)))
          throw std::bad_alloc();
      }
      
      if (inParams.projtype == ProjLib::ALBERS)
      {
        if(!(proj =  new (std::nothrow) AlbersConicProjection
             ( ConvertToDMS(inParams.StdParallel1), 
                 ConvertToDMS(inParams.StdParallel2),
               0.0, 0.0, ConvertToDMS(inParams.CenterLong),
               ConvertToDMS(inParams.NatOriginLat), 
               inParams.FalseEasting, inParams.FalseNorthing, 
               inParams.datum, 
               inParams.unit)))
          throw std::bad_alloc();
        
        
      }
      if (inParams.projtype == ProjLib::AZMEQD)
      {
        if(!(proj = new (std::nothrow) AzimuthalEquidistantProjection
             ( ConvertToDMS(inParams.CenterLong),
               ConvertToDMS(inParams.CenterLat),
               inParams.FalseEasting, inParams.FalseNorthing, 
               0.0, inParams.datum, 
               inParams.unit)))
          throw std::bad_alloc();
      }
      if (inParams.projtype == ProjLib::GNOMON)
      {
        
        if(!(proj = new(std::nothrow) GnomonicProjection
             ( ConvertToDMS(inParams.CenterLong),
               ConvertToDMS(inParams.CenterLat),
               inParams.FalseEasting,inParams.FalseNorthing, 
               0.0, inParams.datum, inParams.unit)))
            throw std::bad_alloc();
      }
      if (inParams.projtype == ProjLib::LAMAZ)
      {
        if(!(proj = new(std::nothrow) LambertAzimuthalProjection
             ( ConvertToDMS(inParams.CenterLong), 
               ConvertToDMS(inParams.CenterLat),
               inParams.FalseEasting, inParams.FalseNorthing, 0.0, 
               inParams.datum, inParams.unit)))
          throw std::bad_alloc();
      }
      if (inParams.projtype == ProjLib::ORTHO)
      {
        if(!(proj = new(std::nothrow) OrthographicProjection
             ( ConvertToDMS(inParams.CenterLong),
               ConvertToDMS(inParams.CenterLat),
               inParams.FalseEasting,inParams.FalseNorthing, 0.0, 
               inParams.datum, inParams.unit)))
          throw std::bad_alloc();
      }
      if (inParams.projtype == ProjLib::STEREO)
      {
        if(!(proj = new(std::nothrow) StereographicProjection
             (ConvertToDMS(inParams.CenterLong),
              ConvertToDMS(inParams.CenterLat),
              inParams.FalseEasting, inParams.FalseNorthing,
              0.0, inParams.datum, inParams.unit)))
          throw std::bad_alloc();
      }
      if (inParams.projtype == ProjLib::MILLER)
      {
        if(!(proj = new (std::nothrow) MillerCylindricalProjection
             ( 0.0, ConvertToDMS(inParams.CenterLong),
               inParams.FalseEasting, inParams.FalseNorthing,
               inParams.datum, inParams.unit)))
          throw std::bad_alloc();
      }
      if (inParams.projtype == ProjLib::ROBIN)
      {
        if(!(proj = new(std::nothrow) RobinsonProjection
             ( 0.0, ConvertToDMS(inParams.CenterLong),
               inParams.FalseEasting, inParams.FalseNorthing, 
               inParams.datum, inParams.unit)))
          throw std::bad_alloc();
      }
      if (inParams.projtype == ProjLib::SNSOID)
      {
        if(!(proj = new(std::nothrow) SinusoidalProjection
             ( 0.0, ConvertToDMS(inParams.CenterLong),
               inParams.FalseEasting,
               inParams.FalseNorthing, inParams.datum, 
               inParams.unit)))
          throw std::bad_alloc();
      }
      if (inParams.projtype == ProjLib::EQUIDC)
      {
        if ( inParams.StdParallel1 == inParams.StdParallel2 )
        {
          if(!(proj =  new(std::nothrow) EquidistantConicProjection
               ( ConvertToDMS(inParams.CenterLat), 0.0, 0.0,
                 ConvertToDMS(inParams.CenterLong),
                 ConvertToDMS(inParams.NatOriginLat),
                 inParams.FalseEasting,
                 inParams.FalseNorthing,
                 inParams.datum, inParams.unit)))
            throw std::bad_alloc();
          
        }
        else
        {      
          if(!(proj = new(std::nothrow) EquidistantConicProjection
               ( ConvertToDMS(inParams.StdParallel1),
                 ConvertToDMS(inParams.StdParallel2),
                 0.0, 0.0,
                 ConvertToDMS(inParams.CenterLong),
                 ConvertToDMS(inParams.NatOriginLat),
                 inParams.FalseEasting,
                 inParams.FalseNorthing,
                 inParams.datum, inParams.unit)))
            throw std::bad_alloc();
        }
      }
      if (inParams.projtype == ProjLib::EQRECT)
      {
        if(!(proj =  new(std::nothrow) EquirectangularProjection
             ( ConvertToDMS(inParams.CenterLat), 0.0, 
               ConvertToDMS(inParams.CenterLong),
               inParams.FalseEasting,inParams.FalseNorthing, 
               inParams.datum, inParams.unit)))
          throw std::bad_alloc();
      }
      if (inParams.projtype == ProjLib::HOM)
      {
        if(!(proj = new (std::nothrow) HotineObliqueMercatorProjection
             ( inParams.ScaleAtNatOrigin, inParams.AzimuthAngle,
               0.0, 0.0, ConvertToDMS(inParams.CenterLong),
               ConvertToDMS(inParams.CenterLat), inParams.FalseEasting,
               inParams.FalseNorthing, inParams.datum, inParams.unit)))
          throw std::bad_alloc();
      }
      if (inParams.projtype == ProjLib::LAMCC)
      {
        if(!(proj = new(std::nothrow) LambertConformalConicProjection
             ( ConvertToDMS(inParams.StdParallel1), 
               ConvertToDMS(inParams.StdParallel2),
               0.0, 0.0, ConvertToDMS(inParams.NatOriginLong),
               ConvertToDMS(inParams.FalseOriginLat),
               inParams.FalseEasting,
               inParams.FalseNorthing, inParams.datum, 
               inParams.unit)))
          throw std::bad_alloc();
      }
      if (inParams.projtype == ProjLib::MERCAT)
      {
        if(!(proj = new(std::nothrow) MercatorProjection
             ( 0.0, 0.0, ConvertToDMS(inParams.NatOriginLong),
               ConvertToDMS(inParams.NatOriginLat),
               inParams.CenterEasting, inParams.CenterNorthing, 
               inParams.datum, inParams.unit)))
          throw std::bad_alloc();
      }
      if (inParams.projtype == ProjLib::POLYC)
      {
        if(!(proj = new(std::nothrow) PolyconicProjection
             ( 0.0, 0.0, ConvertToDMS(inParams.CenterLong),
               ConvertToDMS(inParams.CenterLat), 
               inParams.FalseEasting,
               inParams.FalseNorthing, inParams.datum, 
               inParams.unit)))
          throw std::bad_alloc();
      }
      if (inParams.projtype == ProjLib::PS)
      {
        if(!(proj = new(std::nothrow) PolarStereographicProjection
             (ConvertToDMS(inParams.StraightVertPoleLong),
              ConvertToDMS(inParams.NatOriginLat), 0.0, 0.0,
              inParams.FalseEasting, inParams.FalseNorthing, 
              inParams.datum, inParams.unit)))
          throw std::bad_alloc();
      }
      if (inParams.projtype == ProjLib::ALASKA)
      {
        if(!(proj =  new(std::nothrow) AlaskaConformalProjection
             (0.0, 0.0, inParams.FalseEasting, 
              inParams.FalseNorthing,
              inParams.datum, inParams.unit)))
          throw std::bad_alloc();
      }
      if (inParams.projtype == ProjLib::TM)
      {
        if(!(proj = new(std::nothrow) TransverseMercatorProjection
             (inParams.ScaleAtNatOrigin, 0.0, 0.0,
              ConvertToDMS(inParams.CenterLong),
              ConvertToDMS(inParams.NatOriginLat), 
              inParams.FalseEasting,
              inParams.FalseNorthing, 
              inParams.datum, inParams.unit)))
          throw std::bad_alloc();
      }
      if (inParams.projtype == ProjLib::VGRINT)
      {
        if(!(proj = new(std::nothrow) VanDerGrintenProjection
             ( ConvertToDMS(inParams.CenterLat), 0.0, 
               ConvertToDMS(inParams.CenterLong),
               inParams.FalseEasting,
               inParams.FalseNorthing,
               inParams.datum, inParams.unit)))
          throw std::bad_alloc();
      }
      if (inParams.projtype == ProjLib::GOOD)
      {
        if (!(proj = new (std::nothrow) 
              GoodeHomolosineProjection(0.0, inParams.datum,
                                        inParams.unit)))
          throw std::bad_alloc();
      }
      return proj;
    }
  catch(...)
  {
    return NULL;
  }
}


//********************************************************************
MathLib::Point Projector::
GetConvertedScale(double oldwidth, double oldheight, 
                                   DRect outRec) throw ()
{
  MathLib::Point ret;               //return value
  
  //try to maintain the same number of pixels in each direction
  ret.x = (outRec.right - outRec.left)/oldwidth;
  ret.y = (outRec.top - outRec.bottom)/oldheight;
  return ret;
}

//*********************************************************************
double Projector::ConvertToDMS(double degrees) const throw()
{
  double temp;
  int deg, min;
  
  int sign = 1;
  
  temp = degrees;
  if (degrees < 0.0)
  {
    sign = -1;
    temp = -temp;
  }
  //get the degrees
  deg = static_cast<int>(temp);
  temp -= deg;
  temp *= 60.0; //get minutes
  min = static_cast<int>(temp);
  temp -= min; //get seconds
  temp *= 60;

  temp = deg * 1000000 + min * 1000 + temp;
  return temp*sign;
}

//************************************************************************
double Projector::ConvertFromDMS(double dms) const throw()
{
  double sign = 1.0;
  double temp = dms;
  int deg;
  int min;
  if ( temp < 0.0 )
    {
      temp = -temp;
      sign = -1.0;
    }
  
  // Extract the degrees, minutes, and seconds
  deg = static_cast<int>(temp / 1000000);
  temp -= deg * 1000000;
  min = static_cast<int>(temp / 1000);
  temp -= min * 1000;
  temp /= 3600;
  temp += min / 60.0;
  temp += deg;
  return temp *sign;
}


//********************************************************************
void Projector::
getMinMax(std::vector<double>& array, double& min, double& max)
    throw()
{
  std::sort(array.begin(), array.end());
  
  max = array[array.size() - 1];
  min = array[0];
}

//**********************************************************************
MathLib::Point Projector::
getSameScale(MathLib::Point inoldscale, ProjLib::Projection * in,
	  ProjLib::Projection * out) const throw()
{
  ProjLib::UNIT inunit;     //tempory input unit
  ProjLib::UNIT outunit;    //tempory output unit
  MathLib::Point ret;       //return point
  
  inunit = in->getUnit();
  outunit = out->getUnit();
  
  //run through the cases
  if (inunit == ProjLib::METERS)
  {
    if (outunit == ProjLib::METERS)
      ret = inoldscale;
    
    if (outunit == ProjLib::US_FEET)
    {
      ret.x = inoldscale.x*0.3048;
      ret.y = inoldscale.y*0.3048;
    }
    
    if (outunit == ProjLib::ARC_DEGREES)
    {
      ret.x = inoldscale.x/110574.0;
      ret.y = inoldscale.y/111319.0;
    }
  }
  
  if (inunit == ProjLib::US_FEET)
  {
    if (outunit == ProjLib::METERS)
    {
      ret.x = inoldscale.x/0.3048;
      ret.y = inoldscale.y/0.3048;
    }
    if (outunit == ProjLib::US_FEET)
      ret = inoldscale;
    
    if (outunit == ProjLib::ARC_DEGREES)
    {
      ret.x = inoldscale.x/362776.0;
      ret.y = inoldscale.y/365221.0;
    }
  }
  
  if (inunit == ProjLib::ARC_DEGREES)
  {
    if (outunit == ProjLib::METERS)
    {
      ret.x = inoldscale.x * 110574.0;
      ret.y = inoldscale.y * 111319.0;
    }
    if (outunit == ProjLib::US_FEET)
    {
      ret.x = inoldscale.x*362776.0;
      ret.y = inoldscale.y*365221.0;
    }
    
    if (outunit == ProjLib::ARC_DEGREES)
    {
      ret = inoldscale;
    }
  }
  return ret;
}



#endif













