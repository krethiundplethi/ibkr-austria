/*
 * tranche.h
 *
 *  Created on: 27.03.2022
 *      Author: AndreasFellnhofer
 */



#ifndef SOURCE_TRANCHE_H_
#define SOURCE_TRANCHE_H_

#include <iostream>
#include "security.hpp"
#include "currency.hpp"


namespace ibkr
{

class tranche
{
public:
	enum e_ordertype
	{
		BUY,
		SELL
	};

	tranche() = delete;
	tranche(const security &security, int amount, const currency::price price, const currency::price fee, bool sell);

	inline const security &getSecurity() const { return sec; }
	inline const currency::price &getFee() const { return fee; }

	~tranche()
	{
		std::cout << " #tranche# ";
	};

private:
	const security &sec;
	int amount;
	currency::price price;
	currency::price fee;
	enum e_ordertype ordertype;
};


std::ostream &operator<<(std::ostream &, const tranche &);



} /* namespace ibkr */

#endif /* SOURCE_TRANCHE_H_ */