/*
 * pnl.hpp
 *
 *  Created on: 15.06.2022
 *      Author: AndreasFellnhofer
 */

#ifndef SOURCE_PNL_HPP_
#define SOURCE_PNL_HPP_

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

struct inout_data
{
	int year = 1900;
	std::map <std::string, std::shared_ptr<ibkr::tranche>> map_trades;
	std::map <std::string, std::shared_ptr<ibkr::tranche>> map_forex;
	std::map <std::string, std::shared_ptr<ibkr::tranche>> map_forex_lut;
	std::set <ibkr::currency::unit> foreign_currencies;
	std::map <ibkr::currency::unit, double> balances;
	std::map <ibkr::currency::unit, double> balances_in_eur;
	std::map <ibkr::currency::unit, double> avg_rate;
	std::map <ibkr::currency::unit, double> balances_profit;
	std::map <ibkr::currency::unit, double> balances_losses;
};


std::string construct_key(std::tm const &tm, char const *security_name, int cnt);
void long_and_short_fraction(double balance, double delta, double &long_frac, double &short_frac);


} /* namespace pnl */

}
#endif /* SOURCE_PNL_HPP_ */
