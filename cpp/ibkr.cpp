
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

int g_year = 2022;
std::map <std::string, std::unique_ptr<tranche>> g_map_stock_trades;
std::map <std::string, std::unique_ptr<tranche>> g_map_forex_trades;
std::set <ibkr::currency::unit> g_foreign_currencies;
std::map <ibkr::currency::unit, double> g_balances;


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
	}

	g_map_forex_trades[std::string(buf)] = std::move(p_tranche);
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
			printf("%4d-%02d ; Day ; Symbol  ; K/V/WP ; %-9s ;  Bestand  ; ; Soll EUR  ; Haben EUR ; ; Kurs ; ; Ansatz EUR ; GuV ; Gebühren\n", g_year, month + 1, currency.name);

			for (auto const &elem: g_map_forex_trades)
			{
				tranche t = static_cast<tranche &>(*elem.second);
				std::string key = elem.first;
				std::tm tm = t.getTimeStamp();
				if ((t.getPrice().unit.id == currency.id) && (tm.tm_mon == month))
				{
					printf("%4d-%02d ; %02d  ; ",  g_year, month+1, tm.tm_mday);
					printf("%-7s ; ", t.getSecurity().getName().c_str());

					auto found = g_map_stock_trades.end();

					if (t.getSecurity().isEquity())
					{
						printf("%-6s ; ", "WP");
						found = g_map_stock_trades.find(key);
					}
					else
					{
						printf("%-6s ; ", t.isSell() ? "V" : "K");
					}

					double booking = t.getPrice() - t.getFee();
					double stock_paid = 0.0;
					double stock_fee = 0.0;

					if (found != g_map_stock_trades.end())
					{
						stock_paid = found->second->getPrice().value;
						stock_fee = found->second->getFee().value;
					}
					else
					{
						stock_paid = t.getAmount();
					}

					printf("%-9.02f ; ", stock_paid * (t.isSell() ? -1.00 : 1.00));
					g_balances[currency] += stock_paid * (t.isSell() ? -1.00 : 1.00);
					printf("%9.02f ; ", g_balances[currency]);
					printf(t.isSell() ? "; %9.02f ;           ; " :\
							            ";           ; %9.02f ; ", booking);


					printf("\n");
				}
			}
		}
		break;
	}

	return 0;
}
