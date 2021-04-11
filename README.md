# Little Bang
## OpenCL many-body gravity simulator with *collision* and *coalescence*
*A work-in-progress*

![Little Bang](/images/LB1-large.gif)

A many-body gravity simulator is a common OpenCL demonstration of how hardware parallelism can grapple better with the O(n^2) nature of an n-body gravitational system.  It's also a wonderful example of how a discrete computational approach achieves results impossible for traditional analysis.  That's great.

Yet most gravity simulators miss what I feel is the most exciting part: a particle cloud coalescing into larger orbiting bodies.  That's what Little Bang provides: masses collide and then they can either recoil, shatter or coalesce together.  Eventually, a cloud of particles gathers into larger bodies.  And if the parameters are just right and you're lucky, you might even get a nice little solar system.

## Features
* Real-time display of hundreds of thousands of particles, or millions at a slower pace
* Recoil, shattering and coalescence obey conservation laws
* Select a variety of initial conditions from the start menu, or alter the code for more elaborate scenarios
* Frame rate and viewport controls
* Save and resume option permits extended runs of larger systems
* Export particle data for Little Bang viewer, which lets you scrub through the whole timeline, play forward and reverse, and produce movies
* Export still frames (which can also be sequences into movies)
* Optional scrolling display of the total kinetic energy of the system
* Optional mini-map of the whole system 

![Little Bang Start Menu](/images/LB-startmenu.gif)


