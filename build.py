#!/usr/bin/env python
#coding: utf-8

import os
import sys
import shutil
import platform
from optparse import OptionParser

def win_set_color(c):
	import ctypes
	STD_OUTPUT_HANDLE = -11
	handler = ctypes.windll.kernel32.GetStdHandle(STD_OUTPUT_HANDLE)
	ctypes.windll.kernel32.SetConsoleTextAttribute(handler, c)

def win_print_color(c, s):
	win_set_color(c)
	print s
	win_set_color(7)

def win_red(s):
	win_print_color(4, s)

def win_green(s):
	win_print_color(2, s)

def win_blue(s):
	win_print_color(6, s)

def unix_red(s):
	print '\033[31m' + s + '\033[0m'

def unix_green(s):
	print '\033[32m' + s + '\033[0m'

def unix_blue(s):
	print '\033[36m' + s + '\033[0m'

def run_command(cmd):
	blue('shell: ' + cmd)
	if os.system(cmd) != 0:
		red('failed shell: ' + cmd)
		sys.exit(1)

_src_dir = os.getcwd()
#_install_root = os.path.dirname(_src_dir)
_install_root = _src_dir
_sysstr = platform.system().lower()
if _sysstr in ('unix', 'linux'):
	red = unix_red
	blue = unix_blue
	green = unix_green
elif _sysstr == 'windows':
	red = win_red
	blue = win_blue
	green = win_green

def copy(src_dir, fp, dest_dir):
	src = os.path.join(src_dir, fp)
	dest = os.path.join(dest_dir, fp)
	if os.path.isdir(src):
		shutil.copytree(src, dest)
	elif os.path.isfile(src):
		shutil.copy(src, dest)

def mktree(full_path):
    if os.path.isdir(full_path):
       return None
    os.makedirs(full_path)
#     queue, child = [], full_path
#     queue.append(child)
#     while 1:
#         parent = child.rsplit(os.path.sep, 1)
#         assert parent
#         if len(parent) == 1 or os.path.isdir(parent[0]):
#            break
#         child = parent[0]
#         queue.append(child)
#     queue.reverse()
#     for d in queue:
#         assert not os.path.isdir(d)
#         os.mkdir(d)

def unix_build(build_type='Release', pl='x64'):
	green('\n%s build %s.%s %s' % ('='*32, pl, build_type, '='*32))
	pl = {
		'x86': 'x86',
		'x64': 'x64',
	}[pl]

	args = [
		'--depth=.',
		'--generator-output=build',
		'-D BUILD_TYPE=%s' % build_type,
		'-D ROOT_DIR="%s"' % _install_root,
	]
	if pl == 'x86':
		args.append('-D target_arch=ia32')
	gyp = os.path.join('.', 'build.gyp')
	cmd = ' '.join(['gyp', gyp] + args)
	run_command(cmd)

	cmd = 'cd %s && make -j4 BUILDTYPE=%s' % (os.path.join('.', 'build'), build_type)
	run_command(cmd)

	plf = platform.platform()
	if 'centos' in plf:
		src_dir = 'centos65_%s' % pl
	else:
		red('not support system: %s' % plf)
		sys.exit(1)

	from_dir = os.path.join(_src_dir, 'build', 'out', build_type)
	to_dir = os.path.join(_install_root, '3rd', 'lib') + os.path.sep
	mktree(to_dir)
	files = [
	]
	release_files = [
	]
	green('copy from %s to %s' % (from_dir, to_dir))
	for fp in files:
		green('copy: ' + fp)
		copy(from_dir, fp, to_dir)
	if build_type == 'Release':
		for fp in release_files:
			green('copy: ' + fp)
			copy(from_dir, fp, to_dir)
	return None

def windows_build(build_type='Release', pl='x64'):
	green('\n%s build %s.%s %s' % ('='*16, pl, build_type, '='*16))
	pl = {
		'x86': 'Win32',
		'x64': 'x64',
	}[pl]

	args = [
		'--depth=.',
		'--generator-output=build',
		'-G msvs_version=2015',
		'-D BUILD_TYPE=%s' % build_type,
		'-D ROOT_DIR="%s"' % _install_root,
	]
	gyp = os.path.join('.', 'build.gyp')
	cmd = ' '.join(['gyp', gyp] + args)
	run_command(cmd)

	args = [
		'/m',
		'/nologo',
		'/consoleloggerparameters:ErrorsOnly;PerformanceSummary;Summary',
		'/p:Configuration=%s;Platform=%s' % (build_type, pl),
	]

	sln = os.path.join('.', 'build', 'build.sln')
	cmd = ' '.join(['msbuild', sln] + args)
	run_command(cmd)

	from_dir = os.path.join(_src_dir, 'build', pl, build_type)
	to_dir = os.path.join(_install_root, '3rd', 'lib', pl, build_type) + os.path.sep
	mktree(to_dir)
	files = [
	]
	release_files = [
	]
	green('copy from %s to %s' % (from_dir, to_dir))
	for fp in files:
		green('copy: ' + fp)
		copy(from_dir, fp, to_dir)
	if build_type == 'Release':
		for fp in release_files:
			green('copy: ' + fp)
			copy(from_dir, fp, to_dir)
	return None

def build_package(build_type='Release', pl='x64'):
	if _sysstr in ('unix', 'linux'):
		return unix_build(build_type, pl)
	elif _sysstr == 'windows':
		return windows_build(build_type, pl)
	else:
		print >> sys.stderr, '[ERROR] NOT SUPPORT SYSTEM: %s' % _sysstr
		sys.exit(1)

def build_all(option):
	#build_types:
	tmp = []
	if not option.build_types:
		tmp.append('Release')
	for tp in option.build_types:
		tp = tp.lower()
		if tp not in tmp and tp in ('release', 'debug'):
			tp = tp[0:1].upper() + tp[1:]
			if tp not in tmp:
				tmp.append(tp)
	build_types = tmp
	if not build_types:
		return None

	#platforms:
	tmp = []
	if not option.platforms:
		tmp.append(_sysstr in ('unix', 'linux') and 'x64' or 'x86')
	for pl in option.platforms:
		pl = pl.lower()
		if pl not in tmp and pl in ('x64', 'x86'):
			tmp.append(pl)
	platforms = tmp
	if not platforms:
		return None

	#print platforms, build_types
	def get_build_type_list():
		return ('Debug', 'Release')

	def get_platform_list():
			return ('x86', 'x64')

	post_build = None
	for pl in get_platform_list():
		for bt in get_build_type_list():
			if pl in platforms and bt in build_types:
				pb = build_package(bt, pl)
				if pb:
					post_build = pb

	if post_build:
		post_build()

if __name__ == '__main__':
	parser = OptionParser()
	parser.add_option('--all', dest='all',
					  default=False, action='store_true',
					  help=u'编译相关平台所有库')
	parser.add_option('--build-type', dest='build_types',
					  default=[], action='append',
					  help=u'编译类型: Release|Debug', metavar='BUILDTYPE')
	parser.add_option('--platform', dest='platforms',
					  default=[], action='append',
					  help=u'编译类型PL: x64|x86', metavar='PLATFORM')
	option, args = parser.parse_args()

	if option.all:
		if _sysstr in ('windows', 'unix', 'linux'):
			if not option.platforms:
				option.platforms = ['x86', 'x64']
			if not option.build_types:
				option.build_types = ['Debug', 'Release']
		else:
			print >> sys.stderr, '[ERROR] NOT SUPPORT SYSTEM: %s' % _sysstr
			sys.exit(1)
	build_all(option)
