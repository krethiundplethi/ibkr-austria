#ifndef SOURCE_PNL_EQUITY_HPP_
#define SOURCE_PNL_EQUITY_HPP_

#include "pnl.hpp"
#include "tranche.hpp"
#include "currency.hpp"
#include <set>
#include <map>
#include <memory>

namespace ibkr
{

namespace pnl
{


void equity_calc(
	const enum ibkr::security::type security_type,
	const ibkr::currency::unit &currency,
	double &g_overall_profit,
	double &g_overall_losses,
	inout_data &data
);


} /* namespace pnl */

}
#endif
