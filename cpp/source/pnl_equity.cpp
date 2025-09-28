
#include "pnl_equity.hpp"
#include "pnl.hpp"

#include "tranche.hpp"
#include "currency.hpp"

#include <cmath> 
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>
#include <algorithm>


namespace ibkr
{

namespace pnl
{



/** Walk all data and print PnL tax report.
 * Is it pretty? nope.
 * Does it need to be pretty? nope.
 */
void equity_calc(
	std::FILE *stream,
	const enum ibkr::security::type security_type,
	const ibkr::currency::unit &currency,
	struct performance &pnl,
	inout_data &data
)
{
	std::string prev_symbol;
	ibkr::currency::unit prev_unit;
	std::map<std::string, double> balances;
	std::map<std::string, double> balances_fees;
	std::map<std::string, double> balances_losses;
	std::map<std::string, double> balances_profit;
	std::map<std::string, double> balances_in_eur;
	double currency_fees = 0.0;
	double currency_guv = 0.0;
	bool currency_found = false;
	bool stock_split_finalize = false;

	std::vector<std::shared_ptr<ibkr::tranche>> tranches;
	for (auto const &elem: data.map_trades)
	{
		tranches.push_back(elem.second);
	}
	std::sort(tranches.begin(), tranches.end(), tranche_compare);

	for (std::vector<std::shared_ptr<ibkr::tranche>>::iterator it = tranches.begin(); it != tranches.end(); ++it)
	{
		ibkr::tranche &tran = *(*it);
		std::string symbol = tran.getSecurity().getName();

		if ((tran.getSecurity().getType() != security_type) ||
			(tran.getPrice().unit.id != currency.id))
		{
			continue;
		}
		
		if (symbol.find("CYBN") != std::string::npos)
		{
			std::fprintf(stream, "");
		}
		else
		{
			currency_found = true;
		}

		if (prev_symbol != tran.getSecurity().getName())
		{
			std::fprintf(stream, "Symbol               ");
			std::fprintf(stream, "Datum      1   Menge Kurs(%s)  Preis(EUR) Gebühr Bestand    Kumm(EUR)    glD(EUR)  Ansatz(EUR)     GuV(EUR)  Gewinn(EUR) Verlust(EUR)", tran.getSecurity().getPrice().unit.name);
			std::fprintf(stream, "      SummGuV   SummGewinn  SummVerlust");
			std::fprintf(stream, "\n");
		}

		stock_split_finalize = false;
		const std::string split_symbol = symbol + "--split"; /* special intermedia balance for stock split */
		if (tran.isSplit())
		{
			if (balances.find(split_symbol) == balances.end())
			{
				balances_in_eur[split_symbol] = balances_in_eur[symbol];
				symbol = split_symbol;
			}
			else
			{
				stock_split_finalize = true;
			}
		}

		if (balances.find(symbol) == balances.end())
		{
			balances[symbol] = 0.0;
			balances_fees[symbol] = 0.0;
			balances_losses[symbol] = 0.0;
			balances_profit[symbol] = 0.0;
		}

		const std::tm time = tran.getTimeStamp();

		double eur_paid = 0.0;
		double eur_fee = 0.0;
		double pieces = 0;

		int cnt = 0;
		tran.unfill();

		if ((tran.getSecurity().getPrice().unit == currency::EUR) ||
			(tran.getSecurity().getPrice().value < 0.00001) ||
			(tran.getType() == tranche::HOLD)
		)
		{
			eur_paid = tran.getPrice();
			eur_fee = tran.getFee();
			pieces = tran.getQuanti();
		}
		else
		{
			std::string key;
			while ((tran.unfilled() > 0.01) && (cnt <= 99))
			{
				key = construct_key(time, symbol.c_str(), currency.name, cnt);
				auto it = data.map_forex_lut.find(key);
				if (it == data.map_forex_lut.end())
				{
					printf("****warning: Key %s not found\n", key.c_str());
					pieces = tran.getQuanti();
					eur_paid += pieces * tran.getSecurity().getPrice() / tran.getEcbRate(); /* Use tax correct ECB rates. */
					eur_fee = 0.0;
					tran.fill(pieces);
					break;
				}
				
				if ((it->second->unfilled() >= 0.01) && (tran.unfilled() >= 0.01))
				{
					/* unfilled order, filled by forex transaction */
					//double eur = it->second->getPrice();
					const double stock_paid = it->second->getQuanti() * tran.getSecurity().getPrice();
					const double stock_fee = (it->second->getPrice() / it->second->getSecurity().getPrice() - stock_paid) * (it->second->isSell() ? 1.00 : -1.00);
					//double eur_fee_ = 0.0;

					if (tran.isSell() == it->second->isSell())
					{
						printf("ERROR! Inconsistent Equity/Forex buy/sell");
					}

					eur_paid += stock_paid / tran.getEcbRate(); /* Use tax correct ECB rates. */
					eur_fee += stock_fee / tran.getEcbRate();

					const double fill = it->second->getQuanti();
					tran.fill(fill);
					it->second->fill(fill);
					pieces += fill;
				}
				cnt++;
			}

		}
		if (std::abs(tran.getQuanti() - pieces) > 0.000000001)
		{
			printf("****warning: mismatching pieces: %f from tranche: %f\n", pieces, tran.getQuanti());
		}


		/* this part is complete copy/paste from forex --> refactoring --> BIG TODO */
		double old_balance = balances[symbol];
		double old_balance_eur = balances_in_eur[symbol];
		double long_frac = 0.0;
		double short_frac = 0.0;

		const bool warning = !long_and_short_fraction(balances[symbol], pieces * (tran.isSell() ? -1.00 : 1.00), long_frac, short_frac);
		if (warning) { 
			printf("Warning @%d.%d symbol: %s \n", time.tm_mday, time.tm_mon, symbol.c_str()); 
		};
		balances[symbol] += pieces * (tran.isSell() ? -1.00 : 1.00);

		/* Hier wird der gleitendende Durchschnitt mittels old_balance angesetzt.
		 * Slightly more complicated than thought initially, because being short,
		 * the logic is inverted. Selling updates running average, buying causes PnL.
		 * */

		if (tran.getType() == tranche::HOLD)
		{
			balances_in_eur[symbol] += eur_paid;
		}
		else if (!tran.isSell())
		{
			/* whenever I get currency, the current EUR value is used, not the averaged one */
			balances_in_eur[symbol] += long_frac * eur_paid;
			if (old_balance != 0.0)
			{
				balances_in_eur[symbol] += short_frac * (pieces * old_balance_eur / old_balance);
			}
		}
		else
		{
			balances_in_eur[symbol] -= short_frac * eur_paid;

			if (old_balance != 0.0)
			{
				balances_in_eur[symbol] -= long_frac * (pieces * old_balance_eur / old_balance);
			}
		}

		double guv = 0.0;
		double ansatz = 0.0;

		if (tran.isHold() || tran.isSplit())
		{
			ansatz = eur_paid;
		}
		else if (tran.isSell())
		{
			ansatz = -short_frac * eur_paid;
			if (old_balance != 0.0)
			{
				guv = long_frac * (eur_paid - pieces * old_balance_eur / old_balance);
				ansatz -= long_frac * (pieces * old_balance_eur / old_balance);
			}
		}
		else
		{
			ansatz = long_frac * eur_paid;
			if (old_balance != 0.0)
			{
				guv = short_frac * (pieces * old_balance_eur / old_balance - eur_paid);
				ansatz += short_frac * (pieces * old_balance_eur / old_balance);
			}
		}

		if (guv > 0) { balances_profit[symbol] += guv; }
		else { balances_losses[symbol] += guv; }

		currency_guv += guv;
		balances_fees[symbol] += eur_fee;
		currency_fees += eur_fee;

		std::fprintf(stream, "%-20s ", symbol.c_str()); /* symbol */
		std::fprintf(stream, "%4d-%02d %02d ",  data.year, time.tm_mon + 1, time.tm_mday); /* datum */
		std::fprintf(stream, "%-1s ", tran.getType() == tranche::HOLD ? "H" : tran.isSell() ? "V" : "K");
		std::fprintf(stream, "%7.0f %9.3f ", tran.getQuanti() * (tran.isSell() ? -1.0 : 1.0), tran.getSecurity().getPrice().value); /* Menge Kurs */
		std::fprintf(stream, "%11.2f %6.2f %7.0f ", eur_paid, eur_fee, balances[symbol]); /* Preis Gebühr Bestand */

		if (stock_split_finalize)
		{
			if (balances_in_eur[symbol] == balances_in_eur[split_symbol])
			{
				printf("WARNING: Suspicious split 1:1");
			}
			balances_in_eur[symbol] = balances_in_eur[split_symbol];
			balances[symbol] = balances[split_symbol];
		}

		if (balances_in_eur.find(symbol) == balances_in_eur.end()) {
			balances_in_eur[symbol] = 0.0;
		}

		std::fprintf(stream, "%12.2f ", balances_in_eur[symbol]); /* Kumm(EUR) */

		if ((balances[symbol] > 0.0001) || (balances[symbol] < -0.0001)) {
			std::fprintf(stream, "%11.5f ", balances_in_eur[symbol] / balances[symbol]); /* glD */
		} 
		else {
			std::fprintf(stream, "%11.5f ",  old_balance_eur / old_balance);
		}

		std::fprintf(stream, "%12.2f %12.2f ", ansatz, guv); /* ansatz, GuV */
		std::fprintf(stream, "%12.2f %12.2f", balances_profit[symbol], balances_losses[symbol]); /* Gewinn, Verlust */

		std::fprintf(stream, "\n");
		std::fflush(stream);

		prev_symbol = tran.getSecurity().getName();
		prev_unit = tran.getSecurity().getPrice().unit;

		/* look ahead */
		std::vector<std::shared_ptr<ibkr::tranche>>::iterator next = it;
		++next;
		if (
			(next == tranches.end()) || \
			/* e.g. tricky. Some equity names (esp. CAD) trade in other currencies as well */
			(((*next)->getSecurity().getName() != prev_symbol) || ((*next)->getSecurity().getPrice().unit != prev_unit)) \
		)
		{
			pnl.loss += balances_losses[prev_symbol];
			pnl.profit += balances_profit[prev_symbol];
			std::fprintf(stream, "%-20s ", symbol.c_str());
			std::fprintf(stream, "%4d-12 31 = ======= ========= =========== %6.2f %7.0f ", data.year, balances_fees[symbol], balances[symbol] ); /* menge kurs preis */

			std::fprintf(stream, "%12.2f ", balances_in_eur[symbol]); /* Kumm(EUR) */

			if ((balances[symbol] > 0.0001) || (balances[symbol] < -0.0001))
			{
				std::fprintf(stream, "%11.5f ", balances_in_eur[symbol] / balances[symbol]); /* glD */
			}
			else
			{
				std::fprintf(stream, "%11.5f ",  old_balance_eur / old_balance);
			}

			std::fprintf(stream, "============ ");
			std::fprintf(stream, "%12.2f %12.2f %12.2f ",
					balances_profit[symbol] + balances_losses[symbol], balances_profit[symbol], balances_losses[symbol]);
			std::fprintf(stream, "%12.2f %12.2f %12.2f\n", pnl.profit + pnl.loss, pnl.profit, pnl.loss);

		}
	}

	if (currency_found)
	{
		std::fprintf(stream, "                     "); /* symbol */
		std::fprintf(stream, "%4d-12 31 = ======= == %s == =========== ", data.year, currency.name);
		std::fprintf(stream, "%6.2f                                             %9.2f", currency_fees, currency_guv);
		std::fprintf(stream, "\n");
	}
}

}
} /* namespace ibkr */


