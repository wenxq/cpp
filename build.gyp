{
	'includes': [
		'./template/common.gypi',
	],

	'targets': [
		{ # all:
			'target_name': 'all',
			'type': 'none',
			'dependencies': [
				'./common/common.gyp:*',
				'./cryptof/cryptof.gyp:*',
				'./regex/regex.gyp:*',
			],
		},
	],
}

