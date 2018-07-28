import os
import requests
from gen_pot import gen_pot

if __name__ == '__main__':
    if not 'TRANSIFEX_TOKEN' in os.environ:
        print('env var "TRANSIFEX_TOKEN" not defined.')
        exit(1)
    gen_pot()

    url = 'https://www.transifex.com/api/2/project/vcmpbrowser/resource/VCMPBrowser/content/'
    auth = ('api', os.environ['TRANSIFEX_TOKEN'])
    r = requests.put(url, auth=auth, files={'file': open('en-US.pot', 'rb')})
    r.raise_for_status()
    print(r)
    print(r.text)
