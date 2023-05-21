
#include "currency.hpp"
#include "security.hpp"
#include "tranche.hpp"
#include "ledger.hpp"
#include "ibkr_parser.hpp"

#include "CLI11.hpp"

#include <iostream>
#include <set>

#include "pnl.hpp"
#include "pnl_forex.hpp"
#include "pnl_equity.hpp"


using namespace std;
using namespace ibkr;

struct pnl::inout_data data;

static void cbk_trade(const std::tm &tm, std::unique_ptr<tranche> &p_tranche);
static void cbk_holdings(const std::tm &tm, std::unique_ptr<tranche> &p_tranche);
static void cbk_forex(const std::tm &tm, std::unique_ptr<tranche> &p_tranche);


static void cbk_holdings(const std::tm &tm, std::unique_ptr<tranche> &p_tranche)
{
	cout << "HODL " << *p_tranche << endl;
	const currency::unit &cu = p_tranche->getSecurity().getPrice().unit;

	if (p_tranche->getSecurity().getType() == security::CURRENCY)
	{
		if (data.foreign_currencies.find(cu) != data.foreign_currencies.end())
		{
			cout << "ERROR! Holdings contains multiple entries of " << p_tranche->getSecurity() << endl;
		}
		else
		{
			data.foreign_currencies.insert(cu);
			data.balances[cu] = p_tranche->getQuanti();
			data.balances_in_eur[cu] = p_tranche->getPrice().value;
			data.balances_losses[cu] = 0.0;
			data.balances_profit[cu] = 0.0;
		}
	}
	else
	{
		/* equity holdings from prev year, let's see if we can handle
		 * it like any other trade - prolly not. */
		/* first workaround: set date to 1.1. of taxyear */
		std::tm tm_patched = tm;
		tm_patched.tm_year = data.year - 1900;
		tm_patched.tm_mon = tm_patched.tm_mday = 1;
		tm_patched.tm_hour = tm_patched.tm_min = tm_patched.tm_sec = 0;

		cbk_trade(tm_patched, p_tranche);
	}

}



static void cbk_trade(const std::tm &tm, std::unique_ptr<tranche> &p_tranche)
{
	char buf[32];
	// cout << "STONK " << *p_tranche << endl;
	//auto buf = std::make_unique<char[]>(32); /*only 17, better safe than sorry YYYYMMDDHHMMSSxxx*/

	/* man, c++ can be shitty. doing this in a safe way is 15 lines of code. so'll do it unsafe. */
	snprintf(buf, 31, "%04u%02u%s",
			1900 + tm.tm_year, 1 + tm.tm_mon, //tm.tm_mday,
			/*tm.tm_hour, tm.tm_min, tm.tm_sec,*/
			p_tranche->getSecurity().getName().c_str());

	int cnt = 0;
	auto key = std::string(buf) + "-" + std::to_string(cnt);
	while (data.map_trades.find(key) != data.map_trades.end())
	{
		key = std::string(buf) + "-" + std::to_string(++cnt);
	}

	data.map_trades[key] = std::move(p_tranche);

}


static void cbk_forex(const std::tm &tm, std::unique_ptr<tranche> &p_tranche)
{
	auto ss = stringstream(p_tranche->getSecurity().getName());
	string token;
	vector <std::string> tokenized;

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
	snprintf(buf2, 31, "%04u%02u%s",
			1900 + tm.tm_year, 1 + tm.tm_mon,
			//tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, /* fixme: only works for forex, not equity --> find better datastructure */
			tokenized.back().c_str());

	const currency::unit &cu = p_tranche->getPrice().unit;
	if (data.foreign_currencies.find(cu) == data.foreign_currencies.end())
	{
		cout << "Warning: Transaction without cash balance in " << cu << ": " << *p_tranche << endl;
		data.foreign_currencies.insert(cu);
		data.balances[cu] = 0.0;
		data.balances_in_eur[cu] = 0.0;
		data.balances_losses[cu] = 0.0;
		data.balances_profit[cu] = 0.0;
	}

	int cnt = 0;
	auto key = std::string(buf) + "-" + std::to_string(cnt);
	while (data.map_forex.find(key) != data.map_forex.end())
	{
		key = std::string(buf) + "-" + std::to_string(++cnt);
	}
	data.map_forex[key] = std::move(p_tranche);

	cnt = 0;
	auto key2 = std::string(buf2) + "-" + std::to_string(cnt);
	while (data.map_forex_lut.find(key2) != data.map_forex_lut.end())
	{
		key2 = std::string(buf2) + "-" + std::to_string(++cnt);
	}
	data.map_forex_lut[key2] = data.map_forex[key];
}




int main(int argc, char **argv)
{
    //CLI::App app{"Tax calc"};
    std::string filename_transactions("C:\\Development\\github\\ibkr-austria\\U6443611_20220103_20221230.csv");
    std::string filename_initial_holdings("C:\\Development\\github\\ibkr-austria\\Bestand_2021-12-31.csv");
    int tax_year = 2022;
    bool verbose = true;
    //app.add_option("-f,--file", filename, "csv file from IBKR");
    //app.add_option("-v,--verbose", verbose, "be verbose");

    //CLI11_PARSE(app, argc, argv);

    data.year = tax_year;

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
		for (auto const &elem: data.foreign_currencies)
		{
			cout << elem << endl;
		}

		for (auto const &elem: data.map_trades)
		{
			cout << elem.first << ": " << *elem.second << endl;
		}

		for (auto const &elem: data.map_forex)
		{
			cout << elem.first << "> " << *elem.second << endl;
		}

		cout.flush();
	}

	double overall_profit = 0.0;
	double overall_losses = 0.0;

	for (auto const currency : data.foreign_currencies)
	{
		pnl::forex_calc(currency, overall_profit, overall_losses, data);
	}

	overall_profit = 0.0;
	overall_losses = 0.0;

	data.foreign_currencies.insert(ibkr::currency::EUR);
	for (auto const currency : data.foreign_currencies)
	{
		pnl::equity_calc(currency, overall_profit, overall_losses, data);
	}


	return 0;
}
