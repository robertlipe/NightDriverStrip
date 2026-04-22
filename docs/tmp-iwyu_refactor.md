I totally get that dropping a 120+ file PR on you is rude. Believe me when I say it wasn't a source of joy from this end of the stick, either.
Like a tetanus shot, this will hurt for a little while (sorry!) but it's good for you, me, and the humanity of this code base in the long run.
Inflammation should be minor and short-lived. Ultimately, it's needed to survive long enough for the code base to live.

Diseases cured:
1) We simply had too much code in headers that depended up other code in headers. When the include order changes, and it absolutely does in
Arduino 3 (TBF: it has NEVER been strictly defined) we ran afoul of literally hundreds of cases where target A starts an include chain of A->B->C->D
where target B triggers A->D-C-B, but D depends upon things defined (or declared) in B but _sometimes_ the include of C anesthetized the build to
it so it was OK. But then Arduino 3/ GCC 15 rearranged the undefined behaviour just enough for demo to build, but ledstrip to spontaneously 
burst into flames. Our implicit ordering and dependencies were killers.
[ TODO: List more here ] 

This PR:
* Attempts to enforce IWYU (Include What You Use) dogma without being absolutely annoying. 
  ** If a header needs an include, it includes it.
  ** If a header only needs a decl and not a defn, it tries that. (e.g. struct foo* can be a forward decl; it doesn't _need_ to know everythings about foo;
     only that it's a pointer in "near" memory.
* Get code out of headers.
  ** It's just good taste to have C++ code exist in ONE place. (Sorry.)
  ** This separates definition, declaration, and usage trees much more clearly.
  ** This encourages us to be more honest about call trees. e.g. random_range() is a perfectly lovely template. It's also used in nine 
     source files, not EVERYTHING. Put it in random_utils.h and let the eight places that need it include it.
* Generally tightens up rules.
  ** For anything that might #if something or that may include a "local" header (as opposed to a <system> header), it INSISTS that 
    globals.h is included FIRST. This is "enforced" (militant enforcement TBD) via:
    *** tools/audit_globals_order.py (e.g. global includes are ALPHABETIC) 
    *** tools/audit_include_rules.py (e.g. "globals.h", THEN <sytstems>, THEN "locals")
    *** tools/check_syntax.py (quick syntax checks; faster than compilation for tooling)
    ... except when it's not. :-/ It's not perfect; it's better than I found it.

Completeness:
Every target builds and, to my knowledge, runs. 
  ** Demo, Mesmerizer, yulc-hex, tinyled-demo, and more.
  ** (wait for it...) two NEW targets are added, m5plusdemo_v3, spectrum2_v3, demo_v3 - these are TOTALLY bludgeoned in and are not "supported", 
     but they DO run! What makes them special? They raise the bar to prove that all future commits at least BUILD on Arduino V3. (I have LEDs
     permanently mounted in my home now on ESP32-C6. C6 _requires_ ESP-IDfv5 and hence, Arduino V3. My lights work. (I'm not pretending they're
     building and running on main. I'm saying that CAN be built and run...We're THAT close!)
  ** Hexagon builds and runs like it has so far. I _have_ "real" hexagon gfx support in the oven (still). This PR is NOT that.
  ** I tried like everything to retain all meaningful comments, whitespace, etc. The reality is that I relied on a LOT of automation, ranging
     from perl/sed/awk to CLion to Gemini-cli to Antigravity to pull this all together at scale. If I lost a comment or screwed up a table,
     it's (proably) not because I hated it, but rather that it just fell between the floorboards. I'm not quite saying "Van dik hout zaagt men planken." 
     but I focused on resulting opcodes, not source fidelity. Some sawdust fell from the thick planks.



Where I (no-so) "accidentally" introduced code changes.

I always disliked ByteswapU64() and friends. I stared at the disassembly and realized WHY I hated them; it read the source WAY too many times.
byte_utils.h contains more traditional versions and the generated code is better for both LX6 and RISC-V. In the case that actually matters (These
boards are always LE, any host running ND except maybe Dave's VAX is LE - these thus decay to 'std::memcpy' to allow for unaligned accesses.) That's
even better!

EffectManager()->GetEffectManager()-style changes:

  The core of this PR is decoupling. To improve build times and break circular dependencies, we moved implementations to .cpp files and
  replaced heavy #include statements with forward declarations (e.g., class EffectManager;).

  This created a specific C++ conflict in the SystemContainer:

   1. Name Hiding: In C++, if a member function has the exact same name as a type (the class), the function name "hides" the type name
      within that scope. When SystemContainer had a method named EffectManager(), the compiler would often resolve the word EffectManager
      to the function rather than the class type. (I can't say this is a rule of C++ that I remembered. I had to "discover" it.)
   2. Forward Declaration Failure: This problem becomes acute when using forward declarations. If the compiler hasn't seen the full
      definition of the class yet, it is much less "forgiving" about the name collision. In many contexts (like trailing return types or
      template parameters), the compiler would throw an error because it couldn't tell if you were referring to the class or the
      accessor.
   3. The "Setup/Has/Get" Pattern: To resolve this, we standardized the SystemContainer on a clear accessor pattern. By renaming the
      getter to GetEffectManager(), we:
       * Disambiguated the namespace: The type is EffectManager, and the accessor is GetEffectManager(). No more collisions.
       * Enabled Lean Headers: This allowed SystemContainer.h to use a simple forward declaration for the manager, significantly reducing
         the "recompile the world" effect whenever a small change was made to the effect logic.
       * Improved API Readability: It brought the class into alignment with the rest of the container's logic (GetDeviceConfig,
         GetDisplay, etc.), making the singleton management predictable.

  In short: The rename wasn't for style; it was the "key" that allowed us to stop including effectmanager.h in every single file in the
  project. Without it, the IWYU decoupling would have been physically impossible due to the name-hiding rules of the language.


PatternFoo.h / #pragma once

  90+% of these are overkill. They're included in exactly one place. Except the ones that aren't It's harmless to just make the rule
  that "thou shalt #pragma once headers" and be consistent rather than starting to make excuses for "well, uhm, except for those
  included by effects.cpp. Oh, but some of those, like fireeffect, are included in other places anyway, so there are exceptions to
  the exception."  #pragma once them all. Let the preprocessor sort it out.



effectsupport.h
  Big parts of that moved to colordata, to go into const rodata from flash. Palettes just don't vary at runtime.

formatsize.h
  A good example of something used in once place (probably should be used in 2-3...) that just shouldn't BE in globals.h

gfxbase.h
  Too much code in a .h If the call overhead of a setPixel() is a problem (it's surely not) we'll revisit that. calls aren't free
  as they distrub register allocation, branch predication, and cache, but they're just not THAT bad at our scale.

hashing.h
  We can throw ESP8266 F() cruft overboard. We've never run (nor is this code likely to ever run_ on an ESP8266.

interfaces.h
  Formal line in the sand for things like PSRAM allocation and JSON serialization.

ledbuffer.h
  An example of a source file that seems like it SHOULD be benign that was at the epicenter of like two dozen target-specific
  build issues.
  (Or wat that ledstripeffect?)

network.h
  * I fought having a network.h when there is a Network in the Arduino headers for months. I ultimately surrendered and renamed
  ours to "nd_network.h" to break the circle.
  * Including socketserver.h here surprisingly poisonous.
  * Breaks the dependency upon <Network.h> and this file.h

random_utils, range_utils, time_utils
  * examples of new headers for "old" function bodies

screen.h
  * breaks hardware out into three specific NEW files. (Which, honestly, I don't have the hardware to test...)

soundanalyzer.h
  * I swear that I've been pulling [ID]RAM_ATTR off of decls and into defns for months now. This is More of that.

systemcontainer.h
  * This file ends up in everything. I'm not thrilled with that, but this is better, IMO.

types.h
  * Improves focus.

Actual source code (this was just the warmup to get here! :-)

audio.cpp
  * Will have minimal merge conflicts with my upcoming audio/Arduino3 work.

colordata.cpp
  * Now holds most pallettes, too.

effectfactories.cpp
  * New bucket to create, add, destroy effects.

effectsupport.cpp
  * I should remember why these aren't in colordata, but at this hour, I don't (TODO? FIXME?)

gfxbase.cpp
  * Now holds much more of the central graphics stuff from a dozen headers.

jsonserializer.cpp
  * Manages the border skirmis of ArduinoJSON and internal data structures.

ledbuffer.cpp, ledstripeffect.cpp, ledviewer,cpp
  * adopts a lot of stuff from various headers.

main.cpp
  * rely less on inherited includes.
  * Don't spend clocks on things that are unconfigured.
  * Prep for Arduino3 approach to onboard 3-color LEDs.
  * still more dumb warning fixes.

network.cpp
  * inherit things that used to be in network.h, but that werent' needed everywhere.

screen.cpp, screenimpl.cpp
  * try to manage the three types of screen we wrangle in a consitent way.

socketserver.cpp
  * Listens on 49152 for LEDs[].A

str_sprintf.cpp
  * this function should probably go away, but at the very least, not be in globals.h

systemcontainer.cpp, taskmanager.cpp, types.cpp
  * centralize what was in many headers

ws281gfx.cpp
  * Holds water until we can adopt new FastLED Channels API. Coming Soon. (Not in this PR!)


   
Result:
  * Every target builds (!!)
  ** Notably, this includes the three builds that build on Arduino3, though they aren't yet supported.
     (They WILL break when I change partition taables and filesystem types...)
  * Every target that I can try runs.
  ** Despite this change LOOKING frightening, it's mostly just code motion and a LOT of it was automated..


