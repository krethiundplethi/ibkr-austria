
#include "currency.hpp"
#include "security.hpp"
#include "tranche.hpp"
#include "ledger.hpp"
#include "ibkr_parser.hpp"

#include "CLI11.hpp"

#include <iostream>
#include <set>

using namespace std;
using namespace ibkr;

int g_year = 2021;
std::map <std::string, std::unique_ptr<tranche>> g_map_stock_trades;
std::map <std::string, std::unique_ptr<tranche>> g_map_forex_trades;
std::set <ibkr::currency::unit> g_foreign_currencies;
std::map <ibkr::currency::unit, double> g_balances;
std::map <ibkr::currency::unit, double> g_balances_in_eur;


void cbk_stock(const std::tm &tm, std::unique_ptr<tranche> &p_tranche)
{
	char buf[32];
	// cout << "STONK " << *p_tranche << endl;
	//auto buf = std::make_unique<char[]>(32); /*only 17, better safe than sorry YYYYMMDDHHMMSSxxx*/

	/* man, c++ can be shitty. doing this in a safe way is 15 lines of code. so'll do it unsafe. */
	snprintf(buf, 31, "%04u%02u%02u%02u%02u%02u%-04s",
			1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec,
			p_tranche->getSecurity().getName().c_str());

	g_map_stock_trades[std::string(buf)] = std::move(p_tranche);

}


void cbk_forex(const std::tm &tm, std::unique_ptr<tranche> &p_tranche)
{
	char buf[32];
	// cout << "STONK " << *p_tranche << endl;
	//auto buf = std::make_unique<char[]>(32); /*only 17, better safe than sorry YYYYMMDDHHMMSSxxx*/

	/* man, c++ can be shitty. doing this in a safe way is 15 lines of code. so'll do it unsafe. */

	auto ss = stringstream(p_tranche->getSecurity().getName());
	string token;
	vector <std::string> tokenized;

	tokenized.emplace_back("");
	while (getline(ss, token, ' '))
	{
		tokenized.push_back(token);
	}

	snprintf(buf, 31, "%04u%02u%02u%02u%02u%02u%-04s",
			1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec,
			tokenized.back().c_str());

	const currency::unit &cu = p_tranche->getPrice().unit;
	if (g_foreign_currencies.find(cu) == g_foreign_currencies.end())
	{
		g_foreign_currencies.insert(cu);
		g_balances[cu] = 0.0;
		g_balances_in_eur[cu] = 0.0;
	}

	int cnt = 0;
	auto key = std::string(buf) + "-" + std::to_string(cnt);
	while (g_map_forex_trades.find(key) != g_map_forex_trades.end())
	{
		key = std::string(buf) + "-" + std::to_string(++cnt);
	}

	g_map_forex_trades[key] = std::move(p_tranche);
}


void print_fee(int year, int mon, int day, double fx, double fx_bal, double eur, double eur_bal)
{
	/*       2022-07 ; Day   ; Symbol  ; K/V/WP ; */
	printf("%4d-%02d ; %02d  ; FEE     ; F      ; ",  year, mon, day);
	/*      USD     ; Bestand ; ; Soll EUR ; Haben EUR ; ; EUR gl.D.s. ;   Kurs    ; ; Ansatz EUR ; GuV ; Gebühren*/
	printf("%11.02f ; %11.02f ; ; %9.02f ;           ; ; %11.02f ; %9.05f ; ; ", -fx, fx_bal, eur, eur_bal, eur_bal / fx_bal);
}


int main(int argc, char **argv)
{
    //CLI::App app{"Tax calc"};

    std::string filename("C:\\Development\\github\\ibkr-austria\\U6443611_20210618_20211231.csv");
    //app.add_option("-f,--file", filename, "csv file from IBKR");

    //CLI11_PARSE(app, argc, argv);

	cout << "Opening file: " << filename << endl;

	/*
	currency::price p = {currency::USD, 1.2};
	security aktie("gazprom",  p);
	tranche tranche1({"gazprom", {currency::USD, 1.2}}, 100, p, {currency::USD, 3}, false);
	cout << tranche1 << endl;

	ledger l("gazprom");

	time_t rawtime;
	struct tm * ptm;
	time(&rawtime);
	ptm = gmtime(&rawtime);

	l.add_entry(entry::type::CREDIT, *ptm, tranche1);
	l.add_entry(entry::type::CREDIT, *ptm, tranche1);

	cout << l << endl;
	 */

	ibkr_parser parser(filename);
	parser.register_callback_on_stock(cbk_stock);
	parser.register_callback_on_forex(cbk_forex);
	parser.parse();

	/** complete output for debug **/

	for (auto const &elem: g_map_stock_trades)
	{
		cout << elem.first << ": " << *elem.second << endl;
	}

	for (auto const &elem: g_map_forex_trades)
	{
		cout << elem.first << "> " << *elem.second << endl;
	}

	for (auto const &elem: g_foreign_currencies)
	{
		cout << elem << endl;
	}
	cout.flush();


	for (auto const currency : g_foreign_currencies)
	{
		for (int month = 0; month < 12; ++month)
		{
			printf("%4d-%02d ; Day ; Symbol  ; K/V/WP ; %-11s ;"
				   "  Bestand    ; ; Soll EUR  ; Haben EUR ; ; EUR gl.D.s. ;"
				   "   Kurs    ; ; Ansatz EUR; GuV          \n", g_year, month + 1, currency.name);

			for (auto const &elem: g_map_forex_trades)
			{
				tranche t = static_cast<tranche &>(*elem.second);
				std::string key = elem.first;
				key = key.substr(0, 18);
				std::tm tm = t.getTimeStamp();

				if ((t.getPrice().unit.id == currency.id) && (tm.tm_mon == month))
				{
					if (t.getSecurity().getName() == "AMC")
					{
						printf("");
					}

					printf("%4d-%02d ; %02d  ; ",  g_year, month+1, tm.tm_mday);
					printf("%-7s ; ", t.getSecurity().getName().c_str());

					auto found = g_map_stock_trades.end();


					if (t.getSecurity().isEquity())
					{
						printf("%-6s ; ", t.isSell() ? "WPK" : "WPV"); /* buying equity == selling currency */
						found = g_map_stock_trades.find(key);
					}
					else
					{
						printf("%-6s ; ", t.isSell() ? "V" : "K");
					}

					double stock_paid = 0.0;
					double stock_fee = 0.0;
					double eur_paid = t.getPrice() - t.getFee();
					double eur_fee = 0;

					if (found != g_map_stock_trades.end()) /* look up in the transaktions table */
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

						const tranche &order = *(found->second);

						/* acshually we are only doing this to get the fee on the transaktion,
						 * thats it. All other information is already in the forex table entry. */
						stock_fee = order.getFee().value;
						stock_paid = t.getPrice() * (t.isSell() ? -1.00 : 1.00) / t.getSecurity().getPrice();
						stock_paid = t.isSell() ? stock_paid - stock_fee : stock_paid + stock_fee;

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
					else if(t.getSecurity().getType() == security::EQUITY)
					{
						/* fixme: calculating back from EUR to the foreign currency is odd.
						 * Is needed because for equity the "quantity" is not the pricetag but the amout of stock. */
						stock_paid = t.getPrice() * (t.isSell() ? -1.00 : 1.00) / t.getSecurity().getPrice();
					}
					else
					{
						/* Für devisen gilt Menge = Wert */
						stock_paid = t.getQuanti();
						if (t.isSell())
						{
							/* fixme:
							 * selling currency ... IBKR already substraced the EUR fee, then we substracted again...
							 * which we need to undo for taxing */
							eur_paid += 2 * t.getFee();
						}
					}

					float old_balance = g_balances[currency];
					float old_balance_eur = g_balances_in_eur[currency];

					printf("%11.02f ; ", stock_paid * (t.isSell() ? -1.00 : 1.00));

					g_balances[currency] += stock_paid * (t.isSell() ? -1.00 : 1.00);
					/* Hier wird der gleitendende Durchschnitt mittels old_balance angesetzt. */

					if (!t.isSell())
					{
						/* whenever I get currency, the current EUR value is used, not the averaged one */
						g_balances_in_eur[currency] += eur_paid;
					}
					else
					{
						g_balances_in_eur[currency] += (stock_paid *  (t.isSell() ? -1.00 : 1.00) * old_balance_eur / old_balance);
					}

					printf("%11.02f ; ", g_balances[currency]); 					/* Bestand */
					printf(t.isSell() ? "; %9.02f ;           ; ; " :				/* Soll */
							            ";           ; %9.02f ; ; ", eur_paid);		/* Haben */
					printf("%11.02f ; ", g_balances_in_eur[currency]); 				/* Bestand EUR */
					printf("%9.05f ; ; ", g_balances_in_eur[currency] / g_balances[currency]); 		/* Kurs */

					if (!t.isSell())
					{
						/* whenever I get currency, the current EUR value is used, not the averaged one */
						printf("%9.02f ; ", eur_paid); 		/* Ansatz */
						printf("%12.05f ; ", 0.0); 		/* GuV */
					}
					else
					{
						printf("%9.02f ; ", stock_paid * old_balance_eur / old_balance); 		/* Ansatz */
						printf("%12.05f ; ", eur_paid - stock_paid * old_balance_eur / old_balance); 		/* GuV */
					}


					printf("\n");
					if (stock_fee > 0.0)
					{
						/* Frage ist hier, welcher Kurs für Fremdwährungsspesen angesetzt wird. */
						/* Entscheidung gegen den tagesaktuellen Kurs und für den gleitenden Durchschnitt. */
						/* Begründung: Werden Spesen in einer Fremdwährung beglichen, */
						/* entspricht das einer Veräußerung der Fremdwährung. */
						/* Es entsteht Gewinn/Verlust in Bezug auf die durch Spesen beglichene Dienstleistung. */
						/* Anmerkung: Tagesaktueller Kurs = stock_fee * booking / (stock_paid + stock_fee) */
						g_balances[currency] -= stock_fee;
						g_balances_in_eur[currency] -= stock_fee * old_balance_eur / old_balance;
						print_fee(g_year, month + 1, tm.tm_mday, stock_fee,
								g_balances[currency], eur_fee, g_balances_in_eur[currency]);

						printf("%9.02f ; ", stock_fee * old_balance_eur / old_balance); 		/* Ansatz */
						printf("%12.05f ; ", eur_fee - stock_fee * old_balance_eur / old_balance); 		/* GuV */
						printf("\n");
					}
				}
			}
		}
		break;
	}

	return 0;
}
