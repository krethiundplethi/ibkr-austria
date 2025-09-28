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
	filled = 0.0;

	ordertype = sell ? SELL : BUY;
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
	else if (t.isHold()) os << "(H) ";
	else os << "(B) ";

	os << std::setw(4) << t.getSecurity() << " stk: " << t.getQuanti() << " Price: " << t.getPrice() << " Fee: " << t.getFee();
	return os;
}


bool tranche_compare(std::shared_ptr<ibkr::tranche> tr1, std::shared_ptr<ibkr::tranche> tr2)
{
	const std::tm &ts1 = tr1->getTimeStamp();
	const std::tm &ts2 = tr2->getTimeStamp();

	if (tr1->getSecurity().getName().compare(tr2->getSecurity().getName()) < 0) { return true; }
	if (tr1->getSecurity().getName().compare(tr2->getSecurity().getName()) > 0) { return false; }

	if (tr1->getSecurity().getPrice().unit.id < tr2->getSecurity().getPrice().unit.id) { return true; }
	if (tr1->getSecurity().getPrice().unit.id > tr2->getSecurity().getPrice().unit.id) { return false; }

	if (ts1.tm_mon < ts2.tm_mon) { return true; }
	if (ts1.tm_mon > ts2.tm_mon) { return false; }

	if (ts1.tm_mday < ts2.tm_mday) { return true; }
	if (ts1.tm_mday > ts2.tm_mday) { return false; }

	if (ts1.tm_hour < ts2.tm_hour) { return true; }
	if (ts1.tm_hour > ts2.tm_hour) { return false; }

	if (ts1.tm_min < ts2.tm_min) { return true; }
	if (ts1.tm_min > ts2.tm_min) { return false; }

	if (ts1.tm_sec < ts2.tm_sec) { return true; }
	if (ts1.tm_sec > ts2.tm_sec) { return false; }

	if (tr1->isSplit() && !tr2->isSplit()) { return true; }
	if (tr2->isSplit() && !tr1->isSplit()) { return false; }

	//if (tr1->isSplitOut() && !tr2->isSplitOut()) { return true; }
	//if (tr2->isSplitOut() && !tr1->isSplitOut()) { return false; }

	return false;
}



} /* namespace ibkr */
