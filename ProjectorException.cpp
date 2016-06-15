#ifndef PROJECTOREXCEPTION_CPP
#define PROJECTOREXCEPTION_CPP


#include "ProjectorException.h"


//**********************************************************************
ProjectorException::ProjectorException() : myexception(PROJECTOR_ERROR_UNKOWN)
{}
  
//**********************************************************************
ProjectorException::
ProjectorException(const short unsigned int & inexception)
{
  myexception = inexception;
}

//**********************************************************************
ProjectorException::~ProjectorException()
{}

//*********************************************************************
void ProjectorException::
setException(const short unsigned int & inexception) throw()
{
  myexception = inexception;
}

//**********************************************************************
short unsigned int ProjectorException::getException() const throw()
{
  return myexception;
}

//**********************************************************************
std::string ProjectorException::getExceptionMessage() const throw()
{
  switch(myexception)
    {
    case 0: 
      return std::string("Projector Error: Bad Input");
    default:
      return std::string("Projector Error: Unkown Error");
    }
}

#endif
