/*
 * tranche.cpp
 *
 *  Created on: 27.03.2022
 *      Author: AndreasFellnhofer
 */

#include "tranche.h"
#include "security.h"
#include "currency.h"

namespace ibkr
{

tranche::tranche(const security &sec, int amount, const  currency::price price, currency::price fee, bool sell)
		: sec {sec}, amount {amount}, price {price}, fee {fee}
{
	ordertype = sell ? SELL : BUY;
}


std::ostream &operator<<(std::ostream &os, const tranche &t)
{
	os << "Tranche: " << t.getSecurity() << " Fee: " << t.getFee();
	return os;
}


} /* namespace ibkr */
