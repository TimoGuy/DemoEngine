# Solanine Ideapad

## Rendering

- [x] Clouds
- [x] Better atmospheric scattering
  - [x] Fixed the unusual clamping issue and fixes the rsi. Can't go to space yet unfortunately...
- [x] Make a fog system. That's physically accurate
  - [x] It uses this: https://sebh.github.io/publications/egsr2020.pdf
  - [x] Essentially slices of rendered skybox with a different transmittance and luminance value. Interpolate among that.
- [ ] Make the night sky texture be full size (don't render it with the atmospheric scattering. Disable the fog with nighttime too. Also, where the planet would have the "ocean", make it so that the night sky reflects off of that sphere (do an rsi operation and possibly get the reflect normal variable too to sample from the cubemap??? That'd be a good idea I think.))
  - [ ] The stark and scary look of it should be there for sure. RENDER AT FULL RESOLUTION!
  - [x] Make sure the night sky reflects off the planet **_(this'll be like a glass planet that smoothed out eh!)_**
    - [x] The night sky also runs at 300fps btw lol
- [ ] Make the clouds actually render with a render distance value where at a certain radius the density just goes to 1 for the clouds. This should speed up the clouds.
  - [ ] I suppose this is like a fog of war thing. A limit of 15km should be good I think. Maybe even less
- [ ] OK, so there is a big bug in the clouds. If you go down and look upwards, the clouds get super bright. Some kind of INF bug probably with the fog addition to the clouds. Might have to do that as a postprocessing effect? Idk.


##### Low priority

- [ ] Hey Janitor, there's a red 0 in the luminance texture, fyi
- [ ] Fix raymarching issues with edge of Clouds


## Gameplay

- [ ] Scaling up walls with monster transformation
- [ ] A cipher to convert Japanese Kanji radicals to a different language
  - You could try looking at the Japanese app's radical list
  - Or this: https://kanjialive.com/214-traditional-kanji-radicals/
  - However, a kanji like 無 has to be difficult
- [ ] What's below the cloud layer?


## MISC notes

##### VS2022 Stuff
- Use CTRL+\, CTRL+Bkspace to float a window (custom to me)
- Use CTRL+\, CTRL+\ to make a new vsplit
- Use CTRL+SHIFT+ALT+(PgUp/PgDn) to move tabs over to different tab groups
- Use CTRL+TAB to move among the vsplits (bc vs2022 is too dumb to have this functionality. This is the fallback yo)