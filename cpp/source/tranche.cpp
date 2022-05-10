/*
 * tranche.cpp
 *
 *  Created on: 27.03.2022
 *      Author: AndreasFellnhofer
 */

#include "tranche.hpp"
#include "security.hpp"
#include "currency.hpp"
#include <iomanip>

namespace ibkr
{

tranche::tranche(const security &sec, int amount, const  currency::price price, currency::price fee, bool sell)
		: sec {sec}, amount {amount}, price {price}, fee {fee}
{
	ordertype = sell ? SELL : BUY;
}

tranche::tranche(const tranche &rval)
: sec {rval.sec}, amount {rval.amount}, price {rval.price}, fee {rval.fee}
{
	ordertype = rval.ordertype;
}


void tranche::makeAbsolute(void)
{
	if (price.value < 0) price.value *= -1.0;
	if (fee.value < 0) fee.value *= -1.0;
	if (amount < 0) amount *= -1;
}

std::ostream &operator<<(std::ostream &os, const tranche &t)
{
	os << "Tranche ";
	if (t.isSell()) os << "(S) ";
	else os << "(B) ";

	os << std::setw(4) << t.getSecurity() << " stk: " << t.getAmount() << " Price: " << t.getPrice() << " Fee: " << t.getFee();
	return os;
}


} /* namespace ibkr */
