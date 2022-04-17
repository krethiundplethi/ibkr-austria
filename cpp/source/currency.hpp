/*
 * currency.h
 *
 *  Created on: 27.03.2022
 *      Author: AndreasFellnhofer
 */

#ifndef SOURCE_CURRENCY_H_
#define SOURCE_CURRENCY_H_

#include <iostream>

namespace ibkr
{

namespace currency
{

enum class symbol {
	EUR,
	USD,
	AUD,
	CAD,
	RUB,
	UNKNOWN
};


struct unit
{
	symbol id;
	const char *name;
};


struct price
{
	struct unit unit;
	float value;
};


unit from_symbol(const symbol s);


std::ostream &operator<<(std::ostream &, const currency::price &);
std::ostream &operator<<(std::ostream &, const currency::unit &);


constexpr struct unit EUR = { symbol::EUR, "EUR" };
constexpr struct unit USD = { symbol::USD, "USD" };
constexpr struct unit UNKNOWN = { symbol::UNKNOWN, "UNKNOWN" };

constexpr struct unit match[] = { EUR, USD };

}
} /* namespace ibkr */

#endif /* SOURCE_CURRENCY_H_ */
