# Design
hui is a 4-pass immediate-mode GUI library, written in C99.
The passes are:
1. Creating the element tree and handling events (e.g. responding to a button click).
   This is the code the hui user writes.
2. Layout pass
3. Input handling pass.
4. Rendering pass.

## Layouts
Consists of an x and y position, alongside the width and height.

Rules:
- It is the parent's responsibility to call the `compute_layout` function of its
  children.
- Parents *always* set the position (x and y).
  - Elements should not set their own position, or depend on it,
    as parents may set them after the layout is done.
- A child may never change the parent's layout, but it can read it and use it.
  - If a child requires a property of the parent that hasn't been set yet,
    then it should return ASK_PARENT.
- Parents should always set known properties of themselves and their children as
  soon as possible, and can set temporary values which are then changed if needed.
  - For example, the cluster layout starts by setting itself a temporary
    width and height based on it's parent, then computes the actual width and height based on the
    children, and then sets it to the actual width and height.
- The `compute_layout` may be called multiple times, and must be able do discern
  when it is necessary to recompute.
  - For example, if an element is too large for the cluster layout,
    it sets its width to the cluster's width and the height to `UNSET`,
    and changes the x and y to be on the next line, and calls `compute_layout` again.
    - This means the children's `compute_layout` must *always* recompute the x and y
      position of the grandchildren and call `compute_layout` on them again (for the grandgrandchildren),
      as there is no way of knowing if the x or y position were changed.
- Even if a children's layout is know, the parent should still call `compute_layout`,
  as it may have children to layout.
