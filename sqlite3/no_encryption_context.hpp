#pragma once

#include "encryption_context.hpp"


namespace ysqlite3
{

class no_encryption_context : public encryption_context
{
public:
};

}