
{
  'includes': [
    '../../build/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'dia_sdk',
      'type': 'none',
      'all_dependent_settings': {
        'include_dirs': [
          '<(DEPTH)',
          '$(VSInstallDir)/DIA SDK/include',
        ],
        'msvs_settings': {
          'VCLinkerTool': {
            'AdditionalDependencies': [
              'diaguids.lib',
              'imagehlp.lib',
            ],
          },
        },
        'configurations': {
          'x86_Base': {
            'msvs_settings': {
              'VCLinkerTool': {
                'AdditionalLibraryDirectories':
                  ['$(VSInstallDir)/DIA SDK/lib'],
              },
            },
          },
          'x64_Base': {
            'msvs_settings': {
              'VCLinkerTool': {
                'AdditionalLibraryDirectories':
                  ['$(VSInstallDir)/DIA SDK/lib/amd64'],
              },
            },
          },
        },
      },
    },
    {
      'target_name': 'common_windows_lib',
      'type': 'static_library',
      'sources': [
        'dia_util.cc',
        'dia_util.h',
        'guid_string.cc',
        'guid_string.h',
        'http_upload.cc',
        'http_upload.h',
        'omap.cc',
        'omap.h',
        'omap_internal.h',
        'pdb_source_line_writer.cc',
        'pdb_source_line_writer.h',
        'string_utils.cc',
        'string_utils-inl.h',
      ],
      'dependencies': [
        'dia_sdk',
      ],
    },
    {
      'target_name': 'common_windows_unittests',
      'type': 'executable',
      'sources': [
        'omap_unittest.cc',
      ],
      'dependencies': [
        '<(DEPTH)/client/windows/unittests/testing.gyp:gmock',
        '<(DEPTH)/client/windows/unittests/testing.gyp:gtest',
        'common_windows_lib',
      ],
    },
  ],
}
