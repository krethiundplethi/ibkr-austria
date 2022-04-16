/*
 * depot.cpp
 *
 *  Created on: 27.03.2022
 *      Author: AndreasFellnhofer
 */

#include "ledger.hpp"
#include <iomanip>
#include <chrono>

namespace ibkr {

ledger::~ledger()
{
}


void ledger::add_entry(entry::type type, const timepoint &tp, const tranche &tr)
{
	if (type == entry::type::CREDIT)
	{
		credit_entries.push_back(entry(type, tp, tr));
	}
}


std::ostream &operator<<(std::ostream &os, const ledger &l)
{
	vector <entry> const *deb = NULL;
	vector <entry> const *cre = NULL;

	l.get_entries(&deb, &cre);

	for(vector<entry>::const_iterator it = deb->begin(); it != deb->end(); ++it)
	{
		os << "debit date:" << asctime(&(it->tp)) << ", " << it->tr << std::endl;
	}

	for(vector<entry>::const_iterator it = cre->begin(); it != cre->end(); ++it)
	{
		os << "credit date:" << asctime(&(it->tp)) << ", " << it->tr << std::endl;
	}

	return os;
}

} /* namespace ibkr */
