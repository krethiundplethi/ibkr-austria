/*
 * tranche.h
 *
 *  Created on: 27.03.2022
 *      Author: AndreasFellnhofer
 */



#ifndef SOURCE_TRANCHE_H_
#define SOURCE_TRANCHE_H_

#include <iostream>
#include <ctime>
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
	tranche(const tranche &rval);

	inline void setType(enum e_ordertype ot) {ordertype = ot; }
	inline const security &getSecurity() const { return sec; }
	inline const currency::price &getFee() const { return fee; }
	inline const currency::price &getPrice() const { return price; }
	inline const int &getAmount() const { return amount; }
	inline const std::tm &getTimeStamp() const { return timestamp; }
	inline void setTimeStamp(std::tm const &tm) { timestamp = tm; }

	void makeAbsolute(void);
	inline bool isSell(void) const { return ordertype == SELL; }

	~tranche() { };

private:
	const security sec;
	int amount;
	std::tm timestamp;
	currency::price price;
	currency::price fee;
	enum e_ordertype ordertype;
};


std::ostream &operator<<(std::ostream &, const tranche &);



} /* namespace ibkr */

#endif /* SOURCE_TRANCHE_H_ */
