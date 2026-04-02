{
  "targets": [{
    "target_name": "host_vst",
    "sources": ["native/host.cpp", "native/binding.cpp"],
    "include_dirs": ["native"],
    "cflags_cc": ["-std=c++17", "-O2", "-fno-exceptions"],
    "xcode_settings": {
      "OTHER_CPLUSPLUSFLAGS": ["-std=c++17", "-O2", "-fno-exceptions"],
      "CLANG_CXX_LANGUAGE_STANDARD": "c++17",
      "MACOSX_DEPLOYMENT_TARGET": "10.13"
    },
    "msvs_settings": {
      "VCCLCompilerTool": { "AdditionalOptions": ["/std:c++17", "/O2"] }
    },
    "conditions": [
      ["OS=='mac'", { "libraries": ["-framework CoreFoundation"] }],
      ["OS=='linux'", { "libraries": ["-ldl"] }]
    ]
  }]
}
