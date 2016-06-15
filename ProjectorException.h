#ifndef PROJECTOREXCEPTION_H
#define PROJECTOREXCEPTION_H

#include <new>
#include <string>


//defines for errors

#define PROJECTOR_ERROR_BADINPUT 0
#define PROJECTOR_ERROR_UNKOWN   255


//ProjectorException Class
class ProjectorException
{
public:
  //Constructors and Destructor
  ProjectorException();
  ProjectorException(const short unsigned int & inexception);
  ~ProjectorException();

  void setException(const short unsigned int & inProjection) throw();
  short unsigned int getException() const throw();
  std::string getExceptionMessage() const throw();

protected:
  short unsigned int myexception;
};

#endif
