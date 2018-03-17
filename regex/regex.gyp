{
	'includes': [
		'../template/base.gypi',
	],

	'targets': [
		{ # regex_build:
			'target_name': 'regex_build',
			'type': 'none',
			'dependencies': [
				'./include/headers.gyp:*',
				'./src/regexobj.gyp:*',
				'./test/test.gyp:*',
			],
		},
	],
}
