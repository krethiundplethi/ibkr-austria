#include <iostream>

#include "currency.h"
#include "security.h"
#include "tranche.h"

using namespace std;
using namespace ibkr;


int main(int argc, char **argv)
{
	cout << "Hello world" << endl;

	currency::price p = {currency::USD, 1.2};
	security aktie("gazprom",  p);

	tranche tranche1(aktie, 100, p, {currency::USD, 3}, false);
	//tranche tranche2(security("gazprom", currency(currency::USD, 1.2)), 300, currency(currency::EUR, 800.0), 2.0, false);
	//tranche tranche3(security("gazprom", usd(1.3)), 50, eur(100.0), 2.0, false);

	cout << tranche1 << endl;
	//cout << tranche2 << endl;
	//cout << tranche3 << endl;

	return 0;
}