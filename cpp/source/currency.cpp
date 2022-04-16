/*
 * currency.cpp
 *
 *  Created on: 27.03.2022
 *      Author: AndreasFellnhofer
 */

#include "currency.hpp"

namespace ibkr
{
namespace currency
{


std::ostream &operator<<(std::ostream &os, const currency::price &p)
{
	os << p.value << " " << p.unit.name;
	return os;
}


std::ostream &operator<<(std::ostream &os, const currency::unit &u)
{
	os << " " << u.name;
	return os;
}



}
} /* namespace ibkr */
