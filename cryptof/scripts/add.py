
import os
import cryptof
import kit

in_dir = u'/home/wenxq/output'

assert os.path.isdir(in_dir)

for fp in os.listdir(in_dir):
	if kit.is_have(kit.basename(fp)):
		print '%s is exists in database.' % fp
		continue
	decoder = cryptof.Decoder(os.path.join(in_dir, fp))
	for k, v in decoder.filelist().items():
		print fp
		kit.insert(kit.basename(fp), k)

kit.commit()
kit.close()
