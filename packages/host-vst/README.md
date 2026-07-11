# @audio/host-vst

> Native VST3 plugin host — load, inspect params, process audio block-wise or whole-buffer, from Node. Environment-gated: needs the native addon built for your platform and a real plugin binary.

```js
import { load, scan } from '@audio/host-vst'
const plugin = load(scan()[0].path)
const out = plugin.processAll(channels)
plugin.close()
```

Part of [@audio/host](https://github.com/audiojs/host).
