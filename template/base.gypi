{
	'includes': [
		'./common.gypi',
	],

	'targets': [
		{ # base:
			'target_name': 'base',
			'type': 'none',
			'all_dependent_settings': {
				'defines': [
					'_HAVE_CXX11_=1',
					'NO_TR1=0',
				],
				'include_dirs': [
					'<(ROOT_DIR)/3rd/include',
					'<(ROOT_DIR)/3rd/include/google',
				],
				'conditions': [
					['OS=="linux"', {
						'include_dirs': [
							'/usr/include/python2.7',
						],
						'cflags': [
							'-ftemplate-depth-64',
							'-fpermissive',
							'-std=c++0x',
							'-g',
						],
						'cflags!': [
							'-fvisibility=hidden',
						],
						'library_dirs': [
							'<(ROOT_DIR)/3rd/lib',
						],
						'link_settings': {
							'libraries': [
								'-lpython2.7',
								'-lcryptopp',
								#'-rdynamic -ldouble-conversion',
							],
						},
					}],
					['OS=="win"', {
						'include_dirs': [
							'D:\\Python27\\include',
						],
						'msvs_settings': {
							'VCLinkerTool': {
								'AdditionalLibraryDirectories': [
									'D:\\Python27\\libs',
									'<(ROOT_DIR)\\3rd\\lib\\$(Platform)\\$(Configuration)',
								],
								'AdditionalDependencies': [
									'python27.lib',
									'cryptopp.lib',
									'double-conversion.lib',
								],
							},
							'VCLibrarianTool': {
								'AdditionalLibraryDirectories': [
									'D:\\Python27\\libs',
									'<(ROOT_DIR)\\3rd\\lib\\$(Platform)\\$(Configuration)',
								],
							},
						},
					}],
				],
			},
		},

		{ # gtest:
			'target_name': 'gtest',
			'type': 'none',
			'dependencies': [
				'base',
			],
			'all_dependent_settings': {
				'conditions': [
					['OS=="linux"', {
						'defines': [
							'GTEST_HAS_TR1_TUPLE=1',
							'GTEST_LANG_CXX11=0'
						],
						'link_settings': {
							'libraries': [
								'-lgmock',
							],
						},
					}],
					['OS=="win"', {
						'VCLinkerTool': {
							'AdditionalDependencies': [
								'gmock.lib',
							],
						},
					}],
				],
			},
		},
	],
}
