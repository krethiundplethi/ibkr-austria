
#include "source/currency.hpp"
#include "source/security.hpp"
#include "source/tranche.hpp"
#include "source/ibkr_parser.hpp"
#include "source/rate_parser.hpp"
#include "source/pnl.hpp"
#include "source/pnl_forex.hpp"
#include "source/pnl_equity.hpp"

#include "include/CLI11.hpp"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <time.h>

using namespace std;
using namespace ibkr;

struct pnl::inout_data book;

namespace
{

void cbk_trade(const std::tm &day, std::unique_ptr<tranche> &p_tranche);
void cbk_holdings(const std::tm &day, std::unique_ptr<tranche> &p_tranche);
void cbk_forex(const std::tm &day, std::unique_ptr<tranche> &p_tranche);


void cbk_holdings(const std::tm &day, std::unique_ptr<tranche> &p_tranche)
{
	cout << "HODL " << *p_tranche << '\n';
	const currency::unit &cur = p_tranche->getSecurity().getPrice().unit;

	if (p_tranche->getSecurity().getType() == security::CURRENCY)
	{
		if (book.foreign_currencies.find(cur) != book.foreign_currencies.end())
		{
			cout << "ERROR! Holdings contains multiple entries of " << p_tranche->getSecurity() << '\n';
		}
		else
		{
			book.foreign_currencies.insert(cur);
			book.balances[cur] = p_tranche->getQuanti();
			book.balances_in_eur[cur] = p_tranche->getPrice().value;
			book.balances_losses[cur] = 0.0;
			book.balances_profit[cur] = 0.0;
		}
	}
	else
	{
		/* equity holdings from prev year, let's see if we can handle
		* it like any other trade - prolly not. */
		/* first workaround: set date to 1.1. of taxyear */
		std::tm tm_patched = day;
		tm_patched.tm_year = book.year - 1900;
		tm_patched.tm_mon = tm_patched.tm_mday = 1;
		tm_patched.tm_hour = tm_patched.tm_min = tm_patched.tm_sec = 0;

		cbk_trade(tm_patched, p_tranche);
	}

}


double find_rate(const std::tm &day, std::unique_ptr<tranche> &p_tranche)
{
	double rate = book.rates.get(p_tranche->getPrice().unit.name, day);
	std::tm new_tm = day;
	new_tm.tm_mday--; /* if holiday, check day before */
	
	for (int i = 0; (i < 5) && (rate < 0.0); i++)
	{
		mktime(&new_tm);
		rate = book.rates.get(p_tranche->getPrice().unit.name, new_tm);
		new_tm.tm_mday++;
	}
	if (rate < 0.0)
	{
		std::cerr << "ERROR: Cannot find " << p_tranche->getSecurity() << " EUR." << p_tranche->getPrice().unit.name << " conversion rate." << '\n';
		exit(1);
	}
	return rate;
}


void cbk_trade(const std::tm &day, std::unique_ptr<tranche> &p_tranche)
{
	char buf[48];
	char ckey[32];
	// cout << "STONK " << *p_tranche << endl;
	//auto buf = std::make_unique<char[]>(32); /*only 17, better safe than sorry YYYYMMDDHHMMSSxxx*/

	/* man, c++ can be shitty. doing this in a safe way is 15 lines of code. so'll do it unsafe. */
	snprintf(buf, 47, "%04u%02u%s", 1900 + day.tm_year, 1 + day.tm_mon, p_tranche->getSecurity().getName().c_str());

	int cnt = 0;
	auto key = std::string(buf) + "-" + std::to_string(cnt);
	while (book.map_trades.find(key) != book.map_trades.end())
	{
		key = std::string(buf) + "-" + std::to_string(++cnt);
	}

	const double rate = find_rate(day, p_tranche);
	p_tranche->setEcbRate(rate);
	book.map_trades[key] = std::move(p_tranche);
}


void cbk_forex(const std::tm &day, std::unique_ptr<tranche> &p_tranche)
{
	auto stream = stringstream(p_tranche->getSecurity().getName());
	string token;
	vector <std::string> tokenized;
	char ckey[32];

	tokenized.emplace_back("");
	while (getline(stream, token, ' '))
	{
		tokenized.push_back(token);
	}

	/* key for forex */
	char buf[32];

	snprintf(buf, 31, "%04u%02u%02u%02u%02u%02u%s",
			1900 + day.tm_year, 1 + day.tm_mon,
			day.tm_mday, day.tm_hour, day.tm_min, day.tm_sec,
			tokenized.back().c_str());

	char buf2[32];
	snprintf(buf2, 31, "%04u%02u%s-%s", 1900 + day.tm_year, 1 + day.tm_mon, tokenized.back().c_str(), p_tranche->getPrice().unit.name);

	const currency::unit &cur = p_tranche->getPrice().unit;
	if (book.foreign_currencies.find(cur) == book.foreign_currencies.end())
	{
		cout << "Warning: Transaction without cash balance in " << cur << ": " << *p_tranche << '\n';
		book.foreign_currencies.insert(cur);
		book.balances[cur] = 0.0;
		book.balances_in_eur[cur] = 0.0;
		book.balances_losses[cur] = 0.0;
		book.balances_profit[cur] = 0.0;
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

	const double rate = find_rate(day, p_tranche);
	p_tranche->setEcbRate(rate);
	book.map_forex[key] = std::move(p_tranche);
	book.map_forex_lut[key2] = book.map_forex[key];
}

FILE *open_output(const std::string fname, bool verbose)
{
	if (verbose) { cout << "Open for writing: " << fname << '\n'; }

	std::FILE *file = std::fopen(fname.c_str(), "w");

	if (!file)
	{
		std::perror("fopen");
		exit(1);
	}

	return file;
}


} // anonymous namespace


int main(int argc, char **argv)
{
	CLI::App app{"Tax calc"};
	/* this was 2022 */
	//std::string filename_transactions("C:\\Development\\github\\ibkr-austria\\U6443611_20220103_20221230.csv");
	//std::string filename_initial_holdings("C:\\Development\\github\\ibkr-austria\\Bestand_2021-12-31.csv");
	//int tax_year = 2022;

	/* this is 2024 */
	std::string filename_rates;//{"c:/Development/ibkr-austria/ecb_rates_2024.csv"};
	std::string filename_transactions;//{"c:/Development/ibkr-austria/U6443611_20240101_20241231.csv"};
	std::string filename_initial_holdings;//{"c:/Development/ibkr-austria/Bestand_2023-12-31.csv"};

	bool verbose = true;
	app.add_option("-i,--initial", filename_initial_holdings, "csv file with initial holding")
		->required()
		->check(CLI::ExistingFile);

	app.add_option("-f,--file", filename_transactions, "csv file from IBKR")
		->required()
		->check(CLI::ExistingFile);

	app.add_option("-r,--rates", filename_rates, "csv file with ECB conversion rates")
		->required()
		->check(CLI::ExistingFile);

	app.add_option("-v,--verbose", verbose, "be verbose");

	CLI11_PARSE(app, argc, argv);

	book.year = 2024;

	if (verbose)
	{
		cout << "Open for reading: " << filename_rates << '\n';
	}
	book.rates.open(filename_rates);

	if (verbose)
	{
		cout << "Open for reading: " << filename_initial_holdings << '\n';
	}
	ibkr_parser parser1(filename_initial_holdings);
	parser1.register_callback_on_initial_holding(cbk_holdings);
	parser1.parse();

	if (verbose)
	{
		cout << "Open for reading: " << filename_transactions << '\n';
	}

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
			cout << elem << '\n';
		}

		for (auto const &elem: book.map_trades)
		{
			cout << elem.first << ": " << *elem.second << '\n';
		}

		for (auto const &elem: book.map_forex)
		{
			cout << elem.first << "> " << *elem.second << '\n';
		}

		cout.flush();
	}


	// Build output filename from transactions path (remove extension, add suffix)
	std::string out_filename = filename_transactions;
	const auto dotpos = out_filename.find_last_of('.');
	if (dotpos != std::string::npos) {
		out_filename.erase(dotpos);
	}

	FILE *file = open_output(out_filename + "_output_forex.txt", verbose);
	struct pnl::performance pnl = {0.0, 0.0};

	for (auto const currency : book.foreign_currencies)
	{
		pnl::forex_calc(file, currency, pnl, book);
	}
	std::fclose(file);


	file = open_output(out_filename + "_output_equity.txt", verbose);
	book.foreign_currencies.insert(ibkr::currency::EUR);
	pnl = {0.0, 0.0};

	for (auto const currency : book.foreign_currencies)
	{
		pnl::equity_calc(file, ibkr::security::EQUITY, currency, pnl, book);
	}
	std::fclose(file);


	file = open_output(out_filename + "_output_options.txt", verbose);
	pnl = {0.0, 0.0};

	for (auto const currency : book.foreign_currencies)
	{
		pnl::equity_calc(file, ibkr::security::OPTION, currency, pnl, book);
	}
	std::fclose(file);

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
