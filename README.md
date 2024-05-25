# COPIS - 0.1
### A simple DIY wired scoring box for saber fencing based on Arduino

The superficial rules for saber when a valid hit should be detected are very simple. Touch your oponents conductive target areas with your conductive weapon.
In Foil and Epee it is very easy to build a scoring box as the blades themselves don't carry any current. We only need to handle events when the switches on the tips of the blades are depressed. This eliminates several special cases which saber needs to deal with.
We avoid handling some of these specifically with our implementation.

 - sword X in contact with sword Y
 - sword X in contact with lame X
 - sword X in contact with sword Y and lame X
 - sword X in contact with sword Y and lame Y
 - sword X in contact with sword Y, lame X and lame Y

The main problems with saber compared to the other weapons is that the whole blade and guard is a conductor.
One issue this leads to can be easily avoided via multiple approaches. The other is the blades touching and affecting each others circuits.
Our implementation solves these issues by only having i.e. fencer A's weapon hot and only fencer B's lame reading at that moment.
Then we just repeat this with the fencers swapped respectively.
One last issue is unsolved for now. The scoring box will register a hit if fencer A touches their own lame and fencer B touches the weapon of fencer A.

The electrical circuit can not be much simpler. We only use some resistors to pull-down the inputs, run the LEDs and some diodes to isolate the outputs.

A line is connected through the lame to an input on our microcontroller, B line is the control and connected to the weapon as well as an input and C is the "power" line connected to the weapon and an output.
### Current State

- behaves as expected on bench
- need to do live test

### Resources 

Official FIE material rulebook 2023 Annex B C. SABRE
https://static.fie.org/uploads/32/163442-book%20m%20ang.pdf
