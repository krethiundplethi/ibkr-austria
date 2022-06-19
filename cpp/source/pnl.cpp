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

std::string construct_key(std::tm const &tm, const char *security_name, int cnt)
{
	char buf[32];

	/* man, c++ can be shitty. doing this in a safe way is 15 lines of code. so'll do it unsafe. */
	snprintf(buf, 31, "%04u%02u%s",
			1900 + tm.tm_year, 1 + tm.tm_mon, security_name);

	return std::string(buf) + "-" + std::to_string(cnt);
}


void long_and_short_fraction(double balance, double delta, double &long_frac, double &short_frac)
{
	double long_part = 0.0;
	double short_part = 0.0;

	if ((balance >= 0) && ((balance + delta) >= 0)) { long_part = std::abs(delta); short_part = 0.0; }
	else if ((balance <= 0) && ((balance + delta) <= 0)) { long_part = 0; short_part = std::abs(delta); }
	else if ((balance > 0) && ((balance + delta) < 0)) { long_part = balance; short_part = std::abs(balance + delta); }
	else if ((balance < 0) && ((balance + delta) > 0)) { long_part = balance + delta; short_part = std::abs(balance); }

	if ((long_part != 0.0) || (short_part != 0.0))
	{
		long_frac = long_part / (long_part + short_part);
		short_frac = short_part / (long_part + short_part);
	}
	else
	{
		printf("Long/short Warning");
		long_frac = 1.0;
		short_frac = 0.0;
	}
}

}
}
