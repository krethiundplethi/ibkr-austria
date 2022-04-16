/*
 * ibkr_parser.h
 *
 *  Created on: 16.04.2022
 *      Author: AndreasFellnhofer
 */

#ifndef SOURCE_IBKR_PARSER_HPP_
#define SOURCE_IBKR_PARSER_HPP_

#include <iostream>
#include <sstream>
#include <string>
#include <fstream>


namespace ibkr {


enum class trade_type {
	STOCKS,
	FOREX,
	OPTIONS,
	UNKNOWN
};


struct trade_parse_s {
	const trade_type t;
	const char *match;
};


constexpr trade_parse_s trade_parse[] =
{
	{ trade_type::STOCKS, "Stocks" },
	{ trade_type::FOREX, "Forex" },
	{ trade_type::OPTIONS, "Equity and Index Options" },
};


class ibkr_parser {

public:
	ibkr_parser() = delete;
	ibkr_parser(std::string fname) : fname{fname}, istream(fname) {};

	void parse(void);

	virtual ~ibkr_parser() {};

private:
	std::string fname;
	std::ifstream istream;
};

} /* namespace ibkr */

#endif /* SOURCE_IBKR_PARSER_HPP_ */
