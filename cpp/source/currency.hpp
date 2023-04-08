/*
 * currency.h
 *
 *  Created on: 27.03.2022
 *      Author: AndreasFellnhofer
 */

#ifndef SOURCE_CURRENCY_H_
#define SOURCE_CURRENCY_H_

#include <iostream>
#include "string.h"

namespace ibkr
{

namespace currency
{

enum class symbol {
	EUR,
	USD,
	AUD,
	CAD,
	GBP,
	RUB,
	CHF,
	SGD,
	SEK,
	JPY,
	UNKNOWN
};


struct unit
{
	symbol id;
	const char *name;

	bool operator<(const unit &r) const { return id < r.id; }
	bool operator==(const unit &r) const { return id == r.id; }
	bool operator!=(const unit &r) const { return !(*this == r); }

};


struct price
{
	struct unit unit;
	double value;

	operator double() const { return value; }
	price operator+(price const &rop) const { return { unit, value + rop.value }; }
	price operator-(price const &rop) const { return { unit, value - rop.value }; }
};


unit from_symbol(const symbol s);


std::ostream &operator<<(std::ostream &, const currency::price &);
std::ostream &operator<<(std::ostream &, const currency::unit &);

#define MAKE_CURRENCY_STRUCT(x) constexpr struct unit x = { symbol::x, #x}

MAKE_CURRENCY_STRUCT(EUR);
MAKE_CURRENCY_STRUCT(USD);
MAKE_CURRENCY_STRUCT(AUD);
MAKE_CURRENCY_STRUCT(CAD);
MAKE_CURRENCY_STRUCT(GBP);
MAKE_CURRENCY_STRUCT(RUB);
MAKE_CURRENCY_STRUCT(CHF);
MAKE_CURRENCY_STRUCT(SGD);
MAKE_CURRENCY_STRUCT(SEK);
MAKE_CURRENCY_STRUCT(JPY);
MAKE_CURRENCY_STRUCT(UNKNOWN);

constexpr struct unit match[] = { EUR, USD, AUD, CAD, RUB, GBP, CHF, SGD, SEK, JPY, UNKNOWN };

}
} /* namespace ibkr */

#endif /* SOURCE_CURRENCY_H_ */
