/*
 * currency.cpp
 *
 *  Created on: 27.03.2022
 *      Author: AndreasFellnhofer
 */

#include "currency.hpp"
#include <ostream>

namespace ibkr
{
namespace currency
{


unit from_symbol(const symbol sym)
{
	unit cur = UNKNOWN;

	for (auto unit_i : match)
	{
		if (unit_i.id == sym)
		{
			cur = unit_i;
		}
	}

	return cur;
}


std::ostream &operator<<(std::ostream &ostr, const currency::price &price)
{
	ostr << price.value << " " << price.unit.name;
	return ostr;
}


std::ostream &operator<<(std::ostream &ostr, const currency::unit &cur)
{
	ostr << " " << cur.name;
	return ostr;
}



}
} /* namespace ibkr */
