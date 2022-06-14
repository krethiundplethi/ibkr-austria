#ifndef SOURCE_PNL_FOREX_HPP_
#define SOURCE_PNL_FOREX_HPP_

#include "tranche.hpp"
#include "currency.hpp"
#include <set>
#include <map>
#include <memory>


namespace pnl
{

struct inout_data
{
	int year = 2021;
	std::map <std::string, std::unique_ptr<ibkr::tranche>> map_trades;
	std::map <std::string, std::unique_ptr<ibkr::tranche>> map_forex;
	std::set <ibkr::currency::unit> foreign_currencies;
	std::map <ibkr::currency::unit, double> balances;
	std::map <ibkr::currency::unit, double> balances_in_eur;
	std::map <ibkr::currency::unit, double> avg_rate;
	std::map <ibkr::currency::unit, double> balances_profit;
	std::map <ibkr::currency::unit, double> balances_losses;
};


void forex_calc(
	const ibkr::currency::unit &currency,
	double &g_overall_profit,
	double &g_overall_losses,
	inout_data &data
);


} /* namespace pnl */

#endif
