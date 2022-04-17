/*
 * tranche.cpp
 *
 *  Created on: 27.03.2022
 *      Author: AndreasFellnhofer
 */

#include "tranche.hpp"
#include "security.hpp"
#include "currency.hpp"

namespace ibkr
{

tranche::tranche(const security &sec, int amount, const  currency::price price, currency::price fee, bool sell)
		: sec {sec}, amount {amount}, price {price}, fee {fee}
{
	ordertype = sell ? SELL : BUY;
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
	if (t.isSell()) os << "(S)";
	else os << "(B) ";

	os << t.getSecurity() << " stk: " << t.getAmount() << " Price:s " << t.getPrice() << " Fee: " << t.getFee();
	return os;
}


} /* namespace ibkr */
