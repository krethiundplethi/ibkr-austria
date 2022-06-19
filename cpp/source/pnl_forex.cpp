
#include "pnl_forex.hpp"

#include "tranche.hpp"
#include "currency.hpp"


namespace {

void print_fee(int year, int mon, int day, double fx, double fx_bal, double eur, double eur_bal)
{
	/*       2022-07 ; Day   ; Symbol  ; K/V/WP ; */
	printf("%4d-%02d %02d FEE     F      ",  year, mon, day);
	/*      USD     ; Bestand ; ; Soll EUR ; Haben EUR ; ; EUR gl.D.s. ;   Kurs    ; ; Ansatz EUR ; GuV ; Gebühren*/
	printf("%11.02f %11.02f %10.02f            %11.02f %10.06f ", -fx, fx_bal, eur, eur_bal, eur_bal / fx_bal);
}



const char *transaction_code(const ibkr::tranche &t)
{
	/* note buying equity == selling currency */
	if (t.getSecurity().getType() == ibkr::security::EQUITY)
	{
		if (t.isSell()) return "KWP";
		else return "VWP";
	}
	else if (t.getSecurity().getType() == ibkr::security::OPTION)
	{
		if (t.isSell()) return "KOPT";
		else return "VOPT";
	}
	else
	{
		if (t.isSell()) return "KEUR";
		else return "VEUR";
	}
}

} /* unnamed namespace */


namespace ibkr
{

namespace pnl
{


/** Walk all data and print PnL tax report.
 * Is it pretty? nope.
 * Does it need to be pretty? nope.
 */
void forex_calc(
	const ibkr::currency::unit &currency,
	double &overall_profit,
	double &overall_losses,
	inout_data &data
)
{
	for (int month = 0; month < 12; ++month)
	{
		printf("%4d-%02d D  Symbol  K/V/WP %11s "
			   "    Bestand  Soll(EUR) Haben(EUR)    EUR(glD) "
			   "      Kurs Basis(EUR)     GuV(EUR)  Gewinn(EUR) Verlust(EUR)\n", data.year, month + 1, currency.name);

		for (auto const &elem: data.map_forex)
		{
			ibkr::tranche t = static_cast<ibkr::tranche &>(*elem.second);
			std::tm tm = t.getTimeStamp();

			if ((t.getPrice().unit.id == currency.id) && (tm.tm_mon == month))
			{
				if (t.getSecurity().getName() == "NIC")
				{
					printf("");
				}

				auto it = data.map_trades.end();
				bool found = false;

				std::string key_prefix = elem.first.substr(0, 6) + t.getSecurity().getName();
				//key = key.substr(0, 18);

				int cnt = 0;
				std::string key = key_prefix + "-" + std::to_string(cnt);
				while ((!found) && (cnt < 99))
				{
					it = data.map_trades.find(key);
					if (it != data.map_trades.end())
					{
						if (it->second->unfilled() >= 0.01)
						{
							found = true;
						}
					}
					key = key_prefix + "-" + std::to_string(++cnt);
				}

				double stock_paid = 0.0;
				double stock_fee = 0.0;
				double eur_paid = t.getPrice();
				double eur_fee = 0;

				if (!found)
				{
					//printf("** WARNING: No corresponding %s order found **\n", key.c_str());
					/* fixme: calculating back from EUR to the foreign currency is odd.
					 * Is needed because for equity the "quantity" is not the pricetag but the amout of stock. */
					stock_paid = t.getPrice() / t.getSecurity().getPrice();
				}
				else if (t.getSecurity().getType() != ibkr::security::CURRENCY)
				{
					/* stock_paid = t.getQuanti() * found->second->getSecurity().getPrice().value;
					 * bug: this does not compute.
					 * When shorting, IBKR has fee for each partial fill of an order.
					 * Workaround: Use the original USD amounts, and add the complete
					 * fee to the first transaction. It is not 100% correct,
					 * as the first part already changes the running average...
					 * but better than no tax statement at all...
					 * Is a FIXME for now.
					 */

					ibkr::tranche &order = *(it->second);

					order.fill(t.getQuanti());

					/* acshually we are only doing this to get the fee on the transaktion,
					 * thats it. All other information is already in the forex table entry. */
					stock_paid = t.getQuanti() * order.getSecurity().getPrice();
					stock_fee = (t.getPrice() / t.getSecurity().getPrice() - stock_paid) * (t.isSell() ? 1.00 : -1.00);
					//stock_paid = t.isSell() ? stock_paid - stock_fee : stock_paid + stock_fee;

					/* booking in eur is a bit tricky here because fees need to be extracted. */
					eur_fee = stock_fee * eur_paid / (stock_paid + stock_fee);
					if (t.isSell())
					{
						/* buying equity ... fee is already considered */
						eur_paid -= eur_fee;
					}
					else
					{
						eur_paid += eur_fee;
					}
				}
				else /* CURRENCY */
				{
					ibkr::tranche &order = *(it->second);

					/* Für devisen gilt Menge = Wert */
					stock_paid = t.getQuanti();
					if (!t.isSell())
					{
						eur_paid -= order.getFee();
					}
				}


				double old_balance = data.balances[currency];
				double old_balance_eur = data.balances_in_eur[currency];
				double long_frac = 0.0;
				double short_frac = 0.0;

				long_and_short_fraction(data.balances[currency], stock_paid * (t.isSell() ? -1.00 : 1.00), long_frac, short_frac);
				data.balances[currency] += stock_paid * (t.isSell() ? -1.00 : 1.00);

				/* Hier wird der gleitendende Durchschnitt mittels old_balance angesetzt.
				 * Slightly more complicated that thought initially, because being short,
				 * the logic is inverted. Selling updates running average, buying causes PnL.
				 * */
				if (!t.isSell())
				{
					/* whenever I get currency, the current EUR value is used, not the averaged one */
					data.avg_rate[currency] = (data.avg_rate[currency] * old_balance + eur_paid) / (old_balance + stock_paid);
					data.balances_in_eur[currency] += long_frac * eur_paid;
					if (old_balance != 0.0)
					{
						data.balances_in_eur[currency] += short_frac * (stock_paid * old_balance_eur / old_balance);
					}
				}
				else
				{
					data.balances_in_eur[currency] -= short_frac * eur_paid;
					if (old_balance != 0.0)
					{
						data.balances_in_eur[currency] -= long_frac * (stock_paid * old_balance_eur / old_balance);
					}
				}

				double guv = 0.0;
				double ansatz = 0.0;

				if (t.isSell())
				{
					ansatz = -short_frac * eur_paid;
					if (old_balance != 0.0)
					{
						guv = long_frac * (eur_paid - stock_paid * old_balance_eur / old_balance);
						ansatz -= long_frac * (stock_paid * old_balance_eur / old_balance);
					}
				}
				else
				{
					ansatz = long_frac * eur_paid;
					if (old_balance != 0.0)
					{
						guv = short_frac * (stock_paid * old_balance_eur / old_balance - eur_paid);
						ansatz += short_frac * (stock_paid * old_balance_eur / old_balance);
					}
				}

				if (guv > 0) data.balances_profit[currency] += guv;
				else data.balances_losses[currency] += guv;

				printf("%4d-%02d %02d ",  data.year, month+1, tm.tm_mday);
				printf("%-7s ", t.getSecurity().getName().c_str());
				printf("%-6s ", transaction_code(t));

				printf("%11.02f ", stock_paid * (t.isSell() ? -1.00 : 1.00)); /* Fremdw  */
				printf("%11.02f ", data.balances[currency]); 					/* Bestand */
				printf(t.isSell() ? "%10.02f            " :				/* Soll */
									"           %10.02f ", eur_paid);		/* Haben */
				printf("%11.02f ", data.balances_in_eur[currency]); 				/* Bestand EUR */
				printf("%10.06f ", data.balances_in_eur[currency] / data.balances[currency]); 		/* Kurs */
				printf("%10.02f ", ansatz); 		/* Ansatz */
				printf("%12.05f ", guv); 		/* GuV */
				printf("%12.05f ", data.balances_profit[currency]); 		/* Gewinn kum */
				printf("%12.05f ", data.balances_losses[currency]); 		/* Verlust kum */
				printf("\n");

				if (std::abs(stock_fee) > 0.00001)
				{
					old_balance = data.balances[currency];
					old_balance_eur = data.balances_in_eur[currency];

					/* Frage ist hier, welcher Kurs für Fremdwährungsspesen angesetzt wird. */
					/* Entscheidung gegen den tagesaktuellen Kurs und für den gleitenden Durchschnitt. */
					/* Begründung: Werden Spesen in einer Fremdwährung beglichen, */
					/* entspricht das einer Veräußerung der Fremdwährung. */
					/* Es entsteht Gewinn/Verlust in Bezug auf die durch Spesen beglichene Dienstleistung. */
					/* Anmerkung: Tagesaktueller Kurs = stock_fee * booking / (stock_paid + stock_fee) */
					data.balances[currency] -= stock_fee;
					data.balances_in_eur[currency] -= stock_fee * old_balance_eur / old_balance;
					print_fee(data.year, month + 1, tm.tm_mday, stock_fee,
							data.balances[currency], eur_fee, data.balances_in_eur[currency]);

					printf("%10.02f ", -stock_fee * old_balance_eur / old_balance); 		/* Ansatz */
					double guv = eur_fee - stock_fee * old_balance_eur / old_balance;
					printf("%12.05f ", guv); 		/* GuV */
					if (guv > 0) data.balances_profit[currency] += guv;
					else data.balances_losses[currency] += guv;
					printf("%12.05f ", data.balances_profit[currency]); 		/* Gewinn kum */
					printf("%12.05f ", data.balances_losses[currency]); 		/* Verlust kum */
					printf("\n");
				}
			}
			fflush(stdout);
		}
	} /* for (int month = 0; month < 12; ++month) */

	overall_losses += data.balances_losses[currency];
	overall_profit += data.balances_profit[currency];
	printf("======= == ======= ====== =========== ");
	printf("%11s ", currency.name); 					/* Bestand */
	printf("========== ========== =========== ");
	printf("=========== "); 				/* Bestand EUR */
	printf("=========== "); 		/* Kurs */
	printf("========== ");
	printf("%12.05f ", overall_profit); 		/* Gewinn kum */
	printf("%12.05f ", overall_losses); 		/* Verlust kum */
	printf("\n");

}
}


} /* namespace ibkr */
