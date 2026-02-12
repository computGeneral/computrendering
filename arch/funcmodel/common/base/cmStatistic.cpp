/**************************************************************************
 *
 */

#include "cmStatistic.h"
#include <iostream>

using namespace std;
using namespace arch;
using namespace arch::gpuStatistics;

bool Statistic::disabled = false;

void Statistic::enable()
{
    disabled = false;
}

void Statistic::disable()
{
    disabled = true;
}

bool Statistic::isEnabled()
{
    return !disabled;
}


Statistic::Statistic(string name) : name(name), owner(string("")), freq(0)
{
}

Statistic::~Statistic()
{
}



Statistic& Statistic::operator++()
{
    if ( disabled )
        return *this;

    inc(1);
    return *this;
}

Statistic& Statistic::operator++(int)
{
    if ( disabled )
        return *this;

    inc(1);
    return *this;
}


Statistic& Statistic::operator--()
{
    if ( disabled )
        return *this;

    inc(-1);
    return *this;
}

Statistic& Statistic::operator--(int)
{
    if ( disabled )
        return *this;

    inc(-1);
    return *this;
}


string Statistic::getName() const
{
    return name;
}

void Statistic::setName(string name)
{
    this->name = name;
}

string Statistic::getOwner() const
{
    return owner;
}


void Statistic::setOwner(string owner)
{
    this->owner = owner;
}

void Statistic::setCurrentFreq(int f)
{
    freq = f;
}
