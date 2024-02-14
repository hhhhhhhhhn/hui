# TODO
## Now
- Add a better way of adding handler, instead of using `parent`
- Scroller support
  - Add active render texture
    - This can be a stack, which is pushed/popped during the `draw()` functions
    - NO! This can be done with `BeginScisssorMode`
  - Maybe a similar system for interaction? Like an active interaction Rect
- Change `== UNSET` with `is_unset()`, to check all negative numbers

## Later
- Switcher layout
- Text layout caching
  - Between layout and draw
    - Where newlines and wraps are
      - Probably a null terminated index array, allocated in the frame local arena
        - But how would you store that between frames?
  - Between frames
    - Hashmap of (text, width) -> (newline indexes, height)?

# Done
- Cluster layout
- Separate files
- Interactivity
