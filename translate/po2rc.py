from translate.storage import po

f = open('template.rc', 'r', encoding='utf-16')
template_rc = f.read()
f.close()

f = open('en-US.po', 'rb')
store = po.pofile(f.read())
f.close()

for unit in store.units:
    if not unit.isfuzzy():
        if unit.msgid[0] and unit.msgstr[0]:
            template_rc = template_rc.replace(unit.msgid[0], unit.msgstr[0])

f = open('o.rc', 'w', encoding='utf-16')
f.write(template_rc)
f.close()
