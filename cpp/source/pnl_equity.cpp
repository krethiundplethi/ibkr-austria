
#include "pnl_equity.hpp"
#include "pnl.hpp"

#include "tranche.hpp"
#include "currency.hpp"

#include <ctime>
#include <string>
#include <set>
#include <vector>
#include <algorithm>
#include <iterator>


namespace ibkr
{

namespace pnl
{



/** Walk all data and print PnL tax report.
 * Is it pretty? nope.
 * Does it need to be pretty? nope.
 */
void equity_calc(
	const ibkr::currency::unit &currency,
	double &overall_profit,
	double &overall_losses,
	inout_data &data
)
{
	std::string prev_symbol;
	ibkr::currency::unit prev_unit;
	std::map<std::string, double> balances;
	std::map<std::string, double> balances_losses;
	std::map<std::string, double> balances_profit;
	std::map<std::string, double> balances_in_eur;
	std::map<std::string, double> avg_rate;

	std::vector<std::shared_ptr<ibkr::tranche>> tranches;
	for (auto const &elem: data.map_trades)
	{
		tranches.push_back(elem.second);
	}
	std::sort(tranches.begin(), tranches.end(), tranche_compare);

	for (std::vector<std::shared_ptr<ibkr::tranche>>::iterator it = tranches.begin(); it != tranches.end(); ++it)
	{
		ibkr::tranche &t = *(*it);
		std::string symbol = t.getSecurity().getName();

		if ((t.getSecurity().getType() != ibkr::security::EQUITY) || (t.getPrice().unit.id != currency.id))
		{
			continue;
		}
		else if (symbol == "LAC")
		{
			printf("");
		}

		if (prev_symbol != t.getSecurity().getName())
		{
			printf("Symbol Datum      1   Menge Kurs(%s)  Preis(EUR) Bestand   Kumm(EUR)    glD(EUR)      Ansatz       GuV      Gewinn     Verlust ", t.getSecurity().getPrice().unit.name);
			printf("  SummGuV  SummGewinn SummVerlust");
			printf("\n");
		}

		if (balances.find(symbol) == balances.end())
		{
			avg_rate[symbol] = 0.0;
			balances[symbol] = 0.0;
			balances_losses[symbol] = 0.0;
			balances_profit[symbol] = 0.0;
		}

		std::tm tm = t.getTimeStamp();

		double eur_paid = 0.0;
		double forex_exch = 1.0; /* default for EUR 1:1 */
		int pieces = 0;

		int cnt = 0;
		bool finished = false;
		t.unfill();

		if ((t.getSecurity().getPrice().unit == currency::EUR) ||
			(t.getSecurity().getPrice().value < 0.00001) ||
			(t.getType() == tranche::HOLD)
		)
		{
			eur_paid = t.getPrice();
			pieces = t.getQuanti();
		}
		else
		{
			std::string key;
			while (cnt <= 99)
			{
				key = construct_key(tm, symbol.c_str(), cnt);
				auto it = data.map_forex_lut.find(key);
				if (it == data.map_forex_lut.end())
				{
					break;
				}
				else if ((it->second->unfilled() >= 0.01) && (t.unfilled() >= 0.01))
				{
					/* unfilled order, filled by forex transaction */
					double eur = it->second->getPrice();
					double stock_paid = it->second->getQuanti() * t.getSecurity().getPrice();
					double stock_fee = (it->second->getPrice() / it->second->getSecurity().getPrice() - stock_paid) * (it->second->isSell() ? 1.00 : -1.00);
					double eur_fee = 0.0;

					if (t.isSell() && it->second->isSell())
					{
						printf("ERROR! Equity sell but not a Forex buy ??");
					}

					if (it->second->isSell()) /* Forex sell = equity buy */
					{
						eur_fee = stock_fee * it->second->getPrice() / (stock_paid + stock_fee);
						eur_paid += eur - eur_fee;
					}
					else
					{
						eur_fee = stock_fee * it->second->getPrice() / (stock_paid - stock_fee);
						eur_paid += eur + eur_fee;
					}

					int fill = it->second->getQuanti();
					t.fill(fill);
					it->second->fill(fill);
					pieces += fill;
				}
				else if (t.unfilled() <= 0.01)
				{
					break;
				}
				cnt++;
			}

		}
		if (t.getQuanti() != pieces)
		{
			printf("****warning: mismatching pieces: %d\n", pieces);
		}


		/* this part is complete copy/paste from forex --> refactoring --> BIG TODO */
		double old_balance = balances[symbol];
		double old_balance_eur = balances_in_eur[symbol];
		double long_frac = 0.0;
		double short_frac = 0.0;

		long_and_short_fraction(balances[symbol], pieces * (t.isSell() ? -1.00 : 1.00), long_frac, short_frac);
		balances[symbol] += pieces * (t.isSell() ? -1.00 : 1.00);

		/* Hier wird der gleitendende Durchschnitt mittels old_balance angesetzt.
		 * Slightly more complicated than thought initially, because being short,
		 * the logic is inverted. Selling updates running average, buying causes PnL.
		 * */

		if (t.getType() == tranche::HOLD)
		{
			avg_rate[symbol] = eur_paid / pieces;
			balances_in_eur[symbol] += eur_paid;
		}
		else if (!t.isSell())
		{
			/* whenever I get currency, the current EUR value is used, not the averaged one */
			avg_rate[symbol] = (avg_rate[symbol] * old_balance + eur_paid) / (old_balance + pieces);
			balances_in_eur[symbol] += long_frac * eur_paid;
			if (old_balance != 0.0)
			{
				balances_in_eur[symbol] += short_frac * (pieces * old_balance_eur / old_balance);
			}
		}
		else
		{
			balances_in_eur[symbol] -= short_frac * eur_paid;
			if ((old_balance > 0.0) && (balances[symbol] < 0.0))
			{
				printf("****warning: Think this is a bug. Selling from long to short without recalculating avg_rate!\n");
			}

			if (old_balance != 0.0)
			{
				balances_in_eur[symbol] -= long_frac * (pieces * old_balance_eur / old_balance);
			}
		}

		double guv = 0.0;
		double ansatz = 0.0;

		if (t.getType() == tranche::HOLD)
		{
			ansatz = eur_paid;
		}
		else if (t.isSell())
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

		if (guv > 0) balances_profit[symbol] += guv;
		else balances_losses[symbol] += guv;

		printf("%-6s ", symbol.c_str()); /* symbol */
		printf("%4d-%02d %02d ",  data.year, tm.tm_mon + 1, tm.tm_mday); /* datum */
		printf("%-1s ", t.getType() == tranche::HOLD ? "H" : t.isSell() ? "V" : "K");
		printf("%7.0f %9.3f ", t.getQuanti() * (t.isSell() ? -1.0 : 1.0), t.getSecurity().getPrice().value); /* Menge Kurs */
		printf("%11.2f %7.0f ", eur_paid, balances[symbol]); /* Preis Bestand */

		if (balances_in_eur.find(symbol) == balances_in_eur.end()) balances_in_eur[symbol] = 0.0;

		printf("%11.2f ", balances_in_eur[symbol]); /* Kumm(EUR) */
		if ((balances[symbol] > 0.0001) || (balances[symbol] < -0.0001))
			printf("%11.5f ", balances_in_eur[symbol] / balances[symbol]); /* glD */
		else
			printf("%11.5f ",  old_balance_eur / old_balance);

		printf("%11.2f %9.2f ", ansatz, guv); /* ansatz, GuV*/
		printf("%11.2f %11.2f", balances_profit[symbol], balances_losses[symbol]); /* Gewinn, Verlust */

		printf("\n");
		fflush(stdout);

		prev_symbol = t.getSecurity().getName();
		prev_unit = t.getSecurity().getPrice().unit;

		/* look ahead */
		std::vector<std::shared_ptr<ibkr::tranche>>::iterator next = it;
		++next;
		if (
			(next == tranches.end()) || \
			/* e.g. tricky. Some equity names (esp. CAD) trade in other currencies as well */
			(((*next)->getSecurity().getName() != prev_symbol) || ((*next)->getSecurity().getPrice().unit != prev_unit)) \
		)
		{
			overall_losses += balances_losses[prev_symbol];
			overall_profit += balances_profit[prev_symbol];
			printf("%-6s ", symbol.c_str());
			printf("%4d-12 31 = ======= ========= =========== %7.0f ", data.year, balances[symbol] ); /* menge kurs preis */

			printf("%11.2f ", balances_in_eur[symbol]); /* Kumm(EUR) */
			if ((balances[symbol] > 0.0001) || (balances[symbol] < -0.0001))
				printf("%11.5f ", balances_in_eur[symbol] / balances[symbol]); /* glD */
			else
				printf("%11.5f ",  old_balance_eur / old_balance);

			printf("=========== ");
			printf("%9.2f %11.2f %11.2f ",
					balances_profit[symbol] + balances_losses[symbol], balances_profit[symbol], balances_losses[symbol]);
			printf("%9.2f %11.2f %11.2f\n", overall_profit + overall_losses, overall_profit, overall_losses);
		}
	}
}

}
} /* namespace ibkr */



