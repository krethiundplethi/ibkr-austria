/*
 * ibkr_parser.h
 *
 *  Created on: 16.04.2022
 *      Author: AndreasFellnhofer
 */

#ifndef SOURCE_IBKR_PARSER_HPP_
#define SOURCE_IBKR_PARSER_HPP_

#include "tranche.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <fstream>


namespace ibkr {

namespace trade {
enum class type {
	STOCKS,
	FOREX,
	OPTIONS,
	UNKNOWN
};


struct unit {
	type id;
	const char *name;
};

constexpr unit match[] =
{
	{ trade::type::STOCKS, "Stocks" },
	{ trade::type::FOREX, "Forex" },
	{ trade::type::OPTIONS, "Equity and Index Options" },
};

}

typedef void (*callback_function)(tranche tr);

class ibkr_parser {

public:
	ibkr_parser() = delete;
	ibkr_parser(std::string fname) : fname{fname}, istream(fname) { cbk_stock = nullptr; };

	void parse(void);

	void register_callback_on_stock(callback_function cbk) { cbk_stock = cbk; };

	virtual ~ibkr_parser() {};

private:
	std::string fname;
	std::ifstream istream;
	callback_function cbk_stock;
};

} /* namespace ibkr */

#endif /* SOURCE_IBKR_PARSER_HPP_ */
