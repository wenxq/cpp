# -*- coding: utf-8 -*-

__author__ = 'xingquan.wxq'

import os
import sys
from manger import Manager

if __name__ == '__main__':
    from time import sleep
    if len(sys.argv) < 2:
        print 'Uage: %s [inputs]' % sys.argv[0]
        sys.exit(0)

    manger = Manager()
    tarfile = u'test.out'

    print 'encode ...'
    files = sys.argv[1:]
    for f in files:
        manger.push(tarfile, f)

    print 'decode ...'
    for f in files:
        temp = f + u'.tmp'
        manger.pop(tarfile, f, temp)

        print 'compare: %s' % f
        ret = os.system(u'diff %s %s' % (f, temp))
        os.system(u'rm -f %s' % temp)
        if ret != 0:
            print 'diffenerce %s' % f
    os.system(u'rm -f %s' % tarfile)

