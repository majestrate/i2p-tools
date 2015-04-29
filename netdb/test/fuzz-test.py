# test.py - Test netdb.py
# Author: Chris Barry <chris@barry.im>
# License: MIT

import netdb, os, logging, random

if __name__ == '__main__':
    logging.basicConfig(level=logging.INFO)

    # Make some garbage files and hope they break things.
    for i in range(1,1000):
        with open('{}/netdb/{}.dat'.format(os.environ['PWD'], i), 'wb') as fout:
            fout.write(os.urandom(random.randint(2,400))) # replace 1024 with size_kb if not unreasonably large

    # Now let's inspect the garbage.
    netdb.inspect(netdb_dir='{}/netdb/'.format(os.environ['PWD']))
