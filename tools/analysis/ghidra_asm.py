import json,sys
d=json.load(open(sys.argv[1]))['structuredContent']['raw_result']['value']['assembly']
for i in d:
    if isinstance(i,dict): print(i.get('address'),i.get('text') or i.get('instruction') or i)
    else: print(i)
