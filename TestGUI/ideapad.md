# Solanine Ideapad

## Rendering

- [x] Clouds
- [x] Better atmospheric scattering
  - [x] Fixed the unusual clamping issue and fixes the rsi. Can't go to space yet unfortunately...
- [ ] Make the night sky texture be full size (don't render it with the atmospheric scattering. Disable the fog with nighttime too. Also, where the planet would have the "ocean", make it so that the night sky reflects off of that sphere (do an rsi operation and possibly get the reflect normal variable too to sample from the cubemap??? That'd be a good idea I think.))
  - [ ] The stark and scary look of it should be there for sure. RENDER AT FULL RESOLUTION!
  - [x] Make sure the night sky reflects off the planet **_(this'll be like a glass planet that smoothed out eh!)_**
    - [x] The night sky also runs at 300fps btw lol


##### Low priority

- [ ] Fix raymarching issues with edge of Clouds
- [ ] There's a red 0 in the luminance texture, fyi


## Gameplay

- [ ] Scaling up walls with monster transformation
- [ ] A cipher to convert Japanese Kanji radicals to a different language
  - You could try looking at the Japanese app's radical list
  - Or this: https://kanjialive.com/214-traditional-kanji-radicals/
  - However, a kanji like 無 has to be difficult
- [ ] What's below the cloud layer?
