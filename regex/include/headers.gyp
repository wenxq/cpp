{
	'includes': [
		'../../template/base.gypi',
	],

	'targets' : [
		{ # regex_install_headers
			'target_name': 'regex_install_headers',
			'type': 'none',
			'dependencies': [
				'../src/regexobj.gyp:regex',
			],
			'copies': [{
					'destination': '<(ROOT_DIR)/include/regex',
					'files': [
						'./defines.h',
						'./trie.h',
						'./regex.h',
						'./qmatch.h',
					]
			}],
		},
	],
}
