# Solanine Ideapad

## Engine Tech

### Music and SFX

- [ ] Implement FMOD into the game






### Migration to Vulkan to use DLSS 2.4 and FSR 2.0

- [ ] Velocity Buffer is needed too yo






### Rendering


- [ ] Clouds Optimizations
  - [ ] There could be instead of worley noise, a pre-combined distance from the edge so that we could do classic raymarching where it takes in a signed distance algorithm
    - [ ] We'd really have to experiment how adding the detail noise would affect that however, and also having blocking objects for this.



- [x] Clouds
- [x] Better atmospheric scattering
  - [x] Fixed the unusual clamping issue and fixes the rsi. Can't go to space yet unfortunately...



- [x] Make a fog system. That's physically accurate
  - [x] It uses this: https://sebh.github.io/publications/egsr2020.pdf
  - [x] Essentially slices of rendered skybox with a different transmittance and luminance value. Interpolate among that.
- [x] Separate the sun and the night sky into their own render step. That will be full size, so the night sky can be full size like in the nested task here:
  - [x] Make the night sky texture be full size (don't render it with the atmospheric scattering. Disable the fog with nighttime too. Also, where the planet would have the "ocean", make it so that the night sky reflects off of that sphere (do an rsi operation and possibly get the reflect normal variable too to sample from the cubemap??? That'd be a good idea I think.))
    - [x] The stark and scary look of it should be there for sure. RENDER AT FULL RESOLUTION!
    - [x] Make sure the night sky reflects off the planet **_(this'll be like a glass planet that smoothed out eh!)_**
      - [x] The night sky also runs at 300fps btw lol
- [x] ~~Make the clouds actually render with a render distance value where at a certain radius the density just goes to 1 for the clouds. This should speed up the clouds.~~
  - [x] ~~I suppose this is like a fog of war thing. A limit of 15km should be good I think. Maybe even less~~
  - [x] So what I think we should do instead is have the clouds do a fixed sample size (just 16 samples or something) at a certain distance. This distance should make it very hard to notice flickering from. ~~And then even then it should have a limited raylength~~ We shouldn't need that bc it's a ray-sphere-intersection now. If the ray entered the sphere, it's guaranteed to stop somewhere.
  - [x] **So here's what I ended up doing:**
    - [x] I got a smoothstep() func in there that changed from the really precise raymarching I was doing and moved to the chunky raymarching. The distance changed the quality. Since there was a sudden cutoff line where the RSI failed to grab the inner layer (sphere in a sphere thing. Made a secant/chord in the outer sphere instead), I used a chord-length operation to use as the limit for the raylength. Since with values that are easier on the gpu make noise, I tried some TAA. Turns out that I'd need some kind of temporal reprojection, but I don't know how to produce a velocity buffer for something that's raymarched and no vertices. Oh well. So I'm just using higher quality settings and taking the performance hit. It's anywhere from 220-250fps on the rtx 2080 ti.
- [x] OK, so there is a big bug in the clouds. If you go down and look upwards, the clouds get super bright. Some kind of INF bug probably with the fog addition to the clouds. Might have to do that as a postprocessing effect? Idk.
  - [x] Fixed with the correct atmosphere transmittance formula


- **NOTE: DO NOT USE TAA, STAY WITH FXAA AND THEN WHEN FSR2.0 COMES OUT, PORT THIS SUCKER TO VULKAN AND USE ITS TAA**




#### Low priority

- [x] Hey Janitor, there's a red 0 in the luminance texture, fyi
- [ ] Fix raymarching issues with edge of Clouds
  - [ ] This article's cloud rendering was interesting: https://blog.thomaspoulet.fr/dcs-frame/
  - [ ] It's less of an issue with FXAA enabled. Hah. Up to you, future Timo, but I'm not fixing it.
  - [x] Okay, so I think the possible cause for this was because the depth-sliced atmospheric LUT got cut off early and so the fringes of the atmosphere just never got included.
    - [x] Oh yeah, I fixed it btw. (Up to a certain distance lol)





### Performance

- [x] Get there to be a rendering step before any pbr related rendering happens where the irradiance and prefilter textures are rendered.
  - NOTE: here's how the performance stacks up:
    - *BEFORE*: Screen space (1920x1080): 2073600 pixel lookups and mix() operations
    - *AFTER*: 6 * (128^2 + 64^2 + 32^2 + 16^2 + 8^2 + 32^2) = 137088 pixels
      - AFTER is significantly fewer pixels than 1080p. I'd say it's worth it, bc it's 7% of the pixels as 1080p.
      - NOTE: It does not make a difference if <7% of the screen has pixels in it eh!



- [x] Clouds using signed distance fields
  - [x] The clouds system steps thru the volume at a logarithmic rate, however, this doesn't help performance bc of the non-constant stepping as well as the kinda weird system of 
  - [x] Results (NOTE: these are while recroding on obs):
    - BEFORE: 160fps (6.2~ms per frame)
    - AFTER: 40-160fps (very bad. If looking at the horizon, very bad)
    - [x] I want you to try: going back to the accumulation of transmittance algorithm and see how that looks.
      - It may be that I gotta do this and apply this to all opaque objects and just do the transparent objects as their own post-processing effect.
      - Okay, it's not great. I got 140fps or so. Go back to the before. That way's the best.
  - [x] reverted to 291ef1e89d445971b1df49677a2f6a520ffa35d4 version (old version that ran at 160fps)


- [ ] The idea is to get a similar performance metric to a AAA game in 1080p and 4k.
  - [ ] 2022-04-27 findings:
    - [ ] 4k current benchmark: avg. 53fps (release)
    - [ ] 1440p current benchmark: avg. 134fps (release)
    - [ ] 1080p current benchmark: avg. 230fps (release)
    - [ ] NOTE: these results are similar to an rtx 2080 ti on Shadow of the Tomb Raider (4k).
      - [ ] Well, my numbers are a little lower, bc Shadow of the Tomb Raider is more like avg. 58fps





### Things that could improve

- [ ] ~~Cloud rendering (duh...)~~
  - [ ] ~~If you were okay with a cloudscape that never changed density, you could render out a distance function for the clouds~~
    - NOTE: turns out that raymarching with the signed distance field idea was not that great. It ran super slow

- [ ] Shadows
  - [ ] All the static objects could be the only ones with shadow and then all the dynamic objects could just have a shadow for a certain distance.
    - [ ] That way can minimize (ofc opening doors would have to be a part of the dynamic shadow map) the shadows being rendered for the main light.

- [ ] Forward rendering better
  - [ ] NOTE: I don't really know what the bottleneck is honestly.
  - [ ] Could look into how to do indirect rendering? Or instanced rendering.
  - [ ] Could render the z-prepass front-to-back instead of random
  - [ ] Could make this be a clustered forward shading system.

























-----------

## Gameplay

### Physics-based combat system

NOTE: below are some super secret moves that really you're only supposed to learn inside of special shrines. Or if you look up spoilers eh.

- [ ] Doing the spin attack
  - [ ] See this vid: https://www.youtube.com/watch?v=X6X3-AiAN5k
    - [ ] @5:45 (NOTE: hammer throw, the ball is 16lbs, and the cable is <=1.22m)
  - [x] So having a `spin` value and a `lean` value would be super good.
    - [x] Spin: the y axis angular momentum
  - [ ] So doing the helicopter would be great, yes, however, pressing X while spinning to *throw* the bottle away would be great too
    - [ ] Meant to be in line with "willing yourself to the bottle" bc it's supposed to will itself into your hands or onto your body

- [ ] Bang bang vertical slice
  - [ ] Charge a vertical over-the-head slice (tilt L stick and hold RT), then right after you press X to launch the attack, press A to get your feet off the ground so you can detach yourself/launch yourself off of what you hit.
  - [ ] So basically, whatever you hit can propel you in the direction you hit it. (NOTE: really this is for hard materials or bouncy materials only. NOTE: This is only if you're a light class transformation.)
    - [ ] If you do a vertical swing into the ground, it can propel you upwards like a larger jump (esp. with heavy swing and bouncy floor), or if you're doing a midair frontflip swing, it can propel you forward.
    - [ ] Same is true with the horizontal counterpart. Depending on the distance from the wall you are, you will shoot out perpendicular from the wall or shoot out parallel to the wall.

- [ ] Light and Heavy modes
  - [ ] A light transformation (Human, Slime) could hit hard, sturdy objects and launch self to somewhere!
  - [ ] Heavy transformations (Monster, Lava) could hit the same hard, sturdy object and send them flying, simply bc of the weight difference.
  - [ ] EXAMPLES
    - [ ] So with the light transformations, something like a helicopter would be effective, but with the heavy transformations, you'd wanna go in the reverse spinny spinny direction to do a drill/ground-pound (reverse of the helicopter bc it pushed you downwards). With the added weight, the pound onto the ground is greater.




### Transformations?

- [ ] Scaling up walls with monster transformation
- [ ] A cipher to convert Japanese Kanji radicals to a different language
  - You could try looking at the Japanese app's radical list
  - Or this: https://kanjialive.com/214-traditional-kanji-radicals/
  - However, a kanji like 無 has to be difficult
  - For every screen/display/crt monitor in the world, have the old language (Japanese) have screen burn-in in the spot when the display turns off or a really bright light shines onto the display at certain angles (at least that's how I think burn-in works... I really don't know though).
  - Old people (all slime ppl too) have names that are of the old language. When someone recognizes you as Hikaru, this is very apparent.
- [ ] What's below the cloud layer?
  - [ ] Glass. A world heated so much and cured of so much impurity that it's a perfect sphere that reflects the sky. ~~A world burnt so bad that there's glass underneath and it's a perfect sphere that reflects the sky. This is true with the night sky if you look closely. You'll see this much better when the cloud layer is removed.~~











---------

## Story













---------

## 










---------

## MISC notes

### VS2022 Keyboard shortcuts
- Use CTRL+\, CTRL+Bkspace to float a window (custom to me)
- Use CTRL+\, CTRL+\ to make a new vsplit (custom to me)
- Use CTRL+SHIFT+ALT+(PgUp/PgDn) to move tabs over to different tab groups (custom to me)
- Use CTRL+TAB to move among the vsplits (bc vs2022 is too dumb to have this functionality. This is the fallback yo)
