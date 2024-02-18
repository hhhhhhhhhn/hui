# TODO
## Now
- Add a better way of adding handler, instead of using `parent`
- Change `== UNSET` with `is_unset()`, to check all negative numbers
- Add styling options to button
- Add text input
- Add grid
- Text layout caching
  - TODO: Cache invalidation...

## Later
- Switcher layout

# Done
- Cluster layout
- Separate files
- Interactivity
- Scroller support
  - Add active render texture
    - This can be a stack, which is pushed/popped during the `draw()` functions
    - NO! This can be done with `BeginScisssorMode`
  - Maybe a similar system for interaction? Like an active interaction Rect
