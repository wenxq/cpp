{
	'includes': [
		'../template/base.gypi',
	],

	'targets': [
		{ # crypto_file:
			'target_name': 'crypto_file',
			'type': 'static_library',
			'standalone_static_library': 1,
			'dependencies': [
				'../template/base.gypi:base',
				'../common/common.gyp:common',
			],
			'configurations': {
				'Common_Base': {
					'msvs_configuration_attributes': {
						'CharacterSet': '2',
					},
				},
			},
			'include_dirs': [
				'./include',
				'../include',
			],
			'sources': [
				'./src/encode_file.cpp',
				'./src/decode_file.cpp',
				'./src/factory.cpp',
				'./src/utils.cpp',
			],
		},

		{ # cryptof_headers:
			'target_name': 'cryptof_headers',
			'type': 'none',
			'dependencies': [
				'crypto_file',
			],
			'copies': [{
				'destination': '<(ROOT_DIR)/include/cryptof',
				'files': [
					'./include/exception.h',
					'./include/utils.h',
					'./include/stream.h',
					'./include/interface.h',
					'./include/factory.h',
					'./include/encode_file.h',
					'./include/decode_file.h',
				]
			}],
		},

		{ # cryptof:
			'target_name': 'cryptof',
			'type': 'shared_library',
			'dependencies': [
				'crypto_file',
			],
			'configurations': {
				'Common_Base': {
					'msvs_configuration_attributes': {
						'CharacterSet': '2',
					},
				},
			},
			'include_dirs': [
				'./include',
				'../include',
			],
			'sources': [
				'./src/pywrapper.cpp',
			],
		},

		{ # cryptof_test:
			'target_name': 'cryptof_test',
			'type': 'executable',
			'dependencies': [
				'crypto_file',
			],
			'configurations': {
				'Common_Base': {
					'msvs_configuration_attributes': {
						'CharacterSet': '2',
					},
				},
			},
			'include_dirs': [
				'./include',
				'../include',
			],
			'sources': [
				'./test/manager.h',
				'./test/main.cpp',
			],
		},
	],
}
