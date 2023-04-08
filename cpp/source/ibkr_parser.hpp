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
	ibkr_parser(std::string fname) : fname{fname}, istream(fname)
	{
		cbk_initial_holding = cbk_stock_trade = cbk_options_trade = cbk_forex_trade = cbk_forex = nullptr;
	};

	void parse(void);

	void register_callback_on_initial_holding(callback_function cbk) { cbk_initial_holding = cbk; };
	void register_callback_on_stock_trade(callback_function cbk) { cbk_stock_trade = cbk; };
	void register_callback_on_forex_trade(callback_function cbk) { cbk_forex_trade = cbk; };
	void register_callback_on_options_trade(callback_function cbk) { cbk_options_trade = cbk; };
	void register_callback_on_forex(callback_function cbk) { cbk_forex = cbk; };

	virtual ~ibkr_parser() {};

private:
	std::string fname;
	std::ifstream istream;
	callback_function cbk_stock_trade;
	callback_function cbk_options_trade;
	callback_function cbk_forex_trade;
	callback_function cbk_forex;
	callback_function cbk_initial_holding;
};

} /* namespace ibkr */

#endif /* SOURCE_IBKR_PARSER_HPP_ */
