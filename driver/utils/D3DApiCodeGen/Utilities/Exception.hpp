
#pragma once

#include "stdafx.h"


class Exception : public std::exception
{
public:

    Exception(const std::string& message) :
  m_message(message)
    {
    }

  virtual ~Exception()
    {
    }

    virtual const char* what() const
    {
        return m_message.c_str();
    }

protected:

    std::string m_message;

};

