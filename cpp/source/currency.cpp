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


unit from_symbol(const symbol s)
{
	unit u = UNKNOWN;

	for (int i = 0; i < sizeof(match) / sizeof(match[0]); ++i)
	{
		if (match[i].id == s)
		{
			u = match[i];
		}
	}

	return u;
}


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
