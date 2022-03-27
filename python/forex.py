

class forex_tranche:
    def __init__(self, amount, eur_cost, eur_fee):
        self.balance = amount
        self.eur_fees = eur_fee
        self.eur_cost = eur_cost
        self.eur_rate = amount / eur_cost

    def book(self, amount, eur_fee):
        self.balance += amount
        self.eur_fees += eur_fee
        self.eur_cost += amount / self.eur_rate
        return self.eur_cost


class forex_acc:
    def __init__(self, symbol="EUR", balance=0):
        self.symbol = "EUR"
        self.balance = balance
        self.tranches = []

    def avg_eur_rate(self):
        fx = 0
        eur = 0
        for t in self.tranches:
            fx += t.balance
            eur += t.eur_cost
        return fx / eur

    def draw(self, amount):
        rest = amount
        eur = 0
        for t in self.tranches:
            if rest <= t.balance:
                eur += t.book(-rest, 0)
                rest = 0
            else:
                eur += t.book(t.balance, 0)
                rest -= t.balance
                
        return eur