# Little Bang
## OpenCL many-body gravity simulator with particle *collision* and *coalescence*
*A work-in-progress*

![Little Bang](/images/LB1-large.gif)

A many-body gravity simulator is a common OpenCL demonstration of how hardware parallelism can grapple better with the O(n^2) nature of an n-body gravitational system.  It's also a wonderful example of how a discrete computational physics achieves results impossible for traditional analysis.  That's great.

Yet most gravity simulators miss what I feel is the most exciting part: a particle cloud coalescing into larger orbiting bodies.  That's what Little Bang provides: masses collide and then they can either recoil, shatter or coalesce together.  Eventually, a cloud of particles gathers into larger bodies.  And if the parameters are just right and you're lucky, you might even get a nice little "solar system".

## Key Features
* Real-time display of hundreds of thousands of particles, or millions at a slower pace
* Recoil, shattering and coalescence obey conservation laws
* Select a variety of initial conditions from the start menu, or alter the code for more elaborate scenarios
* Frame rate and viewport controls
* Save and resume option permits extended runs of larger systems
* Export particle data for Little Bang viewer, which lets you scrub through the whole timeline, play forward and reverse, and produce movies
* Export frames as ,png images (which can also be sequenced into movies)
* Optional scrolling display of the total kinetic energy of the system
* Optional mini-map of the whole system

## Little Bang can demonstrate
* The power of contemporary discete computational physics using only retail hardware
* Gravitational slingslots and Keplerian orbits
* How "nebula" of "stars" and "planets" can form from a random distribution of mass exploding outward
* How initial conditions determine whether the sim universe expands indefinitely, "crunches" back together, or comes to an equilibrium point of expansion
* How long it takes for a "star" to acquire "planets" in *stable* orbits.  Most "planets" that are captured either spiral down into the star, collide with something else, or escape due to influence of another body.  Yet some stable systems can be observed, and even more rarely, a "moon."
* Gravitational wobble of "stars" due to their orbiting "planets"
* The cosmological asymmetry paradox.  If you start with a completely uniform distribution of particles, you will get a completely uniform system, rather unlike the "clumps" and "clusters" of the observable universe, which is more readily obtained from a random initial distribution of particles.


The start menu is a good place to begin to play.
![Little Bang Start Menu](/images/LB-startmenu.gif)

## Start menu options
### Initialization parameters
**Max Objects:** The number of particles to start with.  As the sim progresses, the number of objects tends to dramatically reduce over time as smaller particles coalesce.  So the simulation tends to increase in speed after the first several dozen frames.  Yet collisions also bring the possibility of generating *more* particles than the initial value, so this parameter is also an upper limit.

**Max Distance:** Particles which travel very far away are culled beyond this distance.  Far-flung isolated particles tend to bog down the system and don't contribute significantly to the dynamics of particles in the center of the universe.

**Max Frames:** Used in combination with the *Save Viewer Data* option to limit the frames saved to disk for viewing with Little Bang viewer.  Useful if the sim will run for hours unattended and you don't want to fill your hard drive with particle data.

**Max Radius:** Determines the initial spatial distrubtion of the particle cloud.  The denser you start, the more likely you will have early collisions.

**Initial Velocity:** Radially outward.  Too high, and all your particles will disperse (and eventually be culled when the hit the *Max Distance* limit).  Too low, and *crunch!* Everything globs together.  In-between, you'll get interesting dynamics.  The balance depends on the number of particles, their mass, their initial density of distribution and their initial velocity.

**Initial Mass:** Default is 1 for small particles which cannot further subdivide upon collision.  Larger masses bring other possibilities -- see *Assumptions* below for the collision details.

### Make Movie parameters
The in-sim movie option is limited but still useful.  Little Bang Viewer provides more flexibility.

**Make Movie:** If toggled, a .png image of every frame will be exported.  The perspective is whatever your view is at the time the frame is calculated.  Little Bang Viewer, by contrast, allows you to view the same system from a different angle than when it was first calculated.  See *Save Viewer Data* below.

**Max Movie Frames:** Also useful for not filling one's hard drive when left unattended.

### Save Frame Data Options
Frame data refers to the particle data for every particle in the system at a particular moment in time (the frame): their positions, velocities, masses, etc.  As such it completely represents the state of the system.

**Save Viewer Data:** will export frame data for every particle in every frame.  When combined with Little Bang Viewer, this allows you to view the whole timeline of the simulation rapidly, especially when initial frame rates may be slow for large systems.  It also allows you to view the system from any perspective in the Viewer.  

**Save resume data:** only saves frame data for the most recent frame calculated.  Permits resuming your work later.

**Resume last state:** Picks up where you left off if you opted to save resume data previously.

**Display help:** Displays similar information to the above on-screen.

### Start Options
**Start Paused:** Toggles whether the simulation starts paused on its first frame.  See *Keyboard controls* below for how to unpause it.

**Begin!** Starts the simulation, either in a paused or running state, depending on the above.


## Keyboard Controls
### Perspective contols
In the initial view the camera points towards the origin of the system.  Camera controls use a spherical coordinate system.

**Zoom in and out** - **C** to zoom out, **E** to zoom in.  Updated values are displayed in the text console window.
**Rotate theta** - **A** rotates negative 10 degrees, **D** positive 10 degrees.  360-degree rotation is possible.
**Rotate phi** - **Y** rotates positive 10 degrees ("north" on the globe), **H** negative 10 degrees.  Range is limited +180 to -180 degrees.

### Pause and unpause simulation
**Press F** (for "freeze") to pause and unpause sim.  If you opted to start the sim paused, you need to press F to start it going.  "Render OFF" displays in text console output when frozen, and "Render proceeding" when restarted.

**Frame delay** allows you to add a pause between each frame.  Useful when the sim runs so fast you can't see what's going on, but you want to watch it.
**Press P** to add 10 ms frame delay, **Press the letter O** to remove 10 ms frame delay, down to zero.

**Frame display** Little Bang will update the console with its progress.  The following keys tell it how often to report it has calculated a new frame:
**Press 1** to get a message every frame.  Useful with really large system and you wonder if OpenCL has choked or the program crashed.
**Press 2** to get a message every 10 frames.
**Press 3** to get a message every 100 frames.
**Press 4** to get a message every 1000 frames.
**Press zero** for no message.

**Lock view to most massive body** -- Press **L** to toggle "lock to most massive."  Helps greatly to follow a "sun" or "planet" that has emerged in your system.  It may jump around, however, if the most massive body collides with another, splits, and is no longer the most massive body in your system!

### Optional Display Data & Overlays
**Press V** (for *velocity*) to toggle a report to the text console about the particle with the fastest velocity in the sytem.

**Press K** (for *KE*) to toggle a graphical overlay on the bottom of the screen which plots the kinetic energy of the system over time.  It will not be constant, of course, because only the total of kinetic energy and potential energy is conserved.

**Press U** (for *universe*) to toggle a graphical overlay mini-map of the whole "universe" in the upper right-hand corner of the screen.  The display is heat-map style: brighter colors (from red to yellow) mean more particles in that area.

*If you are making a movie from within Little Bang, the graphical overlays will also be present in your movie frames.*  The in-sim movie option simply captures a ,png of the screen every frame.  Saving the binary data and using Little Bang viewer gives more flexibility in this regard.

**Start a movie right now!!** Sometimes you didn't plan to record a sim, but something interesting happens.  **Press M** to toggle capturing images of every frame.

### Save state options
**Press 7** to save frame data for the current frame, making it possible to resume from this point.  Note: this is a one-time only save.

**Press 9** to continually save the last frame rendered to disk.  Only a single frame of particle data is saved, but it is overwritten with the most recent data every step of the simulation.

**Press 8** to load from a previously saved point, *which will abandon your current state.*
These convenience functions have no confirmation dialogue, so be careful.

## Assumptions and Display Conventions
**Bodies are color-coded by mass** to assist comprehension of the scene, since distance and perspective make it hard to identify large bodies when they are all the same color.

* *White* = Mass of less than 10
* *Blue* = Mass between 10 and 100
* *Green* = Mass between 100 and 1000
* *Red* = Mass between 1000 and 10000
* *Yellow* = Mass greater than 10000

**Bodies have uniform density.** A body's mass is the same as its volume.  Consequently its radius relates to its mass by the formula for the volume of a sphere: vol = (4 pi / 3) r^3.  So radius = cuberoot (3 * mass/4 pi).  There may be better options but I'm unsure what.

**Minimum effective distance for gravitational force** is simply the sum of the radius of the two bodies concerned.  To explain: when bodies are advanced to their new positions in a new frame, it is possible that they overlap and thus collide.  But the gravitational attraction between the masses is still in effect, and is calculated as usual, using the distance between the centers of the two bodies prior to handling their collision.  When this distance is very small, the resultant force is huge since `F=k (m1 * m2) / r^2`  I.e., if two bodies' centers land in the same spot (thus r --> 0), the force is nearly infinite.  To prevent the absurd result, the gravitational force kernel caps the minimum effective distance at the sum of the radius of the two bodies: the closest they'd get before colliding in real life.

**How collisions are handled.** There's a lot of room for variation and experimentation here.  The relevant logic occurs in the `collision_routine()` function.  Currently here are the rules:
* Masses smaller than 30 don't shatter, only coalesce
* If a body of much larger mass collides with a smaller one, the smaller coalesces to the larger.  Currently this happens with a mass ratio greater than 5:1, but the ratio could certainly be larger.
* Coalescing bodies are handled as a completely inelastic collision.  The resultant particle's momemtum is a vector sum of the incident momenta.
* Colliding bodies are handled with a two-body, off-center elastic collision equation.  If a mass shatters, it shatters immediately after the collision.
* On collision both the deflected body and shattered parts are advanced one timestep to avoid immediately colliding again on the next frame.  This is fitting since they are merely detected as coincident on the prior frame.

*Other things I've tried* include: 
* including a probability factor in whether masses break.  Works fine, just multiplies cases.
* specifying that bodies with mass greater than X (i.e., > 5000) always coalesce with other bodies.  But this eliminates spectacular collisions of large bodies, which is unrealistic and no fun.  The mass-ratio approach seems more realistic.

*Further ideas* include:
* Basing the shatter-or-stick logic on momentum rather than velocity.  A small fast-moving bullet-like particle should be more able to shatter a larger body.
* Adding some way to represent bodies of different density or elastisticity of collision

**Unactivated Code option:** Constantly generate new particles into the system.  Set the variable `spew_on` to `true` to have a steady source of new particles added to the system.  Set `spewstop` to a frame number to automatically terminate the behavior.

## Known shortcomings
This was my first real project in C++ and OpenCL, so I'm sure it has plenty of flaws.  In particular, I bet the OpenCL computation could be better optimized in several ways.  It may also be more performant to render all the spheres to a texture first and then simply display the result.

## Special thanks 
to Ramon Santamaria, the author of [raylib](https://github.com/raysan5/raylib) and raygui, which made coding this program so much easier!


