﻿#include "StringEventArgs.h"


namespace EngineLayer
{

    StringEventArgs::StringEventArgs(const std::string &s, std::vector<std::string> &nestedIDs) : MyRecursiveEventArgs(nestedIDs)
    {
        this->privateS = s;
    }

    std::string StringEventArgs::getS() const
    {
        return privateS;
    }
}
