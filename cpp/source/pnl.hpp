/*
 * pnl.hpp
 *
 *  Created on: 15.06.2022
 *      Author: AndreasFellnhofer
 */

#ifndef SOURCE_PNL_HPP_
#define SOURCE_PNL_HPP_

#include "rate_parser.hpp"
#include "tranche.hpp"
#include "currency.hpp"
#include <map>
#include <set>
#include <string>
#include <memory>


namespace ibkr
{

namespace pnl
{

struct performance
{
	double profit;
	double loss;
};


struct inout_data
{
	int year = 1900;
	RateParser rates;
	std::map <std::string, std::shared_ptr<ibkr::tranche>> map_trades;
	std::map <std::string, std::shared_ptr<ibkr::tranche>> map_forex;
	std::map <std::string, std::shared_ptr<ibkr::tranche>> map_forex_lut;
	std::set <ibkr::currency::unit> foreign_currencies;
	std::map <ibkr::currency::unit, double> balances;
	std::map <ibkr::currency::unit, double> balances_in_eur;
	std::map <ibkr::currency::unit, double> balances_profit;
	std::map <ibkr::currency::unit, double> balances_losses;
};


std::string construct_key(std::tm const &day, char const *security_name, char const *currency, int cnt);
bool long_and_short_fraction(double balance, double delta, double &long_frac, double &short_frac);


} /* namespace pnl */

}
#endif /* SOURCE_PNL_HPP_ */
