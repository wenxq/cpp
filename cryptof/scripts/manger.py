# -*- coding: utf-8 -*-

__author__ = 'xingquan.wxq'

import os
import base64
import cryptof
import kit

class Manager:
    def __init__(self):
        self.encoder = {}
        self.decoder = {}
        self.default_key = 'a2b4c6d8e0,'

    def _add_record(self, tarfile, fp, version):
        #baiduyun_map(uuid, filename, encode)
        pass

    def push(self, tarfile, fp, key=None, version=None):
        assert tarfile and fp
        if not key:
            key = self.default_key

        if not version:
            version = (1, 0)

        if tarfile not in self.encoder:
            self.encoder[tarfile] = cryptof.Encoder(tarfile, key)
        encoder = self.encoder[tarfile]
        if os.path.isdir(fp):
            files = []
            for f in os.listdir(fp):
                fpath = os.path.join(fp, f)
                if os.path.isfile(fpath):
                    if not encoder.push(fpath, version):
                        return (False, [ fpath ])
                    files.append(fpath)
                    self._add_record(tarfile, fpath, version)
                else:
                    result = self.push(tarfile, fpath, key, version)
                    if not result[0]:
                        return result
                    files.extend(result[1])
            return (True, files)
        else:
            if not encoder.push(fp, version):
                return (False, [ fp ])
            self._add_record(tarfile, fp, version)
            return (True, [ fp ])

    def pop(self, tarfile, fp, output=None, key=None, prefix=None):
        assert tarfile and fp
        if not key:
            key = self.default_key

        if tarfile not in self.decoder:
            self.decoder[tarfile] = cryptof.Decoder(tarfile, key)

        decoder = self.decoder[tarfile]
        files = decoder.filelist()
        if fp in files:
            assert output
            result = decoder.pop(fp, output)
            return (result, [ fp ])

        assert os.path.isdir(fp)
        for f in files:
            if prefix:
                if not f.startswith(prefix):
                    continue
                output = os.path.join(fp, f.split(prefix)[1].lstrip(os.path.sep))
            else:
                output = os.path.join(fp, os.path.basename(f))
            if not decoder.pop(f, output):
                return (False, [ f ])

        return (True, files)

    def get_encoder(self, tarfile):
        assert tarfile
        if tarfile not in self.encoder:
            return None
        return self.encoder[tarfile]

    def get_decoder(self, tarfile, key=None):
        assert tarfile
        if not key:
            key = self.default_key

        if tarfile not in self.decoder:
            self.decoder[tarfile] = cryptof.Decoder(tarfile, key)
        return self.decoder[tarfile]

    def encoder_files(self):
        return self.encoder.keys()

    def decoder_files(self):
        return self.decoder.keys()

if __name__ == '__main__':
    from time import sleep
    from tqdm import tqdm

    print '-' * 16

    in_dir  = u'/home/wenxq/input'
    out_dir = u'/home/wenxq/output'
    imgtar  = os.path.join("/home/wenxq/img", 'star.img')
    txttar  = os.path.join("/home/wenxq/img", 'stor.txt')

    def is_image(f):
        image = (u'.jpg', u'.gif', u'.png')
        for t in image:
            if f.endswith(t):
                return True
        return False

    def is_text(f):
        text = (u'.txt', )
        for t in text:
            if f.endswith(t):
                return True
        return False

    manger = Manager()

    print 'encode ...'

    try:
        files = os.listdir(in_dir)
        for i in tqdm(xrange(1, len(files) + 1)):
            f = files[i-1]
            fp = os.path.join(in_dir, f)
            if is_image(f):
                manger.push(imgtar, fp)
            elif is_text(f):
                manger.push(txttar, fp)
            else:
                new_file = os.path.join(out_dir, f)
                manger.push(new_file, fp)
    finally:
        #print 'decode ...'
        # tarfile = u'/home/wenxq/eb945d1596bba25e2bfdb81d8455276a'
        # for i in manger.get_decoder(tarfile).filelist():
        #     print i
        #manger.pop(tarfile, out_dir)
		print '-' * 16
