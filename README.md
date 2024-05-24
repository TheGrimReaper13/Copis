# COPIS - 0.1
### A simple wired scoring box for saber fencing

The main problems with saber compared to the other weapons is that the whole blade and guard is a conductor.
One issue this leads to can be easily avoided via multiple approaches. The other is the blades touching and affecting each others circuits.
We work around these issues by only having i.e. fencer A's weapon hot and only fencer B's lame reading at that exact moment.
Then we just repeat this with the fencers swapped respectively.
One more issue thoug is unsolved for now. The scoring box will register a hit if fencer A touches their own lame and fencer B touches the weapon of fencer A.

The electrical circuit can not be much simpler. We only use some resistors to pull-down the inputs, isolate the outputs and run the LEDs.

### Resources 

Official FIE material rulebook 2023, specifically Annex B C. SABRE
https://static.fie.org/uploads/32/163442-book%20m%20ang.pdf
