/*
 * pnl.cpp
 *
 *  Created on: 17.06.2022
 *      Author: AndreasFellnhofer
 */

#include "pnl.hpp"

#include <string>
#include <ctime>

namespace ibkr
{

namespace pnl
{

std::string construct_key(std::tm const &tm, const char *security_name, const char *currency, int cnt)
{
	char buf[32];

	/* man, c++ can be shitty. doing this in a safe way is 15 lines of code. so'll do it unsafe. */
	snprintf(buf, 31, "%04u%02u%s-%s",
			1900 + tm.tm_year, 1 + tm.tm_mon, security_name, currency);

	return std::string(buf) + "-" + std::to_string(cnt);
}


bool long_and_short_fraction(double balance, double delta, double &long_frac, double &short_frac)
{
	double long_part = 0.0;
	double short_part = 0.0;

	if ((balance >= 0) && ((balance + delta) >= 0)) { long_part = std::abs(delta); short_part = 0.0; }
	else if ((balance <= 0) && ((balance + delta) <= 0)) { long_part = 0; short_part = std::abs(delta); }
	else if ((balance > 0) && ((balance + delta) < 0)) { long_part = balance; short_part = std::abs(balance + delta); }
	else if ((balance < 0) && ((balance + delta) > 0)) { long_part = balance + delta; short_part = std::abs(balance); }

	if ((std::abs(long_part) > 0.000001) || (std::abs(short_part) > 0.000001))
	{
		long_frac = long_part / (long_part + short_part);
		short_frac = short_part / (long_part + short_part);
	}
	else
	{
		if (std::abs(long_frac + short_frac - 1.0) > 0.00001) {
			printf("long_and_short_fraction warning: balance:%.2f delta:%.2f long_frac:%.2f short_frac:%.2f\n", balance, delta, long_frac, short_frac);
		}
		long_frac = 1.0;
		short_frac = 0.0;
		return false;
	}

	return true;
}

}
}
