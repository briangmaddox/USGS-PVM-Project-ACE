
#ifdef _WIN32
#define __GNU_LIBRARY__
#endif

#ifdef _WIN32
#pragma warning( disable : 4291 ) // Disable VC warning messages for
                                  // new(nothrow)
#endif


#include <iostream>
#include <fstream>
#include <time.h>
#include "getopt.h"

#include "FrodoLib/SpinnerThingy.h"
#include "PvmProjector.h"

using namespace ProjLib;

//Get the projection from a input file
Projection * SetProjection(std::string parameterfile) throw();
DATUM GetDatum(std::string indatum) throw();
UNIT  GetUnit(std::string inunit) throw();
double ConvertToDMS(double degrees) throw();
void Status(long int linenumber, long int totallines);

int main(int argc, char **argv)
{
  PvmProjector projector;
  Projection * outproj = NULL; //output projection
  MathLib::Point newscale;
  int pmeshname, pmeshsize;
  int myopt;
  clock_t start, finish;
  std::string logname, filename(std::string(" ")), 
    parameterfile(std::string(" "));
  std::string outfile;
  bool timefile = false;
  bool samescale = false;
  std::ofstream out;
  int numofslaves;

  try
  {
    pmeshname = 0;
    pmeshsize = 4;
    newscale.x = 0;
    newscale.y = 0;
    //parse the arguments
    while (( myopt = getopt(argc, argv, "Ss:l:x:n:p:?")) != -1)
      switch(myopt)
      {
      case 'x':
        {
          if (optarg)
          {
            numofslaves = atoi(optarg);
          }
        }
        break;
      case 'S':
        {
          samescale = true;
        }
        break;
      case 's':
        {
          if (optarg)
          {
            newscale.x = atof(optarg);
            newscale.y = newscale.x;
          }
          
        }
        break;
      case 'l':
        {
          if (optarg)
          {
            timefile = true;
            logname = optarg;
          }
        }
		break;
      case 'n':
        {
          if (optarg)
          {
            pmeshsize = atoi(optarg);
            if (pmeshsize < 4)
              pmeshsize = 4;

          }
		  
        }
        break;
      case 'p':
        {
          if (optarg)
          {
            pmeshname = atoi(optarg);
                  
          }
          break;
        }
      case '?':
      default: // help
        {
          std::cout << "Usage: " << argv[0] << " [options]" 
                    << "InputFile InputProjectionFile [outputfile]"
                    << std::endl;
          std::cout << "where options are: " << std::endl;
          std::cout << "   -p pmesh number 6=LeastSqrs 8=Bilinear "
                    << "9=bicubic" << std::endl;
          std::cout << "   -n pmeshsize" << std::endl;
          std::cout << "   -l logfile for timings " << std::endl;
          std::cout << "   -s number where number is the output scale" 
                    << std::endl;
          std::cout << "   -S forces output scale same length as input scale"
                    << std::endl;
          std::cout << "   -x number the number of slaves" << std::endl;
          std::cout << "   -? this help screen" << std::endl;
          return 0;
        }
      }
    
    if (optind > argc-2 || argv[optind+1] == NULL)
    {
      std::cout << "You must specify a input file and a input projection file"
                << std::endl
                << "See -h for help" << std::endl;
      return 0;
    }
    else
    {
      filename = std::string(argv[optind++]);
      parameterfile = std::string(argv[optind++]);
    }
  
    if (parameterfile == std::string(" ") || filename == std::string(" "))
    {
      std::cout << "You must specify a input file and parameter file " 
                << "use -help for options" << std::endl;
      return 0;
    }
    
    outproj = SetProjection(parameterfile);
    
    if (!outproj)
    {
      std::cout << "Could not create the output projection!"
                << std::endl;
      return 0;
    }
    
    //check for the time file
    if (timefile)
    {
      out.open(logname.c_str()); //open the ouput file
      start = time(NULL);           //get the start
      out << start << std::endl; //output the start
    }
    
    if (optind == argc-1)
    {
      projector.setOutputFileName(std::string(argv[optind]));
    }

    projector.setOutputProjection(outproj);
    projector.setPmeshName(pmeshname);
    projector.setPmeshSize(pmeshsize);
    
    if (!samescale)
      projector.setOutputScale(newscale);
    else
      projector.setSameScale(true);
    
    projector.setInputFile(filename);
    projector.setNumberOfSlaves(numofslaves);
    if (!projector.project(Status))
      return false;
    
    Status(0,0);
    if (timefile)
    {
      finish = time(NULL);
      out << finish << std::endl;
      out << (finish - start);
      out.close();
    }
    return 0;
  }
  catch(...)
  {
    std::cout << "An exception has been thrown" << std::endl;
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
            (std::string("nad83sp"));
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
      
      if (projtype == std::string("GOOD"))
      {
        in >> sdatum >> sunit;
        if (!(proj = new (std::nothrow) GoodeHomolosineProjection(0.0,
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
   
   return UNKNOWN_DATUM;
}
  
UNIT  GetUnit(std::string inunit) throw()
{
  if (inunit == std::string("METERS"))
    return METERS;

  if (inunit == std::string("ARC_DEGREES"))
    return ARC_DEGREES;

  if ((inunit == std::string("FEET")) || (inunit == std::string("US_FEET")))
    return US_FEET;

  return UNKNOWN_UNIT;

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


//*********************************************************************
void Status(long int linenumber, long int totallines)
{
  static SpinnerThingy thingy;
  static bool first = true;

  if (first)
    {
      std::cout << "Reprojecting " << totallines << " scanlines" << std::endl;
      first = false;
    }
  
  if (linenumber == totallines)
    thingy.done("Done.");
  else
    {
    
      if ((linenumber % 10) == 0)
        thingy.update(linenumber);
    }

  

}





