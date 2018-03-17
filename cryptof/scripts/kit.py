
import os
import base64
import sqlite3

def basename(filename):
	if os.path.sep in filename:
		return os.path.basename(filename)
	else:
		return filename.split(os.path.sep == '/' and '\\' or '/')[-1]

database = u'data.sqlite3'

db  = sqlite3.connect(database)
cur = db.cursor()

def list_files():
	cur.execute('SELECT filename from `baiduyun_meta`')
	for filename in cur.fetchall():
		fn = filename[0]
		print basename(base64.b64decode(fn))

def find_file(name):
	filename = base64.b64encode(name.encode('utf-8'))
	cur.execute('SELECT COUNT(*) FROM `baiduyun_meta` WHERE filename LIKE "%%%s%%"' \
				% filename)
	x = int(cur.fetchone()[0])
	return x > 0

def is_have(uuid):
	cur.execute('SELECT COUNT(*) FROM `baiduyun_meta` WHERE uuid="%s"' % uuid)
	x = int(cur.fetchone()[0])
	return x > 0

def insert(uuid, name):
	filename = base64.b64encode(name.encode('utf-8'))
	cur.execute('INSERT INTO `baiduyun_meta` VALUES("%s", "%s", 1, "utf-8")' \
				% (uuid, filename))

def commit():
	db.commit()

def close():
	cur.close()
	db.close()
