
import os
import re
import base64
import hashlib

in_dir = u'/home/wenxq/output'

for f in os.listdir(in_dir):
	if u'.img' in f:
		continue

	if re.match('^[a-z0-9]{32}$', f):
		continue

	if f.endswith('.cfg'):
		continue

	# if u'.' not in f:
	# 	continue

	m = hashlib.md5()
	fp = os.path.join(in_dir, f)
	with open(fp, 'rb') as inf:
		buf = inf.read(16 * 1024)
		assert buf
		m.update(buf)
	os.rename(fp, os.path.join(in_dir, m.hexdigest()))

print 'rename finish, next to add.py'
