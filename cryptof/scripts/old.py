
import os
import base64
import kit

in_dir = u'/home/wenxq/output'

assert os.path.isdir(in_dir)

kit.cur.execute('SELECT uuid, filename, encode FROM `baiduyun_map`')
records = kit.cur.fetchall()
for r in records:
	uuid, filename, encode = r
	filename = base64.b64decode(filename).decode('gbk')
	if kit.is_have(uuid):
		print '%s is exists in database.' % filename
		continue
	kit.insert(uuid, filename, encode)

kit.commit()
kit.close()
