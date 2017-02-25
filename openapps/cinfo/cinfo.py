import os
import sys
here = sys.path[0]
print here
sys.path.insert(0,os.path.join(here,'..','..','..','coap'))

from coap import coap
import signal

MOTE_IP = '::1'

c = coap.coap()

# read the information about the board status
c.POST('coap://[{0}]/res'.format(MOTE_IP),False,[],[10])



while True:
        input = raw_input("Done. Press q to close. ")
        if input=='q':
            print 'bye bye.'
            #c.close()
            os.kill(os.getpid(), signal.SIGTERM)
