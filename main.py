
import sys
import argparse
import pandas as pd
import numpy as np

from datetime import date, timedelta


matches = \
{
    'Realized & Unrealized Performance Summary':'performance',
    'Open Positions':'positions',
    'Forex Balances':'forex',
    'Trades':'trades',
    'Forex P/L Details':'swaps',
    'Dividends':'dividends',
}


def daterange(start_date, end_date):
    for n in range(int((end_date - start_date).days)):
        yield start_date + timedelta(n)


def df_of_day(df, day):
    mask = (df['Date/Time'] >= np.datetime64(day)) & (df['Date/Time'] < (np.datetime64(day) + np.timedelta64(1,'D')))
    return df.loc[mask]


def process_day(data, day):
    daily_swaps = df_of_day(data['swaps'], day)
    daily_trades = df_of_day(data['trades'], day)

    print(day.strftime("%Y-%m-%d: "))
    if not daily_swaps.empty or not daily_trades.empty:
        print(daily_swaps)
        print(daily_trades)


def replay(data): 
    start_date = date(2021, 6, 23)
    end_date = date(2021, 6, 23)
    for day in daterange(start_date, end_date + timedelta(days=1)):
        process_day(data, day)


def main(args):
    err = 0
    offset = 0
    headings = []
    data = {}

    for l in args.input_file.readlines():
        offset += 1
        if ',Header' in l:
            headings.append((offset, l.split(',')[0]))

    headings.append((offset, 'EOF'))
    print('Found headings: %s' % headings)

    args.input_file.seek(0)
    for idx in range(0, len(headings)):
        if headings[idx][1] in matches.keys():
            p0 = headings[idx][0] - 1 
            p1 = headings[idx+1][0] - 1
            print('Reading section %d:%d' % (p0, p1))
            df = pd.read_csv(args.input_file, sep=",", quoting=0, quotechar='"', header=0, skiprows=p0, nrows=p1-p0-1)
            if 'Date/Time' in df:
                df['Date/Time'] = pd.to_datetime(df['Date/Time'], format="%Y-%m-%d, %H:%M:%S", errors="coerce")
            else:
                print("no dates in %s" % headings[idx][1])

            data[matches[headings[idx][1]]] = df
            args.input_file.seek(0)

    print(data['swaps'])

    replay(data)

    return err


if __name__ == '__main__':     
    # Instantiate the parser
    parser = argparse.ArgumentParser(description='IBKR tax accounting for Austria')

    parser.add_argument('input_file', type=argparse.FileType('r', encoding='latin-1'), help="IBKR csv export")

    args = parser.parse_args()
    sys.exit(main(args))
