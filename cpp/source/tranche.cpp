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

tranche::tranche(const security &sec, double quanti, const  currency::price price, currency::price fee, bool sell)
		: sec {sec}, quanti {quanti}, price {price}, fee {fee}
{
	timestamp.tm_year = 0;
	timestamp.tm_mon = 0;
	timestamp.tm_mday = 0;

	ordertype = sell ? SELL : BUY;
}

tranche::tranche(const tranche &rval)
: sec {rval.sec}, quanti {rval.quanti}, price {rval.price}, fee {rval.fee}, timestamp {rval.timestamp}
{
	ordertype = rval.ordertype;
}


void tranche::makeAbsolute(void)
{
	if (price.value < 0) price.value *= -1.0;
	if (fee.value < 0) fee.value *= -1.0;
	if (quanti < 0) quanti *= -1.0;
}

std::ostream &operator<<(std::ostream &os, const tranche &t)
{
	os << "Tranche ";
	if (t.isSell()) os << "(S) ";
	else os << "(B) ";

	os << std::setw(4) << t.getSecurity() << " stk: " << t.getQuanti() << " Price: " << t.getPrice() << " Fee: " << t.getFee();
	return os;
}


} /* namespace ibkr */
