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
#include "GNUgetopt/getopt.h"


using namespace ProjLib;

//Get the projection from a input file
Projection * SetProjection(std::string parameterfile) throw();
DATUM GetDatum(std::string indatum) throw();
UNIT  GetUnit(std::string inunit) throw();
double GetConvertedScale(Projection * in, Projection * out, double oldscale)
  throw ();

double ConvertToDMS(double degrees) throw();
void getMinMax(std::vector<double>& array, double& min, double& max)
  throw();

int main(int argc, char **argv)
{
 ProjIOLib::ProjectionReader reader;
 ProjIOLib::ProjectionWriter writer;
 Projection * fromprojection, * toprojection;
 USGSImageLib::ImageIFile * infile;
 USGSImageLib::GeoTIFFImageOFile* out = NULL; //the output file  
 USGSImageLib::CacheManager* cache = NULL;    //cache
 USGSImageLib::GeoTIFFImageIFile* ingeo;
 USGSImageLib::DOQImageIFile * indoq;
 PmeshLib::ProjectionMesh * pmesh = NULL;     //projection mesh
 int myopt;
 std::string filename(" ");
 std::string parameterfile(" ");
 long int xcounter, ycounter;                 //counter for loops
 long int height, width, newheight, newwidth; //image metrics
 double x, y;                                 //projection constants
 long int _x, _y;                             // for inputs
 int pmeshsize = 4;                           //pmeshsize default is 10
 unsigned char*  scanline = NULL;             //output scanline
 unsigned char*  inscanline = NULL;           // for the input scanlines
 std::string datumtype, units;                //strings for checks
 double left, right, bottom, top;             //projected extremes
 double min_x, min_y, max_x, max_y;           //unprojected extremes
 int photo, spp, bps;                         //image type
 float xscale = 0.0;                          //scale
 float yscale = 0.0; 
 std::string outfile("out.tif");              //output filename
 int interpolator = 0;

 // Arrays used to find the minimum and maximum values
 std::vector<double> xarr, yarr;
 double newscale;
 double oldscale;
 double scale[2] = {0};
 double tp[6] = {0}; //geotiff tie points
 double res[2] = {0}; //geotiff resoultion
 std::string logname("time.out");
 std::string name;
;
 double tempx, tempy;
 try
   {
     //parse the arguments
     while (( myopt = getopt(argc, argv, "l:n:p:f:i:?")) != -1)
       switch(myopt)
         {
         case 'l':
           {
             if (optarg)
               {
                 logname = optarg;
               }
           }
         case 'n':
           {
             if (optarg)
               {
                 pmeshsize = atoi(optarg);
                 
               }
           }
           break;
         case 'p':
           {
             if (optarg)
               {
                interpolator = atoi(optarg);
                 
               }
             break;
           }
         case 'f': //input file switch
           {
             if (optarg) // only change if they entered something
               filename = std::string(optarg);
             break;
           }
         case 'i': //parameter file for output
           {
             if (optarg)
               parameterfile = std::string(optarg);
             break;
           }
         case '?':
         default: // help
           {
             std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
             std::cout << "where options are: " << std::endl;
             std::cout << "   -f input file to reproject" << std::endl;
             std::cout << "   -i input parameter file" << std::endl;
             std::cout << "   -? this help screen"   
                       << "(Which you obviously already knew about)"
                       << std::endl;
             return 0;;
           }
         }
     
     if ((filename == std::string(" ")) || (parameterfile == std::string(" ")))
       {
         std::cout << "You must enter a input file and a parameter file" 
                   << " use -? for options" << std::endl;
         return 0;
       }

     //start the time
     name = std::string("date > ") + logname; 
     system(name.c_str());

     //try to open the file as a doq
      //try to open the file
    if (!(infile = new(std::nothrow)USGSImageLib::DOQImageIFile(filename)))
      throw std::bad_alloc();
        
    if (!infile->good())
      {
        delete infile;
        
        if (!(infile = new(std::nothrow)
              USGSImageLib::GeoTIFFImageIFile(filename)))
          throw std::bad_alloc();
            
        if (!infile->good())
          {
            std::cout << "Invalid file type" << std::endl;
            return 0;
          }
        
        fromprojection = reader.createProjection
          (dynamic_cast<USGSImageLib::GeoTIFFImageIFile*>(infile));
        
        if (!fromprojection)
          {
            std::cout << "Unable to open a projection for input file"
                      << std::endl;
            return 0;
          }
        
        //get the scale and such
        ingeo = dynamic_cast<USGSImageLib::GeoTIFFImageIFile *>(infile);
        ingeo->getPixelScale(scale);
        if (scale[0] != scale[1])
          {
            std::cout << "scale different in x and y directions" << std::endl;
            throw;
          }
        oldscale=scale[0];
        ingeo->getTiePoints(tp, 6);
        min_x = tp[3];
        max_y = tp[4];
        
      }
    else
      {
        fromprojection = reader.createProjection
          (dynamic_cast<USGSImageLib::DOQImageIFile*>(infile));
        
        if (!fromprojection)
          {
            std::cout << "Unable to open a projection for input file"
                      << std::endl;
            return 0;
          }

        indoq = dynamic_cast<USGSImageLib::DOQImageIFile * > (infile);
        
        indoq->getHorizontalResolution(xscale);
        oldscale = xscale;
        indoq->getXOrigin(min_x);
        indoq->getYOrigin(max_y);
       
      }

    //get some image metrics
    infile->getHeight(height);
    infile->getWidth(width);
    infile->getSamplesPerPixel(spp);
    infile->getBitsPerSample(bps);
    infile->getPhotometric(photo);    

    //create the cache for the file
    if (!(cache = new(std::nothrow) USGSImageLib::CacheManager(infile, 300, 
                                                               true)))
      throw std::bad_alloc();
    if (!cache->good())
      {
        std::cerr << "Doh!  Cache is bad!" << std::endl;
        throw USGSImageLib::ImageException(IMAGE_FILE_OPEN_ERR);
      }


    max_x = min_x + oldscale * width;
    min_y = max_y - oldscale * height;
    
    //TODO create the to projection here
    toprojection = SetProjection(parameterfile);
    
    if (!toprojection)
      throw std::bad_alloc();

    //get the new scale
    newscale = GetConvertedScale(fromprojection, toprojection, oldscale);
    
    //setup Pmesh
    if (interpolator != 0)
      {
        if (!(pmesh = new (std::nothrow) PmeshLib::ProjectionMesh()))
          throw std::bad_alloc();
        
         //setup the foward mesh
        pmesh->setSourceMeshBounds(min_x, min_y, max_x, max_y);
        pmesh->setMeshSize(pmeshsize,pmeshsize); 
        pmesh->setInterpolator(interpolator);
        //now project all the points onto the mesh
        pmesh->calculateMesh((*fromprojection), (*toprojection));
      }

    
    //make room in the vector for the edges
    xarr.resize(2*(width+height));
    yarr.resize(2*(height + width));
    
    //now loop through and project the edges
    for (xcounter = 0; xcounter < width; xcounter++)
    {
      //reproject the top line
      tempx = min_x + oldscale*xcounter;
      tempy = max_y;
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
      tempx = min_x + oldscale*xcounter;
      tempy = min_y;
      if (pmesh)
        pmesh->projectPoint(tempx, tempy);
      else
        {
          fromprojection->projectToGeo(tempx, tempy, tempy, tempx);
          toprojection->projectFromGeo(tempy, tempx, tempx, tempy);
        }
      xarr[xcounter + width] = tempx;
      yarr[xcounter + width] = tempy;
    }
    
    //now do the height
    for (ycounter = 0; ycounter < height; ycounter++)
    {
      //reproject the left line
      tempy = max_y - oldscale * ycounter;
      tempx = min_x;
      if (pmesh)
        pmesh->projectPoint(tempx, tempy);
      else
        {
          fromprojection->projectToGeo(tempx, tempy, tempy, tempx);
          toprojection->projectFromGeo(tempy, tempx, tempx, tempy);
        }
      xarr[ycounter + 2*width] = tempx;
      yarr[ycounter + 2*width] = tempy;
      
      //reproject the right line
      tempy = max_y - oldscale * ycounter;
      tempx = max_x;
      if (pmesh)
        pmesh->projectPoint(tempx, tempy);
      else
        {
          fromprojection->projectToGeo(tempx, tempy, tempy, tempx);
          toprojection->projectFromGeo(tempy, tempx, tempx, tempy);
        }
      xarr[ycounter + 2*width + height] = tempx;
      yarr[ycounter + 2*width + height] = tempy;
    }


    //get the min and max of the coordinates
    getMinMax(xarr, left, right);
    getMinMax(yarr, bottom, top);
    if (pmesh)
      {
        //now setup the reverse projection mesh
        pmesh->setSourceMeshBounds(left, bottom, right, top);
        pmesh->setMeshSize(pmeshsize,pmeshsize); 
        pmesh->setInterpolator(interpolator);
        //now project all the points onto the mesh
        pmesh->calculateMesh((*toprojection), (*fromprojection));
      }
    
    //get the new pixel width and height
    newwidth  = static_cast<long int>((right - left)/(newscale) + 0.5);
    newheight = static_cast<long int>((top - bottom)/(newscale) + 0.5);
    
    
    //create the scanline
    if (!(scanline = new(std::nothrow)unsigned char[newwidth]))
      throw std::bad_alloc();
    
    if (!(inscanline = new(std::nothrow) unsigned char[newwidth]))
      throw std::bad_alloc();

    //Create the output file
    out = writer.createGeoTIFF(toprojection, outfile, newwidth, 
                               newheight, photo);
    
    if (!out)
      {
        std::cout << "Unable to open the output file" << std::endl;
        return 0;
      }
      
    tp[3]=left;
    tp[4]=top;
    res[0] = newscale;
    res[1] = newscale;
    
    out->setSamplesPerPixel(spp);
    out->setBitsPerSample(bps);
    out->setTiePoints(tp, 6);
    out->setPixelScale(res);
    out->setPlanarConfig(1);

    //now start addding pixels
    for (ycounter =0; ycounter < newheight; ycounter++)
      {
        std::cout << ycounter+1 << " of " << newheight 
                  << " processed" << std::endl;
        
        for (xcounter = 0; xcounter < newwidth; xcounter++)
          {   
            x = left + newscale * xcounter;
            y = top  - newscale * ycounter;
                
            //now get the reverse projected value
            if (pmesh)
              pmesh->projectPoint(x, y);
            else
              {
                toprojection->projectToGeo(x, y, y, x);
                fromprojection->projectFromGeo(y, x, x, y);
              }
            _x = static_cast<long int>((x - min_x) / (oldscale) + 0.5);
            _y = static_cast<long int>((max_y - y) / (oldscale) + 0.5);

            if ((_x >= width) || (_x < 0) || (_y >= height) || (_y < 0))
              {
                scanline[xcounter] = 0;
              }
            else
              {
                cache->getRawScanline(_y, inscanline);
                scanline[xcounter] = inscanline[static_cast<long int>(_x)];
              }
            }
        //write the scanline
        out->putRawScanline(ycounter, scanline);
      }

    //end the done time
    name = std::string("date >> ") + logname;
    system(name.c_str());
    
    delete cache;
    delete toprojection;
    delete [] scanline;
    delete infile;
    return 0;
    
  }
 catch(ProjectionException &e)
 {
   std::cout << "projection exeception thrown" << std::endl;
   delete toprojection;
   delete [] scanline;
   delete infile;
   delete cache;
   return 0;
 }
 catch(USGSImageLib::ImageException &ie)
   {
     std::cout << "image exeception thrown" << std::endl;
     delete toprojection;
     delete [] scanline;
     delete infile;
     delete cache;
     return 0;
   }
 catch(...)
  {
    std::cout << "An error ocurred at inscanline " << y << std::endl
              << "output scanline " << ycounter << std::endl;
    delete toprojection;
    delete [] scanline;
    delete infile;
    delete cache;
    return 0;
  }

}


Projection * SetProjection(std::string parameterfile) throw()
{
   
  std::ifstream in;
  std::string projtype, sdatum, sunit;
  double StdParallel1 = 0.0;
  double StdParallel2 = 0.0;
  double NatOriginLong = 0.0;
  double NatOriginLat = 0.0;
  double FalseEasting = 0.0;
  double FalseNorthing = 0.0;
  double FalseOriginLong = 0.0;
  double FalseOriginLat = 0.0;
  double FalseOriginEasting = 0.0;
  double FalseOriginNorthing = 0.0;
  double CenterLong = 0.0;
  double CenterLat = 0.0;
  double CenterEasting = 0.0;
  double CenterNorthing = 0.0;
  double ScaleAtNatOrigin = 1.0;
  double AzimuthAngle = 0.0;
  double StraightVertPoleLong = 0.0;
  int zone = 0;
  Projection * proj;
  
  try
    {
      in.open(parameterfile.c_str());
      
      
      in >> projtype;
      
      if (projtype == std::string("GEO"))
        {
          in >> sdatum;
          in >> sunit;
          if (!(proj = new (std::nothrow) GeographicProjection
                (GetDatum(sdatum), GetUnit(sunit))))
            throw std::bad_alloc();
        }
      
      if (projtype == std::string("UTM"))
        {
          in >> zone;
          in >> sdatum;
          in >> sunit;
          
          
          if (!(proj = new (std::nothrow) UTMProjection(zone, 
                                                        GetDatum(sdatum), 
                                                        GetUnit(sunit))))
            throw std::bad_alloc();
        }
      
      if (projtype == std::string("SPCS"))
        {
          in >> zone;
          in >> sdatum;
          in >> sunit;
          StatePlaneProjection::setNAD83ParameterFilename
            (std::string("./nad83sp"));
          //create the state plane projection
          //here I am assuming that the zone will be 
          //a correct state plane zone
          if (!(proj = new (std::nothrow) StatePlaneProjection
                (zone, GetDatum(sdatum), GetUnit(sunit))))
            throw std::bad_alloc();
        }
      
      if (projtype == std::string("ALBERS"))
        {
          in >> sdatum;
          in >> sunit;
          in >> StdParallel1
             >> StdParallel2
             >> CenterLong
             >> NatOriginLat
             >> FalseEasting
             >> FalseNorthing;
          
          if(!(proj =  new (std::nothrow) AlbersConicProjection
               ( ConvertToDMS(StdParallel1), 
                 ConvertToDMS(StdParallel2),
                 0.0, 0.0, ConvertToDMS(CenterLong),
                 ConvertToDMS(NatOriginLat), 
                 FalseEasting, FalseNorthing, GetDatum(sdatum), 
                 GetUnit(sunit))))
            throw std::bad_alloc();
        
          
        }
      if (projtype == std::string("AZMEQD"))
        {
          in >> sdatum >> sunit >> CenterLong >> CenterLat
             >> FalseEasting >> FalseNorthing;
          

          if(!(proj = new (std::nothrow) AzimuthalEquidistantProjection
               ( ConvertToDMS(CenterLong),
                 ConvertToDMS(CenterLat),
                 FalseEasting, FalseNorthing, 0.0, GetDatum(sdatum), 
                 GetUnit(sunit))))
            throw std::bad_alloc();
        }
      if (projtype == std::string("GNOMON"))
        {
          in >> sdatum >> sunit >> CenterLong >> CenterLat
             >> FalseEasting >> FalseNorthing;

          if(!(proj = new(std::nothrow) GnomonicProjection
               ( ConvertToDMS(CenterLong),
                 ConvertToDMS(CenterLat),
                 FalseEasting,FalseNorthing, 
                 0.0, GetDatum(sdatum), GetUnit(sunit))))
            throw std::bad_alloc();
        }
      if (projtype == std::string("LAMAZ"))
        {
          in >> sdatum >> sunit >> CenterLong
             >> CenterLat >> FalseEasting >> FalseNorthing;
          
          if(!(proj = new(std::nothrow) LambertAzimuthalProjection
               ( ConvertToDMS(CenterLong), ConvertToDMS(CenterLat),
                 FalseEasting, FalseNorthing, 0.0, 
                 GetDatum(sdatum), GetUnit(sunit))))
            throw std::bad_alloc();
        }
      if (projtype == std::string("ORTHO"))
        {
          in >> sdatum >> sunit >> CenterLong
             >> CenterLat >> FalseEasting >> FalseNorthing;
          
          if(!(proj = new(std::nothrow) OrthographicProjection
               ( ConvertToDMS(CenterLong),
                 ConvertToDMS(CenterLat),
                 FalseEasting,FalseNorthing, 0.0, 
                 GetDatum(sdatum), GetUnit(sunit))))
            throw std::bad_alloc();
        }
      if (projtype == std::string("STEREO"))
        {
          in >> sdatum >> sunit >> CenterLong
             >> CenterLat >> FalseEasting >> FalseNorthing;
          
          if(!(proj = new(std::nothrow) StereographicProjection
               (ConvertToDMS(CenterLong),
                ConvertToDMS(CenterLat),
                FalseEasting, FalseNorthing,
                0.0, GetDatum(sdatum), GetUnit(sunit))))
            throw std::bad_alloc();
        }
      if (projtype == std::string("MILLER"))
        {
          in >> sdatum >> sunit >> CenterLong
             >> FalseEasting >> FalseNorthing;
          
          if(!(proj = new (std::nothrow) MillerCylindricalProjection
               ( 0.0, ConvertToDMS(CenterLong),
                 FalseEasting, FalseNorthing,
                 GetDatum(sdatum), GetUnit(sunit))))
            throw std::bad_alloc();
        }
      if (projtype == std::string("ROBIN"))
        {
          in >> sdatum >> sunit >> CenterLong
             >> FalseEasting >> FalseNorthing;

          if(!(proj = new(std::nothrow) RobinsonProjection
               ( 0.0, ConvertToDMS(CenterLong),
                 FalseEasting, FalseNorthing, 
                 GetDatum(sdatum), GetUnit(sunit))))
            throw std::bad_alloc();
        }
      if (projtype == std::string("SNSOID"))
        {
          in >> sdatum >> sunit >> CenterLong
             >> FalseEasting >> FalseNorthing;
          if(!(proj = new(std::nothrow) SinusoidalProjection
               ( 0.0, ConvertToDMS(CenterLong),
                 FalseEasting,
                 FalseNorthing, GetDatum(sdatum), GetUnit(sunit))))
            throw std::bad_alloc();
        }
      if (projtype == std::string("EQUIDC"))
        {
          in >> sdatum >> sunit >> StdParallel1
             >> StdParallel2;
          
          if ( StdParallel1 == StdParallel2 )
            {
              in >> CenterLat >> CenterLong >> NatOriginLat
                 >> FalseEasting >> FalseNorthing;
              if(!(proj =  new(std::nothrow) EquidistantConicProjection
                   ( ConvertToDMS(CenterLat), 0.0, 0.0,
                     ConvertToDMS(CenterLong),
                     ConvertToDMS(NatOriginLat),
                     FalseEasting,
                     FalseNorthing,
                     GetDatum(sdatum), GetUnit(sunit))))
                throw std::bad_alloc();
              
            }
          else
            {      
              in >> CenterLong >> NatOriginLat >> FalseEasting
                 >> FalseNorthing;
              if(!(proj = new(std::nothrow) EquidistantConicProjection
                   ( ConvertToDMS(StdParallel1),
                     ConvertToDMS(StdParallel2),
                     0.0, 0.0,
                     ConvertToDMS(CenterLong),
                     ConvertToDMS(NatOriginLat),
                     FalseEasting,
                     FalseNorthing,
                     GetDatum(sdatum), GetUnit(sunit))))
                throw std::bad_alloc();
            }
        }
      if (projtype == std::string("EQRECT"))
        {
          in >> sdatum >> sunit >> CenterLat >> CenterLong
             >> FalseEasting >> FalseNorthing;
          
          if(!(proj =  new(std::nothrow) EquirectangularProjection
               ( ConvertToDMS(CenterLat), 0.0, 
                 ConvertToDMS(CenterLong),
                 FalseEasting,FalseNorthing, 
                 GetDatum(sdatum), GetUnit(sunit))))
            throw std::bad_alloc();
        }
      if (projtype == std::string("HOM"))
        {
          in >> sdatum >> sunit >> ScaleAtNatOrigin
             >> AzimuthAngle >> CenterLong
             >> CenterLat >> FalseEasting 
             >> FalseNorthing;
          
          if(!(proj = new (std::nothrow) HotineObliqueMercatorProjection
               ( ScaleAtNatOrigin, AzimuthAngle,
                 0.0, 0.0, ConvertToDMS(CenterLong),
                 ConvertToDMS(CenterLat), FalseEasting,
                 FalseNorthing, GetDatum(sdatum), GetUnit(sunit))))
            throw std::bad_alloc();
        }
      if (projtype == std::string("LAMCC"))
        {
          in >> sdatum >> sunit >> StdParallel1 >> StdParallel2
             >> NatOriginLong >> FalseOriginLat
             >> FalseEasting >> FalseNorthing;
          
          if(!(proj = new(std::nothrow) LambertConformalConicProjection
               ( ConvertToDMS(StdParallel1), ConvertToDMS(StdParallel2),
                 0.0, 0.0, ConvertToDMS(NatOriginLong),
                 ConvertToDMS(FalseOriginLat),
                 FalseEasting,FalseNorthing, GetDatum(sdatum), 
                 GetUnit(sunit))))
            throw std::bad_alloc();
        }
      if (projtype == std::string("MERCAT"))
        {
          in >> sdatum >> sunit >> NatOriginLong
             >> NatOriginLat >> CenterEasting 
             >> CenterNorthing;
          
          if(!(proj = new(std::nothrow) MercatorProjection
               ( 0.0, 0.0, ConvertToDMS(NatOriginLong),
                 ConvertToDMS(NatOriginLat),
                 CenterEasting, CenterNorthing, 
                 GetDatum(sdatum), GetUnit(sunit))))
            throw std::bad_alloc();
        }
      if (projtype == std::string("POLYC"))
        {
          
          in >> sdatum >> sunit >> CenterLong
             >> CenterLat >> FalseEasting
             >> FalseNorthing;
          
          if(!(proj = new(std::nothrow) PolyconicProjection
               ( 0.0, 0.0, ConvertToDMS(CenterLong),
                 ConvertToDMS(CenterLat), FalseEasting,
                 FalseNorthing, GetDatum(sdatum), 
                 GetUnit(sunit))))
            throw std::bad_alloc();
        }
      if (projtype == std::string("PS"))
        {
          in >> sdatum >> sunit >> StraightVertPoleLong
             >> NatOriginLat >> FalseEasting >> FalseNorthing;
          
          if(!(proj = new(std::nothrow) PolarStereographicProjection
               (ConvertToDMS(StraightVertPoleLong),
                ConvertToDMS(NatOriginLat), 0.0, 0.0,
                FalseEasting,FalseNorthing, 
                GetDatum(sdatum), GetUnit(sunit))))
            throw std::bad_alloc();
        }
      if (projtype == std::string("ALASKA"))
        {
          in >> sdatum >> sunit >> FalseEasting 
             >> FalseNorthing;
          
          if(!(proj =  new(std::nothrow) AlaskaConformalProjection
                   (0.0, 0.0, FalseEasting, FalseNorthing,
                    GetDatum(sdatum), GetUnit(sunit))))
            throw std::bad_alloc();
        }
      if (projtype == std::string("TM"))
        {
          in >> sdatum >> sunit >> ScaleAtNatOrigin 
             >> CenterLong >> NatOriginLat
             >> FalseEasting >> FalseNorthing;
          
          if(!(proj = new(std::nothrow) TransverseMercatorProjection
               (ScaleAtNatOrigin, 0.0, 0.0,
                ConvertToDMS(CenterLong),
                ConvertToDMS(NatOriginLat), FalseEasting,
                FalseNorthing, 
                GetDatum(sdatum), GetUnit(sunit))))
              throw std::bad_alloc();
        }
      if (projtype == std::string("VGRINT"))
        {
          in >> sdatum >> sunit >> CenterLat
             >> CenterLong >> FalseEasting 
             >> FalseNorthing;
          if(!(proj = new(std::nothrow) VanDerGrintenProjection
               ( ConvertToDMS(CenterLat), 0.0, 
                 ConvertToDMS(CenterLong),
                 FalseEasting,FalseNorthing,
                 GetDatum(sdatum), GetUnit(sunit))))
            throw std::bad_alloc();
        }
      in.close();
      return proj;
    }
  catch(...)
    {
      in.close();
      return NULL;
    }
}
      
      
DATUM GetDatum(std::string indatum) throw()
{
   
   if (indatum == std::string("ADINDAN"))
     return ADINDAN;
   
   if (indatum == std::string("ARC1950"))
     return ARC1950;
   
   if (indatum == std::string("ARC1960"))
     return ARC1960;
   
   if (indatum == std::string("AUSTRALIAN_GEODETIC_1966"))
     return AUSTRALIAN_GEODETIC_1966;
   
   if (indatum == std::string("AUSTRALIAN_GEODETIC_1984"))
     return AUSTRALIAN_GEODETIC_1984;
     
   if (indatum == std::string("CAPE"))
     return CAPE;
   
   if (indatum == std::string("EUROPEAN_DATUM_1950"))
     return EUROPEAN_DATUM_1950;
   
   if (indatum == std::string("HU_TZU_SHAN"))
     return HU_TZU_SHAN;
   
   if (indatum == std::string("INDIAN"))
     return INDIAN;

   if (indatum == std::string("NAD27"))
     return NAD27;
     
   if (indatum == std::string("NAD83"))
     return NAD83;

   if (indatum == std::string("ORDNANCE_SURVEY_1936"))
     return ORDNANCE_SURVEY_1936;

   if (indatum == std::string("PULKOVO_1942"))
     return PULKOVO_1942;

   if (indatum == std::string("PROVISIONAL_S_AMERICAN_1956"))
     return PROVISIONAL_S_AMERICAN_1956;

   if (indatum == std::string("TOKYO"))
     return TOKYO;
   
   if (indatum == std::string("WGS_72"))
     return WGS_72;
   
   if (indatum == std::string("WGS_84"))
     return WGS_84;
   
   return -1;
}
  
UNIT  GetUnit(std::string inunit) throw()
{
  if (inunit == std::string("METERS"))
    return METERS;

  if (inunit == std::string("ARC_DEGREES"))
    return ARC_DEGREES;

  if ((inunit == std::string("FEET")) || (inunit == std::string("US_FEET")))
    return US_FEET;

  return -1;

}

// **************************************************************************
double GetConvertedScale(Projection * in, Projection * out, double oldscale)
throw ()
{
  UNIT inunit, outunit; //the units
 
  try
    {
      inunit = in->getUnit();
      outunit = out->getUnit();
      
      if (inunit == outunit)
        return oldscale; //no conversion

      if ((inunit == METERS) && (outunit == US_FEET))
        return oldscale*3.28083988;//3.28083988 Feet/Meter

      if ((inunit == US_FEET) && (outunit == METERS))
        return oldscale/3.28083988;// inverse of above

      if((inunit == METERS) && (outunit == ARC_DEGREES))
        return oldscale/120000;

      return 0; //error
    }
  catch(...)
    {
      return 0;
    }
}

// **************************************************************************
double ConvertToDMS(double degrees) throw()
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

// ************************************************************************
void getMinMax(std::vector<double>& array, double& min, double& max)
  throw()
{
  std::sort(array.begin(), array.end());

  max = array[array.size() - 1];
  min = array[0];
}





