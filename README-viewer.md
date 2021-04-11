# Little Bang Viewer notes
## A data viewer and movie maker for [Little Bang](/README.md)

* Allows you to view your simulation data from any angle.  
* Plays back larger systems at a more constant frame rate since no calculation time between frames is involved.
* You can output frames of your choice to .png files, which can then be sequenced into a movie -- this gives the smoothest display of large systems

![Viewer Screenshot](/images/bounce.gif)
(*Above: Orbiting green body takes a hard knock at frame ,and gets absorbed into red one.  The other green body slingshots around just afterwards.)

## Instructions for use
* Create a directory named `fdata` for the frame data exported by LittleBang.  
* Copy the frame data here.  Sequentially numbered files of the form fd000000.bin 
* Run the program

## Sidebar Controls
*From top to bottom:*

**Top button** simply displays the frame data file name currently displayed.  Quite useful to pare down a set of files to the portion you wish to retain.

**Renderer pause** toggles playback

**Forward** and **Backward** radio buttons which determine the direction of playback.  Useful to pause and rewind.

**Framerate delay** slider bar: introduces a delay of so many milliseconds between frames.  Basically slow motion for when the action is fast.

**Enlarge small planets** increases the size of particles with mass less than 10 so they are more visible.  Helpful for seeing more distance small masses.  Since this program is only a viewer, no frame data is changed.  Particles are simply rendered larger, which may cause them to appear to intersect each other.

**Lock Most Massive:** same as in Little Bang.  The camera will remain focused on the most massive particle in the system.

**Draw All Objects:** a toggle.  As currently programmed, if off, the renderer will omit rendering particles of mass 1.  This can significantly speed up draw times in large systems that begin with 100s of thousands of small particles.

## Bottom bar
Allows you to jump to any moment in the sequence of frame data you have loaded

## Keyboard Controls
### Perspective contols
In the initial view the camera points towards the origin of the system.  Camera controls use a spherical coordinate system.

**Zoom in and out** - **C** to zoom out, **E** to zoom in.  Updated values are displayed in the text console window.
**Rotate theta** - **A** rotates negative 10 degrees, **D** positive 10 degrees.  360-degree rotation is possible.
**Rotate phi** - **Y** rotates positive 10 degrees ("north" on the globe), **H** negative 10 degrees.  Range is limited +180 to -180 degrees.

### Pause and unpause simulation
**Press F** (for "freeze") to pause and unpause sim.  If you opted to start the sim paused, you need to press F to start it going.  "Render OFF" displays in text console output when frozen, and "Render proceeding" when restarted.

**Press G** to hide the GUI overlay, for example, when making movies

**Press M** toggles whether to output a .png file for every frame displayed, which can be sequenced into a movie

**O** and **P** change playback direction to backward and foreward.
**K** and **L** subtract or add delay between frames in 10ms increments
