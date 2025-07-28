/*
 * tranche.h
 *
 *  Created on: 27.03.2022
 *      Author: AndreasFellnhofer
 */



#ifndef SOURCE_TRANCHE_H_
#define SOURCE_TRANCHE_H_

#include "security.hpp"
#include "currency.hpp"

#include <asm-generic/errno.h>
#include <iostream>
#include <ctime>
#include <memory>


namespace ibkr
{

class tranche
{
public:
	enum e_ordertype
	{
		BUY,
		SELL,
		HOLD
	};

	tranche() = delete;
	tranche(const security &security, double quanti, const currency::price price, const currency::price fee, bool sell = false);

	inline void setType(enum e_ordertype ot) {ordertype = ot; }
	inline void setEcbRate(double rate) { ecb_rate = rate; }
	inline security &getSecurity() { return sec; }
	inline const security &getSecurity() const { return sec; }
	inline const currency::price &getFee() const { return fee; }
	inline const currency::price &getPrice() const { return price; }
	inline const enum e_ordertype &getType() const { return ordertype; }
	inline const double getQuanti() const { return quanti; }
	inline const double unfilled() const { return quanti - filled; }
	inline const double getEcbRate() const { return ecb_rate; }
	inline void fill(double fill) { filled += fill; }
	inline void unfill(void) { filled = 0.0; }

	inline const std::tm &getTimeStamp() const { return timestamp; }
	inline void setTimeStamp(std::tm const &tm) { timestamp = tm; }

	void makeAbsolute(void);
	inline bool isSell(void) const { return ordertype == SELL; }
	inline bool isHold(void) const { return ordertype == HOLD; }

	inline void setQuanti(double d) { quanti = d; }
	inline double getQuanti(void) { return quanti; }

	~tranche() { };

private:
	security sec;
	double quanti;
	double filled;
	std::tm timestamp;
	currency::price price;
	currency::price fee;
	double ecb_rate;
	enum e_ordertype ordertype;
};


std::ostream &operator<<(std::ostream &, const tranche &);
bool tranche_compare(std::shared_ptr<ibkr::tranche> t1, std::shared_ptr<ibkr::tranche> t2);
bool tranche_date_compare(ibkr::tranche const &t1, ibkr::tranche const &t2);

} /* namespace ibkr */

#endif /* SOURCE_TRANCHE_H_ */
