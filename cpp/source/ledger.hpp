/*
 * depot.h
 *
 *  Created on: 27.03.2022
 *      Author: AndreasFellnhofer
 */

#ifndef SOURCE_LEDGER_H_
#define SOURCE_LEDGER_H_

#include "currency.hpp"
#include "security.hpp"
#include "tranche.hpp"

#include <vector>
#include <chrono>


namespace ibkr {

using namespace std;
using timepoint = std::tm;


struct entry {
	enum class type {
		DEBIT,
		CREDIT,
	};

	type ty;
	timepoint tp;
	tranche tr;

	entry() = delete;
	entry(type ty, const timepoint &tp, const tranche &tr) : ty {ty}, tp {tp}, tr {tr} {};
	entry(const entry &e) : ty {e.ty}, tp {e.tp}, tr {e.tr} {};
};


class ledger {

public:

	ledger(const char *name) : name {name} {};
	virtual ~ledger();

	void add_entry(entry::type type, const timepoint &tp, const tranche &tr);
	void get_entries(std::vector <entry> const ** debit, std::vector <entry> const ** credit) const
	{
		*debit = const_cast<std::vector <entry> *>(&debit_entries);
		*credit = const_cast<std::vector <entry> *>(&credit_entries);
	}
private:
	const char *name;
	std::vector <entry> debit_entries;
	std::vector <entry> credit_entries;
};


std::ostream &operator<<(std::ostream &, const ledger &);


} /* namespace ibkr */

#endif /* SOURCE_DEPOT_H_ */
