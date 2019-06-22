{
  "targets": [{
    "target_name" : "macadam",
    "conditions": [
      ['OS=="mac"', {
        'sources' : [ "src/macadam_util.cc", "src/macadam.cc",
          "src/capture_promise.cc", "src/playback_promise.cc",
          "src/timecode.cc", "src/converted_frame.cc" ],
        'xcode_settings': {
          'GCC_ENABLE_CPP_RTTI': 'YES',
          'MACOSX_DEPLOYMENT_TARGET': '10.7',
          'OTHER_CPLUSPLUSFLAGS': [
            '-std=c++11',
            '-stdlib=libc++'
          ]
        },
        "link_settings": {
          "libraries": [
            '-F/Library/Frameworks',
            '-framework', 'DeckLinkAPI'
          ]
        },
        "include_dirs" : [
          "decklink/Mac/include"
        ]
      }],
      ['OS=="linux"', {
        'sources' : [ "src/macadam_util.cc", "src/macadam.cc",
          "src/capture_promise.cc", "src/playback_promise.cc",
          "src/timecode.cc", "src/converted_frame.cc" ],
        'link_settings' : {
          "libraries": [
            "/usr/lib/libDeckLinkAPI.so"
          ],
          "ldflags" : [
            "-lm -ldl -lpthread"
	      ]
        },
        "include_dirs" : [
          "decklink/Linux/include"
        ]
      }],
      ['OS=="win"', {
        "sources" : [ "src/macadam_util.cc", "src/macadam.cc",
          "src/capture_promise.cc", "src/playback_promise.cc",
          "src/timecode.cc", "src/converted_frame.cc",
          "decklink/Win/include/DeckLinkAPI_i.c" ],
        "configurations": {
          "Release": {
            "msvs_settings": {
              "VCCLCompilerTool": {
                "RuntimeTypeInfo": "true"
              }
            }
          }
        },
        "libraries": [
        ],
        "include_dirs" : [
          "decklink/Win/include"
        ]
      }]
    ]
  }]
}
