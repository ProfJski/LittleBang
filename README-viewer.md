# Little Bang Viewer notes
## A data viewer and movie maker for [Little Bang](/README.md)

![Viewer Screenshot](/images/bounce.gif)

## Instructions for use
* Create a directory named `fdata` for the frame data exported by LittleBang.  
* Copy the frame data here.  Sequentially numbered files of the form fd000000.bin 
* Run the program

## Sidebar Controls
*From top to bottom:*
**Top button** simply displays the frame data file name currently displayed.  Quite useful to pare down a set of files to the portion you wish to retain.
**Renderer pause** toggles the player
**Forward** and **Backward** radio buttons which determine the direction of playback.  Useful to pause and rewind.
**Framerate delay** slider bar: introduces a delay of so many milliseconds between frames.  Basically slow motion for when the action is fast.
**Enlarge small planets** increases the size of particles with mass less than 10 so they are more visible.  Helpful for seeing more distance small masses.  
Since this program is only a viewer, no frame data is changed.  Particles are simply rendered larger, which may cause them to appear to intersect each other.
**Lock Most Massive:** same as in Little Bang.  The camera will remain focused on the most massive particle in the system.
**Draw All Objects:** a toggle.  As currently programmed, if off, the renderer will omit rendering particles of mass 1.  This can significantly speed up draw times in large systems that begin with 100s of thousands of small particles.

## Bottom bar
Allows you to jump to any moment in the sequence of frame data you have loaded

## Keyboard controls
