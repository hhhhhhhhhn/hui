# TODO
## Now
- Improve text input
- Make border and padding a Vec4
- Deinit text cache on exit
- Add styling options to button
- Add nothing element
- Add grid

## Later
- Switcher layout

# Done
- Add text input
- Add a better way of adding handler, instead of using `parent`
- Cluster layout
- Separate files
- Interactivity
- Scroller support
  - Add active render texture
    - This can be a stack, which is pushed/popped during the `draw()` functions
    - NO! This can be done with `BeginScisssorMode`
  - Maybe a similar system for interaction? Like an active interaction Rect
- Fix inherent text width, wrapping has error on some font sizes
- Change `== UNSET` with `is_unset()`, to check all negative numbers
