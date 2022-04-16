/*
 * security.cpp
 *
 *  Created on: 27.03.2022
 *      Author: AndreasFellnhofer
 */

#include "security.hpp"

namespace ibkr
{

std::ostream &operator<<(std::ostream &o, const security &s)
{
	o << s.getName() << ": " << s.getPrice();
	return o;
}


} /* namespace ibkr */
