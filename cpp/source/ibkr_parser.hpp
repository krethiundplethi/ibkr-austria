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
#include <memory>
#include <ctime>


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

typedef void (*callback_function)(const std::tm &tm, std::unique_ptr<tranche> &tr);

class ibkr_parser {

public:
	ibkr_parser() = delete;
	ibkr_parser(std::string fname) : fname{fname}, istream(fname) { cbk_stock = cbk_forex = cbk_options = nullptr; };

	void parse(void);

	void register_callback_on_stock(callback_function cbk) { cbk_stock = cbk; };
	void register_callback_on_forex(callback_function cbk) { cbk_forex = cbk; };
	void register_callback_on_options(callback_function cbk) { cbk_options = cbk; };

	virtual ~ibkr_parser() {};

private:
	std::string fname;
	std::ifstream istream;
	callback_function cbk_stock;
	callback_function cbk_forex;
	callback_function cbk_options;
};

} /* namespace ibkr */

#endif /* SOURCE_IBKR_PARSER_HPP_ */
