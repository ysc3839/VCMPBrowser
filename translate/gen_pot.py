from translate.storage.rc import rcfile
from translate.convert import rc2po
import re

orig_re_compile = re.compile
def my_re_compile(*args):
    re.compile = orig_re_compile
    largs = list(args)
    largs[0] = """
                 (?:
                 LANGUAGE\s+[^\n]*|                              # Language details
                 /\*.*?\*/[^\n]*|                                      # Comments
                 \/\/[^\n\r]*|                                  # One line comments
                 (?:[0-9A-Z_]+\s+(?:MENU|DIALOG|DIALOGEX|TEXTINCLUDE)|STRINGTABLE)\s  # Translatable section or include text (visual studio)
                 .*?
                 (?:
                 BEGIN(?:\s*?POPUP.*?BEGIN.*?END\s*?)+?END|BEGIN.*?END|  # FIXME Need a much better approach to nesting menus
                 {(?:\s*?POPUP.*?{.*?}\s*?)+?}|{.*?})+[\n]|
                 \s*[\n]|         # Whitespace
                 \#[^\n\r]*       # Preprocessor directives
                 )
                 """
    return orig_re_compile(*largs)

class myrcfile(rcfile):
    def __init__(self, inputfile=None, lang=None, sublang=None, **kwargs):
        super(rcfile, self).__init__(**kwargs)
        self.filename = getattr(inputfile, 'name', '')
        self.lang = lang
        self.sublang = sublang
        self.unit_names = set()
        re.compile = my_re_compile
        self.parse(inputfile.read())

    def addunit(self, unit):
        if unit.name in self.unit_names:
            i = 1
            while "{}_{}".format(unit.name, i) in self.unit_names:
                i += 1
            unit.name = "{}_{}".format(unit.name, i)
        self.unit_names.add(unit.name)
        super(rcfile, self).addunit(unit)

def gen_pot():
    rc2po.rc.rcfile = myrcfile

    infile = open('../langs/en-US.rc', 'r', encoding='utf-8')
    outfile = open('en-US.pot', 'wb')
    rc2po.convertrc(infile, outfile, None, pot=True, charset='utf-16', lang='LANG_ENGLISH', sublang='SUBLANG_ENGLISH_US')

if __name__ == '__main__':
    gen_pot()
