{
	'includes': [
		'../../template/base.gypi',
	],

	'target_defaults': {
		'dependencies': [
			'../src/regexobj.gyp:regex',
		],
		'include_dirs': [
			'../src',
			'../../include',
		],
	},

	'targets': [
		{ # test_trie
			'target_name': 'test_trie',
			'type': 'executable',
			'sources': [
				'./test_trie.cc',
			],
		},

		{ # test_regex
			'target_name': 'test_regex',
			'type': 'executable',
			'sources': [
				'./test_regex.cc',
			],
		},

		{ # regex_test
			'target_name': 'regex_test',
			'type': 'executable',
			'sources': [
				'./regex_test.cc',
			],
		},

		{ # test_regexobj
			'target_name': 'test_regexobj',
			'type': 'executable',
			'sources': [
				'./test_regexobj.cc',
			],
		},

		{ # test_qmatch
			'target_name': 'test_qmatch',
			'type': 'executable',
			'sources': [
				'./test_qmatch.cc',
			],
		},
	],
}
