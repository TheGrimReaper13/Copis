# COPIS - 0.5
### A simple DIY wired scoring box for saber fencing based on Arduino

The superficial rules for saber when a valid hit should be detected are very simple. Touch your oponents conductive target areas with your conductive weapon.
In Foil and Epee it is very easy to build a scoring box as the blades themselves don't carry any current. We only need to handle events when the switches on the tips of the blades are depressed. This eliminates several special cases which saber needs to deal with.
We avoid handling some of these specifically with our implementation.

The main problems with saber compared to the other weapons is that the whole blade and guard is a conductor.
One issue this leads to can be easily avoided via multiple approaches. The other is the blades touching and affecting each others circuits.
Our implementation solves these issues by only having i.e. fencer A's weapon hot and only fencer B's lame reading at that moment.
Then we just repeat this with the fencers swapped respectively.

The electrical circuit can not be much simpler. We only use some resistors to pull-down the inputs, run the LEDs and some diodes to isolate the outputs.

A line is connected through the lame to an input on our microcontroller, B line is the control and connected to the weapon as well as an input and C is the "power" line connected to the weapon and an output.

### Current State and To-do

- behaves as expected on bench (whip-over fully implemented but not tested)
- it's not going to be be trivial testing the whip-over
- second live test went very well, apart from some very rare false positives that I should be able to iron out
- self_hit leds lit up continuously during live test but not on bench, this bug didn't seem to affect behaviour, 

### Resources 

Official FIE material rulebook 2023 Annex B C. SABRE
https://static.fie.org/uploads/32/163442-book%20m%20ang.pdf

### Special Thanks

I want to thank github user Solexis2001 for his comment years ago on another persons project which got me on the right path with the idea to alternate which weapon is hot so we can handle each fencer seperately.