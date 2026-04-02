{
  "targets": [{
    "target_name": "host_clap",
    "sources": ["native/host.c", "native/binding.c"],
    "include_dirs": ["native"],
    "cflags": ["-std=c99", "-O2"],
    "xcode_settings": {
      "OTHER_CFLAGS": ["-std=c99", "-O2"],
      "MACOSX_DEPLOYMENT_TARGET": "10.13"
    },
    "msvs_settings": {
      "VCCLCompilerTool": { "AdditionalOptions": ["/O2"] }
    },
    "conditions": [
      ["OS=='linux'", { "libraries": ["-ldl"] }]
    ]
  }]
}
