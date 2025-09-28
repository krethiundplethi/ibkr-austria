#ifndef SOURCE_PNL_FOREX_HPP_
#define SOURCE_PNL_FOREX_HPP_

#include "pnl.hpp"
#include "tranche.hpp"
#include "currency.hpp"
#include <set>
#include <map>
#include <memory>
#include <cstdio>


namespace ibkr
{

namespace pnl
{

void forex_calc(
	std::FILE *stream,
	const ibkr::currency::unit &currency,
	struct performance  &pnl,
	inout_data &data
);


} /* namespace pnl */

}

#endif
