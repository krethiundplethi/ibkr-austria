
#include "currency.hpp"
#include "security.hpp"
#include "tranche.hpp"
#include "ledger.hpp"
#include "ibkr_parser.hpp"
#include "rate_parser.hpp"

#include "CLI11.hpp"

#include <iostream>
#include <set>
#include <time.h>

#include "pnl.hpp"
#include "pnl_forex.hpp"
#include "pnl_equity.hpp"


using namespace std;
using namespace ibkr;

struct pnl::inout_data book;

static void cbk_trade(const std::tm &tm, std::unique_ptr<tranche> &p_tranche);
static void cbk_holdings(const std::tm &tm, std::unique_ptr<tranche> &p_tranche);
static void cbk_forex(const std::tm &tm, std::unique_ptr<tranche> &p_tranche);


static void cbk_holdings(const std::tm &tm, std::unique_ptr<tranche> &p_tranche)
{
	cout << "HODL " << *p_tranche << endl;
	const currency::unit &cu = p_tranche->getSecurity().getPrice().unit;

	if (p_tranche->getSecurity().getType() == security::CURRENCY)
	{
		if (book.foreign_currencies.find(cu) != book.foreign_currencies.end())
		{
			cout << "ERROR! Holdings contains multiple entries of " << p_tranche->getSecurity() << endl;
		}
		else
		{
			book.foreign_currencies.insert(cu);
			book.balances[cu] = p_tranche->getQuanti();
			book.balances_in_eur[cu] = p_tranche->getPrice().value;
			book.balances_losses[cu] = 0.0;
			book.balances_profit[cu] = 0.0;
		}
	}
	else
	{
		/* equity holdings from prev year, let's see if we can handle
		 * it like any other trade - prolly not. */
		/* first workaround: set date to 1.1. of taxyear */
		std::tm tm_patched = tm;
		tm_patched.tm_year = book.year - 1900;
		tm_patched.tm_mon = tm_patched.tm_mday = 1;
		tm_patched.tm_hour = tm_patched.tm_min = tm_patched.tm_sec = 0;

		cbk_trade(tm_patched, p_tranche);
	}

}


double find_rate(const std::tm &tm, std::unique_ptr<tranche> &p_tranche)
{
	double rate = book.rates.get(p_tranche->getPrice().unit.name, tm);
	std::tm new_tm = tm;
	new_tm.tm_mday--; /* if holiday, check day before */
	
	for (int i = 0; (i < 5) && (rate < 0.0); i++)
	{
		mktime(&new_tm);
		rate = book.rates.get(p_tranche->getPrice().unit.name, new_tm);
		new_tm.tm_mday++;
	}
	if (rate < 0.0)
	{
		throw std::runtime_error("ERROR: Cannot find rate.");
	}
	return rate;
}


static void cbk_trade(const std::tm &tm, std::unique_ptr<tranche> &p_tranche)
{
	char buf[48];
	char ckey[32];
	// cout << "STONK " << *p_tranche << endl;
	//auto buf = std::make_unique<char[]>(32); /*only 17, better safe than sorry YYYYMMDDHHMMSSxxx*/

	/* man, c++ can be shitty. doing this in a safe way is 15 lines of code. so'll do it unsafe. */
	snprintf(buf, 47, "%04u%02u%s", 1900 + tm.tm_year, 1 + tm.tm_mon, p_tranche->getSecurity().getName().c_str());

	int cnt = 0;
	auto key = std::string(buf) + "-" + std::to_string(cnt);
	while (book.map_trades.find(key) != book.map_trades.end())
	{
		key = std::string(buf) + "-" + std::to_string(++cnt);
	}

	double rate = find_rate(tm, p_tranche);
	p_tranche->setEcbRate(rate);
	book.map_trades[key] = std::move(p_tranche);
}


static void cbk_forex(const std::tm &tm, std::unique_ptr<tranche> &p_tranche)
{
	auto ss = stringstream(p_tranche->getSecurity().getName());
	string token;
	vector <std::string> tokenized;
	char ckey[32];

	tokenized.emplace_back("");
	while (getline(ss, token, ' '))
	{
		tokenized.push_back(token);
	}

	/* key for forex */
	char buf[32];

	snprintf(buf, 31, "%04u%02u%02u%02u%02u%02u%s",
			1900 + tm.tm_year, 1 + tm.tm_mon,
			tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
			tokenized.back().c_str());

	char buf2[32];
	snprintf(buf2, 31, "%04u%02u%s-%s", 1900 + tm.tm_year, 1 + tm.tm_mon, tokenized.back().c_str(), p_tranche->getPrice().unit.name);

	const currency::unit &cu = p_tranche->getPrice().unit;
	if (book.foreign_currencies.find(cu) == book.foreign_currencies.end())
	{
		cout << "Warning: Transaction without cash balance in " << cu << ": " << *p_tranche << endl;
		book.foreign_currencies.insert(cu);
		book.balances[cu] = 0.0;
		book.balances_in_eur[cu] = 0.0;
		book.balances_losses[cu] = 0.0;
		book.balances_profit[cu] = 0.0;
	}

	int cnt = 0;
	auto key = std::string(buf) + "-" + std::to_string(cnt);
	while (book.map_forex.find(key) != book.map_forex.end())
	{
		key = std::string(buf) + "-" + std::to_string(++cnt);
	}

	cnt = 0;
	auto key2 = std::string(buf2) + "-" + std::to_string(cnt);
	while (book.map_forex_lut.find(key2) != book.map_forex_lut.end())
	{
		key2 = std::string(buf2) + "-" + std::to_string(++cnt);
	}

	double rate = find_rate(tm, p_tranche);
	p_tranche->setEcbRate(rate);
	book.map_forex[key] = std::move(p_tranche);
	book.map_forex_lut[key2] = book.map_forex[key];
}




int main(int argc, char **argv)
{
	//CLI::App app{"Tax calc"};
	/* this was 2022 */
	//std::string filename_transactions("C:\\Development\\github\\ibkr-austria\\U6443611_20220103_20221230.csv");
	//std::string filename_initial_holdings("C:\\Development\\github\\ibkr-austria\\Bestand_2021-12-31.csv");
	//int tax_year = 2022;

	/* this is 2023 */
	const std::string filename_rates("/mnt/c/Development/ibkr-austria/ecb_rates.csv");
	const std::string filename_transactions("/mnt/c/Development/ibkr-austria/U6443611_20230102_20231229.csv");
	const std::string filename_initial_holdings("/mnt/c/Development/ibkr-austria/Bestand_2022-12-31.csv");
	int tax_year = 2023;

	bool verbose = true;
	//app.add_option("-f,--file", filename, "csv file from IBKR");
	//app.add_option("-v,--verbose", verbose, "be verbose");

	//CLI11_PARSE(app, argc, argv);

	book.year = tax_year;

	if (verbose) cout << "Opening file: " << filename_rates << endl;
	book.rates.open(filename_rates);

	if (verbose) cout << "Opening file: " << filename_initial_holdings << endl;
	ibkr_parser parser1(filename_initial_holdings);
	parser1.register_callback_on_initial_holding(cbk_holdings);
	parser1.parse();

	if (verbose) cout << "Opening file: " << filename_transactions << endl;

	ibkr_parser parser(filename_transactions);
	parser.register_callback_on_stock_trade(cbk_trade);
	parser.register_callback_on_forex_trade(cbk_trade);
	parser.register_callback_on_options_trade(cbk_trade);
	parser.register_callback_on_forex(cbk_forex);
	parser.register_callback_on_stocksplit(cbk_trade);
	parser.parse();

	if (verbose)
	{
		for (auto const &elem: book.foreign_currencies)
		{
			cout << elem << endl;
		}

		for (auto const &elem: book.map_trades)
		{
			cout << elem.first << ": " << *elem.second << endl;
		}

		for (auto const &elem: book.map_forex)
		{
			cout << elem.first << "> " << *elem.second << endl;
		}

		cout.flush();
	}

	double overall_profit = 0.0;
	double overall_losses = 0.0;

	printf("\n\nFOREX\n\n");

	for (auto const currency : book.foreign_currencies)
	{
		pnl::forex_calc(currency, overall_profit, overall_losses, book);
	}

	overall_profit = 0.0;
	overall_losses = 0.0;

	book.foreign_currencies.insert(ibkr::currency::EUR);

	printf("\n\nEQUITY\n\n");

	for (auto const currency : book.foreign_currencies)
	{
		pnl::equity_calc(ibkr::security::EQUITY, currency, overall_profit, overall_losses, book);
	}

	printf("\n\nOPTIONS\n\n");

	overall_profit = 0.0;
	overall_losses = 0.0;

	for (auto const currency : book.foreign_currencies)
	{
		pnl::equity_calc(ibkr::security::OPTION, currency, overall_profit, overall_losses, book);
	}

	/* 
	printf("\n\nFUTURES\n\n");

	overall_profit = 0.0;
	overall_losses = 0.0;

	for (auto const currency : book.foreign_currencies)
	{
		pnl::equity_calc(ibkr::security::FUTURE, currency, overall_profit, overall_losses, book);
	}
	*/

	return 0;
}
