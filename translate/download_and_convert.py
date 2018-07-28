import os
import requests
from convert_po import convert_to_rc

languages = {
    'ar': ('LANG_ARABIC', 'SUBLANG_DEFAULT'),
    'bn': ('LANG_BENGALI', 'SUBLANG_BENGALI_BANGLADESH'),
    'zh-CN': ('LANG_CHINESE', 'SUBLANG_CHINESE_SIMPLIFIED'),
    'nl': ('LANG_DUTCH', 'SUBLANG_DEFAULT'),
    'pl': ('LANG_POLISH', 'SUBLANG_DEFAULT'),
    'pt-BR': ('LANG_PORTUGUESE', 'SUBLANG_PORTUGUESE_BRAZILIAN'),
    'es': ('LANG_SPANISH', 'SUBLANG_SPANISH_MODERN'),
    'tr': ('LANG_TURKISH', 'SUBLANG_DEFAULT')
}

if not 'TRANSIFEX_TOKEN' in os.environ:
    print('env var "TRANSIFEX_TOKEN" not defined.')
    exit(1)

auth = ('api', os.environ['TRANSIFEX_TOKEN'])

for lang in languages.items():
    lang_code = lang[0]
    print(lang_code)
    url = 'https://www.transifex.com/api/2/project/vcmpbrowser/resource/VCMPBrowser/translation/{}/?mode=reviewed&file'.format(lang_code)
    r = requests.get(url, auth=auth)
    r.raise_for_status()
    print(r)
    with open(lang_code + '.po', 'wb') as fd:
        for chunk in r.iter_content(chunk_size=128):
            fd.write(chunk)
    convert_to_rc(lang_code + '.po', '../langs/{}.rc'.format(lang_code), lang[1][0], lang[1][1])
