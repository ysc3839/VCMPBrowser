import gen_pot
from translate.storage import rc
from translate.convert import po2rc
import six

class tmpstring:
    def __init__(self, value):
        self.value = value

    def encode(self, charset):
        return self.value

orig_escape_to_rc = rc.escape_to_rc
def myescape_to_rc(string):
    return tmpstring(orig_escape_to_rc(string))

class myrerc(po2rc.rerc):
    def makestoredict(self, store, includefuzzy=False):
        rc.escape_to_rc = myescape_to_rc
        super().makestoredict(store, includefuzzy)
        rc.escape_to_rc = orig_escape_to_rc

    def convertblock(self, block):
        orig_six_text_type = six.text_type
        six.text_type = type(None)
        retval = super().convertblock(block)
        six.text_type = orig_six_text_type
        return retval

def convert_to_rc(infilename, outfilename, lang, sublang):
    po2rc.rc.rcfile = gen_pot.myrcfile
    po2rc.rerc = myrerc

    infile = open(infilename, 'rb')
    outfile = open(outfilename, 'w', encoding='utf-8')
    templatefile = open('../langs/en-US.rc', 'r', encoding='utf-8')
    po2rc.convertrc(infile, outfile, templatefile, lang=lang, sublang=sublang)

if __name__ == '__main__':
    convert_to_rc('en-US.po', 'test.rc', 'LANG_ENGLISH', 'SUBLANG_ENGLISH_US')
