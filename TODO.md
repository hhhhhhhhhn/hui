# TODO
- Separate files
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
