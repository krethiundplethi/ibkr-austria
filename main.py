
import sys
import argparse



def main(args):
    err = 0

    print(args.input-file)

    return err


if __name__ == '__main__':     
    # Instantiate the parser
    parser = argparse.ArgumentParser(description='IBKR tax accounting for Austria')

    parser.add_argument('input-file', type=argparse.FileType('r', encoding='latin-1'), help="IBKR csv export")

    sys.exit(main(parser.parse_args()))
