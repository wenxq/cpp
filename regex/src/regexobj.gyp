{
	'includes': [
		'../../template/base.gypi',
	],

	'targets': [
		{ # regex_base:
			'target_name': 'regex_base',
			'type': 'none',
			'dependencies': [
				'../../template/base.gypi:base',
                '../../common/common.gyp:common',
			],
			'all_dependent_settings': {
				'include_dirs': [
					'<(ROOT_DIR)/3rd/include/pcre2',
					'../include',
				],
				'conditions': [
					['OS=="linux"', {
						'defines' : [
							'IS_UNIX',
						],
						'link_settings': {
							'libraries': [
								'-Wl,-Bstatic,--whole-archive -lpcre2-8 -Wl,-Bdynamic,--no-whole-archive', # pcre2-8
							],
						},
					}],
					['OS=="win"', {
						'defines': [
							'IS_WIN32',
						],
						'msvs_settings': {
							'VCLinkerTool': {
								'AdditionalDependencies': [
									'pcre2-8.lib',
								],
							},
						},
					}],
				],
			},
		},

		{ # regex 
			'target_name': 'regex',
			'type': 'static_library',
			'standalone_static_library': 1,
			'dependencies': [
				'regex_base',
			],
			'configurations': {
				'Release_Base': {
					'defines': [
						'NDEBUG_LOG',
					],
				},
			},
			'defines': [
				'REGEX_MAX_NODES=307',
			],
			'include_dirs': [
				'../../include',
				'.',
			],
			'sources': [
				'./trie_tree.h',
				'./trie_tree.cc',
				'./regex_object.h',
				'./regex_object.cc',
				'./regex.cpp',
				'./qmatch.cpp',
				'./trie.cpp',
			],
		},
	],
}
