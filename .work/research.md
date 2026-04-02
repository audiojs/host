## Context

The project is developed as part of audiojs org (github.com/audiojs) — "plumbing platform audio."

Audiojs maintains: portable Web Audio API for Node.js with 100% WPT conformance, web-audio-stream, audio-context singleton, and various audio utilities.

## Goal

Universal plugin host: VST, WAM, LV2, CLAP — bridging professional/desktop plugin ecosystems with modern web audio.

---

# ∞ INTENTION — OFFERING OR EGO TRIP?

**Real motive check:**
Is this about serving musicians/producers who suffer — or proving we can build something technically sophisticated?

The honest answer matters because it shapes everything: feature scope, UI, simplicity, documentation, distribution.

**What problem wants solving that's REAL, not hypothetical?**
- Musicians exist. They have plugin chains. They switch between computers. They break their live rigs.
- Developers exist. They maintain VST + LV2 + CLAP versions of the same plugin. They hate it.
- Web audio exists. It's powerful. It can't talk to your studio gear.

**These are real people's actual hours lost, not convenience features.**

**What would we be refusing?**
- Refusing scope creep: we'd say no to DAW features, sequencers, mixer polish that aren't core to "hosting plugins reliably."
- Refusing to be everything: we'd pick one hard job and nail it, not 10 soft jobs.
- Refusing ego: if someone else solves this better, we rejoice. We're not proving we're the only ones who can do this.

**What would make this an offering vs a performance?**
- It would work so well that it disappears — people forget they're using a host, just use their plugins.
- It would be humble: radically simple UI, no branding, no "look how clever we were" moments.
- It would serve people already struggling, not convince people they need a problem.

---

# 1. EXPLORE — WHAT PROBLEM WANTS SOLVING?

## The Territory: What Exists

### Plugin Standards Landscape (2026)

**CLAP (CLever Audio Plugin)** — *The Rising Standard*
- Born 2022, backed by Bitwig, u-he, others. Pure C interface, single header file.
- **Why it matters:** Open-source (MIT), MIDI 2.0 native, simpler threading than VST3, no proprietary licensing friction.
- **Status:** Rapidly adopted by new plugin developers. VST2 is dying, VST3 is complex, LV2 is over-engineered.
- **Momentum:** Every new open-source plugin wants CLAP support. Proprietary plugins (iZotope, Waves, Universal Audio) haven't moved yet.

**VST3** — *Professional Standard, Complex*
- Steinberg's modern format, widely supported, but complex threading model, higher barrier to entry for plugin devs.
- **Status:** Required for professional audio. No pure open-source governance.
- **Problem:** VST2 plugins outnumber VST3 by 20:1. Migration is slow.

**LV2** — *Over-Engineered*
- Linux standard, open-source, but sprawling specification. "Over-engineered" is the repeated criticism.
- **Status:** Entrenched on Linux, dying elsewhere. Developers avoid it due to complexity.
- **Problem:** Only host that speaks pure LV2 well is Carla. Everything else is partial.

**Audio Units (AU)** — *Mac Only*
- Apple's format, Mac/iOS exclusive. AU3 is moving toward VST3-like architecture.
- **Status:** Required if you want Mac users.
- **Problem:** Completely separate universe, proprietary, no Linux/Windows.

**Web Audio Modules (WAM)** — *Browser Plugins, Emerging*
- The "VST of the web." Standard for Web Audio API plugins. Load dynamically from the web.
- **Status:** 2026 — becoming real. AudioWorklet + WebAssembly enables near-native performance.
- **Problem:** Completely isolated from desktop. Can't call a VST from a WAM, can't run a WAM in a DAW.
- **Opportunity:** Untouched bridge between web and desktop audio.

### Existing Solutions & Gaps

**Live Performance Hosts (Gig Performer, Cantabile, Bloxpander)**
- *Strength:* Scene management, MIDI mapping, live-specific features. Deep feature set for gigging musicians.
- *Weakness:* Platform-specific (Windows/Mac), VST-centric, expensive, closed ecosystems. CLAP support is emerging but not native.

**Modular/Tech Hosts (VCV Rack, Element)**
- *Strength:* VST3 + LV2 support, open-source ethos (Element), extensible architecture.
- *Weakness:* Designed for modular workflows, not traditional plugin chains. Modular paradigm is not familiar to most musicians. Overkill for basic hosting.

**Cross-Platform Attempts**
- *UAPMD:* Multi-format (VST3/AU/LV2/CLAP) + MIDI 2.0. Impressive technically.
- *CrossPlatformVstHost:* Skeleton project, unmaintained.
- *Carla:* LV2 host, jack transport, but aging UI, niche Linux audience.

**The Void:**
- **No single host handles VST2 + VST3 + CLAP + LV2 equally well.** Most pick a subset.
- **No cross-platform host is both simple AND supports modern formats.** Simplicity hasn't been prioritized.
- **No bridge between Web Audio and desktop plugins.** Completely separate worlds.
- **No host assumes "I might move this live rig between my Mac, my laptop on Windows, and the cloud."** Portability is an afterthought.

---

## Who Suffers: The Real People

### Live Musicians / Gigging Keyboardists
- **Friction:** "I have 15 VST synths for live. On gigs, I need instant switching, foot pedal mapping, zero crashes. Cantabile does this but it's Windows only. Gig Performer works but $400."
- **Deeper problem:** Every plugin host crashes. Every one. Scene switching is always flaky. Why isn't there ONE that just works?
- **Untapped:** Musicians who want to use plugins but can't trust the stability. People who've given up.

### Plugin Developers
- **Friction:** "I maintain a VST3 version, an LV2 version, an AU version. They're the same code, different wrappers. I spend 50% of my time on this instead of making my plugin sound better."
- **Deeper problem:** Fragmented ecosystem. No "write once, run everywhere" story. CLAP helps but doesn't solve AU, VST2.
- **Signal:** Developers are choosing CLAP precisely because it's simpler to wrap. If we made hosting simpler, they'd choose it too.

### Home Studio Producers
- **Friction:** "I use a DAW for recording but want to use plugins without launching Logic/Studio One. Why is every VST host either featureless or bloated?"
- **Deeper problem:** No middle ground. Either you get a toy (bare VST host) or a studio (full DAW).

### Web Audio Developers
- **Friction:** "I built an amazing plugin in Web Audio. Can't use it in Ableton. Can't talk to VST plugins. I'm stuck in the browser."
- **Deeper problem:** Web audio is powerful but isolated. Desktop dominates music production. The two worlds don't touch.

### Live Streamers / Remote Musicians
- **Friction:** "I need consistent plugin behavior whether I'm on Mac at home, Windows at the studio, or cloud streaming. Impossible. Each host is different."
- **Deeper problem:** Portability across platforms and contexts is not a design goal of any existing host.

---

## The Signal: What Do They Say?

From KVR Audio forums, GitHub issues, Gearspace discussions:

- *"Why is VST hosting so fragile?"* — repeated constantly. Not technical question, emotional one.
- *"I just want CLAP support in Cantabile."* — demand, but Cantabile moves slowly.
- *"Can I run my plugin in the browser?"* — unanswered need.
- *"I want to switch between Mac and Windows. Why do I need different hosts?"* — reasonable ask that no one solves.
- *"Plugin A broke my whole live rig."* — isolation/sandboxing would help. Exists nowhere.

---

## What's Obvious Opportunity vs What Wants To Be Born?

**Obvious: "Build a host that supports CLAP + VST3 + LV2"**
- Yes, needed. But it's a feature list, not a vision.

**What wants to be born:**
- A host so **reliable and invisible** that musicians trust it with their career.
- A host **platform-agnostic** — one rig, multiple machines, cloud-ready.
- A host **plugin-developer-friendly** — prove once that your plugin works here, deploy everywhere.
- A **bridge** between web audio and desktop, not two separate universes.

**The spark:** What if the same host worked on your Mac live rig, your Windows studio, your browser session, your iPad? Not as a gimmick but as the default assumption?

---

# 2. PROJECT — ONE JOB, WHAT MUST IT DO PERFECTLY?

## The Timeless Form: What Survives the Fire?

If this could only do ONE thing, what is it?

**Not:** "Host plugins from 5 formats."
**Yes:** "Run any plugin, anywhere, reliably."

*One metric: Could a musician build their entire live rig here and never worry about crashes, compatibility, or platform switching?*

---

## The Theoretical Ideal

Strip away every feature, constraint, and compromise.

What would a plugin host look like if:
- Format differences were invisible (you load a plugin, it works, you don't know if it's CLAP or VST3).
- Platform differences were invisible (same project, Mac/Windows/Linux, no changes).
- Dependencies were invisible (user never thinks about MIDI routing, CPU, plugin communication).
- Failures were impossible (crashed plugin doesn't crash the host, bad parameter doesn't break the chain).

**The Platonic ideal:** A host so simple that its existence is barely noticeable. You load a plugin. It sounds. You move the rig. It still sounds.

---

## The Theoretical Minimum

Smallest thing that solves the core problem?

**Single plugin loading.** That's it.
- Can you load ONE VST/CLAP/LV2/AU and route audio to it?
- Can you load your synth, plug in a keyboard, hear sound?
- Can you save and restore that state?

Everything else is ornament. Prove THIS works before adding layers.

---

## Single-Player Value

Before network effects, features, ecosystem — what does one person get alone?

A musician, alone, with a CLAP synth and a CLAP effect:
- Load both.
- Route: Keyboard → Synth → Effect → Audio Out.
- Save the project.
- Come back tomorrow, sound is identical.
- Move the file to another machine. Sound is identical.

**That's victory.** Everything else is extension.

---

## Boundary: What Must It NOT Become?

**Not a DAW.** No sequencer, timeline, mixing console, automation curves.
**Not a plugin development environment.** No SDK, no debugging tools.
**Not a format converter.** Not "save as VST" or "compile CLAP."
**Not a synth engine.** No built-in instruments.
**Not a server.** Not "host plugins in the cloud and stream audio."
**Not a plugin marketplace.** Not discovery, rating, downloading.

If users ask for these, the honest answer is: "No. Use a DAW for that. We do one thing."

---

## The Essence: What Is The Spark?

Where is the magic hiding?

**In portability.** One project file (`.host`) loads identically on Mac/Windows/Linux/Web. That's the moat. That's what no one else does.

**In simplicity.** The host disappears. No menus, no routing nightmares, no plugin communication debugging. Load → Sound. Close → Same.

**In trust.** A crashed plugin doesn't crash the host. A bad parameter doesn't break the chain. Error recovery is invisible.

**In standards.** Not "our format," but full support for CLAP/VST3/LV2/AU standards—not partial, not "experimental," but complete.

---

## The Spine: Happy Path

What do 90% of users actually do?

1. **Load a chain:** Keyboard synth → 2-3 effects → Output.
2. **Play and tweak:** Adjust parameters, save presets.
3. **Save and reload:** Project loads identically next time.
4. **Transport:** Same project to another machine/context, works unchanged.

All other features exist to support this path. Anything off-path is extra.

---

## The Price: What Are We Trading?

**Speed vs Completeness:** Ship a simple, complete host NOW — or perfect, incomplete host later? Choose: **Ship now.** Simple wins.

**Breadth vs Depth:** Support all formats partially, or supported formats completely? Choose: **Complete support for chosen formats.** Better to nail CLAP + VST3 than half-support 8 formats.

**Cross-Platform vs Native Polish:** One code base that works everywhere, or native Mac/Windows apps? Choose: **One code base.** Portability is the thesis.

**Closed Ecosystem vs Open Extensibility:** Proprietary host, or open-source? Choose: **Open-source (MIT).** We're not building walls.

---

## What Unlocks Everything?

What decision, if made, makes all others obvious?

**"The host is not the app. The host is the format."**

Meaning: A `.host` project file is the unit of truth. The app is just a reader/editor. You can build Mac apps, CLI tools, web players — all reading the same format.

This unlocks:
- Web playback (WAM plugins can feed a .host file through audiojs Web Audio API)
- CLI rendering (batch process .host files on a render farm)
- Mobile versions (iOS/Android app that reads .host files)
- Plugin developers (test their plugins with our reference host, share rigs as .host files)

**One format. Multiple consumers.**

---

## Does The Name Reveal or Betray?

Current: "audio-host" — Functional, forgettable.

What if it was:
- **Harness:** (We're harnessing plugins, controlling the energy)
- **Rig:** (Music industry term, clear metaphor)
- **Mixer:** (Overclaimed, music term, might confuse with mixing console)
- **Rack:** (Common term, but VCV Rack owns it)
- **Ensemble:** (Poetic, but obscure)
- **Weave:** (Plugins woven together, suggests interconnection)

**Strong option: "Rig"** — A musician says "my live rig," "I'm setting up my rig." It's honest to its audience. It says "this is what working musicians use."

Alternative: Keep "Host." Clear, technical, not trendy.

---

# 3. WHAT CREATES VALUE FOR USERS?

## Value Hypotheses (Test These)

### Hypothesis 1: Cross-Platform Portability
**Claim:** Musicians value "same rig, any machine" above all else.
**Why:** Gig on Mac, studio on Windows, rehearse on Linux. Exhausting to maintain 3 setups.
**How to test:** Ask 10 gigging musicians: "If you could take your live rig from any machine to any other (Windows, Mac, Linux, cloud) without changes, how much is that worth?" Listen to the energy of the answer.

### Hypothesis 2: Reliability >> Features
**Claim:** One crash erases all feature value. Stability matters more than breadth.
**Why:** A live musician who crashes mid-show loses gigs. They'll choose 5 trustworthy plugins over 50 flaky ones.
**How to test:** Ask: "Have you ever been burned by a plugin host crash?" Count the hands. Ask: "Would you trade half the plugins for zero crashes?" Listen.

### Hypothesis 3: Plugin Developers are Users Too
**Claim:** If plugin developers can test their code in one standard host, they'll target CLAP/open formats. Proprietary formats die.
**Why:** Every format a developer must support is a format they won't polish their plugin for.
**How to test:** Ask plugin developers: "If there was one reference host where CLAP plugins were guaranteed to work, would you target CLAP?" gauge interest.

### Hypothesis 4: Web Audio Isolation is Temporary
**Claim:** Soon, musicians will want WAM plugins alongside VST.
**Why:** Web audio is catching up in quality. Streaming musicians will want web plugins. Some will need both.
**How to test:** Ask: "Would you use a web-based plugin in your live rig if it was as good as VST?" Distinguish between "sounds good" and "stable enough for gigs."

### Hypothesis 5: UI Simplicity is a Feature
**Claim:** The simpler the UI, the more we win.
**Why:** Complexity creates crashes. Simplicity attracts musicians tired of menu diving.
**How to test:** Show two mockups: one with all features visible, one bare-bones. Which would each type of user pick? (Liveperson vs Studio Producer vs Developer)

---

## Value Propositions (By Audience)

### For Live Musicians
> *"Your live rig. Any stage, any continent, any OS. One file. Load and play."*
- **Metric:** "I played this same rig on Mac, Windows, and cloud streaming without changes."

### For Plugin Developers
> *"Target one standard, reach everywhere. No format fragmentation. No wrapper hell."*
- **Metric:** "I built CLAP once. It works in web, desktop, mobile, cloud. I moved on."

### For Home Studios / Producers
> *"Plugins without a DAW. Load your synths, effects, presets. No bloat. No learning curve."*
- **Metric:** "I prefer this to my DAW for working with plugins."

### For Web Audio Developers
> *"Bring your plugins to the desktop. No rewrites. No compromise."*
- **Metric:** "My web audio plugin works in a professional live rig."

---

## What Would Make People Say "Whoa"?

The magic moment, the story they tell:

- **A musician:** "I built my live rig on my Mac. Flew to another continent. Opened the file on a Windows laptop. Sound identical. I cried."

- **A plugin dev:** "I wrote CLAP once. It runs on Windows, Mac, Linux, inside a browser, on iPad. One code base. I'm done with format hell."

- **A producer:** "I switched from my DAW's plugin hosting to this. Simpler, faster, more reliable. I get more work done."

- **A live streamer:** "I stream from 3 different machines. Same rig every time. No setup time. I just focus on the music."

---

## Success Criteria (Measurable)

1. **Load any CLAP/VST3/LV2 plugin without configuration.** (Zero-setup principle)
2. **Same project file loads identically on Mac/Windows/Linux.** (Portability)
3. **One plugin crash doesn't crash the host.** (Resilience)
4. **Non-technical musicians can build a rig in < 5 minutes.** (Simplicity)
5. **Project files are human-readable and portable.** (Longevity — format survives 10 years)
6. **Can be used on stage, in studio, in browser without rethinking the rig.** (Universality)

---

## What's The Untapped Opportunity?

Where is competition irrelevant?

**Existing hosts compete on:** Feature count, UI polish, live-specific gimmicks.

**Where competition is irrelevant:** Making the host disappear. Making it so simple and trustworthy that people forget they're using a host. Making it work the same way everywhere.

This is not a saturated market move. Existing hosts are feature-rich, platform-specific, opinionated. The untouched blue ocean is **radical simplicity + universal portability.**

---

# Recommended Direction

## Phase 1: Find The Truth (Do This First)

**Talk to people.** Not "Would you want X?" but "Show me your pain. Show me the host you use and why it breaks your heart."

Target audiences (5-10 conversations each):
1. **Live musicians** (gigging keyboardists, guitarists who use VST)
2. **Plugin developers** (CLAP/VST3 plugin authors)
3. **Home producers** (not professionals, using DAW's plugin host)
4. **Web audio developers** (using audiojs technologies)

Ask:
- What's your current setup?
- What breaks most often?
- What would change if you had a host you trusted completely?
- Would portability (Mac→Windows) matter?
- How often do you think about the host vs. the plugins?

Write up findings. The truth might surprise you.

## Phase 2: Start with ONE Format, Well

Don't try VST3 + LV2 + CLAP + WAM at once.

Choose: **CLAP** (simplest, most modern, least baggage, growing momentum)

Build a CLAP-only host that is:
- Stateless (every interaction leaves zero side effects)
- Portable (one `.host` file, any OS)
- Resilient (one plugin crash doesn't kill the host)
- Transparent (users understand what's happening)

Prove the vision with one format. Then expand.

## Phase 3: Prove Portability

Get the `.host` file format rock-solid. It's the core thesis.

A `.host` file should:
- Be JSON or text (human-readable, diff-able in git)
- Contain: plugin IDs, parameter values, port mappings, presets
- Load identically on any OS
- Not contain absolute paths or OS-specific state

Test: Save on Mac, load on Windows, sound identical. If you can't do this, nothing else matters.

## Phase 4: Bridge Web Audio

Once desktop hosting is solid, add WAM support.

A `.host` file can contain CLAP plugins AND WAM plugins. Mix and match. This is the innovation.

Test: Load a `.host` file with both VST and WAM plugins in a web player. This is the "whoa" moment.

---

# Final Thought: The Irreducible Essence

What is this project's **taste, its light?** (Krishna in BG 7.8)

Not "we built a multi-format host." 

Rather: **We made plugins portable. One rig. Infinite contexts.**

That's the spark. Everything else is ornament.

If you forget every feature and remember only that, you've got the essence.
