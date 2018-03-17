
#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#include <exception>
#include <string>

class Exception : public std::exception
{
public:
	Exception(const char* msg)
	  : mMessage(msg)
	{}

	virtual const char* what() const throw()
	{
		return mMessage.c_str();
	}

    virtual ~Exception() throw()
    {}

private:
	std::string mMessage;
};

#endif // _EXCEPTION_H_
