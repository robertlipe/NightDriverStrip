
  Reviewer’s Field Guide: The IWYU Refactor


  I realize that seeing "+193 files" in a PR is usually a reason to go for a long walk and never come back. This note is intended to
  explain why this was necessary, why it’s safe, and why some things (like GetEffectManager) had to change.


  1. The Disease: Implicit Include Chaos
  Previously, our headers were "anesthetized" by a lucky include order. Target A would build because Header B happened to include Header
  C, which defined a type needed by Header D.


  In Arduino v3 (GCC 15), this luck ran out. The include order changed just enough that demo would build, but ledstrip would
  spontaneously fail. Our implicit dependencies were literal "build killers."


  2. The Cure: IWYU and Implementation Migration
  To fix this, I followed "Include What You Use" (IWYU) dogma:
   * Implementations live in `.cpp` files. It is simply good taste to have code exist in one place. This separates the definition,
     declaration, and usage trees.
   * Headers are for declarations. If a header only needs a pointer to a struct, we now use a forward declaration (class Foo;) instead of
     dragging in the whole world.
   * `globals.h` Precedence. We now strictly insist that globals.h is the first local include. This is enforced via new audit tools in
     /tools.

  3. The "Why" behind the Renames


  EffectManager() → GetEffectManager()
  This wasn't a stylistic choice; it was a compiler requirement for decoupling.
   1. Name Hiding: In C++, a member function with the same name as a type hides that type. If SystemContainer had a method named
      EffectManager(), we couldn't easily forward-declare the class without the compiler getting confused between the function and the
      type.
   2. Disambiguation: Standardizing on GetEffectManager() allows us to use lean headers with forward declarations, significantly reducing
      the "recompile the world" effect when effect logic changes.


  network.h → nd_network.h
  Arduino v3 introduced a system <Network.h>. Our local network.h was causing "poisonous" collisions. I surrendered and renamed ours to
  nd_network.h to break the circle.


  4. Notable Optimizations
   * `byte_utils.h`: Replaced ByteswapU64 and friends with more traditional versions. The previous versions were reading source memory
     far too many times; the new versions decay to std::memcpy on our LE architecture, which is significantly faster and handles
     unaligned access better.
   * Palette Offloading: Moved large palettes to colordata.cpp to ensure they reside in rodata (Flash) rather than consuming precious RAM
     at runtime.


  5. Completeness and "Sawdust"
   * 53/53 Builds: Every single target in the project builds and runs.
   * Arduino v3 Proof: Added demo_v3, m5plusdemo_v3, and spectrum2_v3 targets. While not yet fully "supported" (partition
     tables/filesystems will change), they prove the core logic is now v3-ready.
   * Fidelity: I used a mix of automation (Perl/Sed/Gemini-CLI) and manual care. I tried to retain every meaningful comment and bit of
     technical "lore" (like Stefan Petrick’s notes). If a comment fell between the floorboards, it was "sawdust" from the heavy
     lifting—not intentional removal.


  6. How to Review
  The vast majority of this PR is Code Motion. If you see a function disappear from a .h and appear in a .cpp, it is the same logic
  you’ve already approved—just living in its proper home.

  ---


  Van dik hout zaagt men planken. We had to saw some thick planks to get here, but the resulting frame is much stronger.


  — robertlipe


