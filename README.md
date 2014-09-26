CD runtime system framework                             Modified date: 09.26.2014


[Introduction]

This repository is Containment Domain (CD) runtime system libraries. This consists of four subdirectories.
src : it contains the source code of runtime system.
lib : it contains shared objects.
doc : it contains the document of runtime system.
tools : it contains the tools running with CD runtime system.
test : it contains the example codes that is annotated with CD libraris.


[Job allocation of runtime system development]

Song - Communication logging

Kyushick - Overall runtime system operations

Jinsuk - Overall runtime system operations

Seong-lyong - Libc logging 


[TODO]

1. (Kyushick) Reporting CDEntry of local space to the centralized data structure of CDEntry (HeadCD)

2. (Seong-lyong) Runtime logging: Isn’t runtime logging (what Rahul started) kind of critical for recovery, even on a single node?
Need to come up with error injection framework.
++ Generate OS Error Signals from separate process.?

3. (Song) Communication logging 

4. Some decision framework to map CD to storage for preservation. This is kind of important to be figured out soon, and currently it is selected by changing Macro (MEMORY=0) to preserving to file. This would be involved with error detection mechanism to be applied to current CD or decided statically by auto-tuner before run.

5. BLCR intergration and test for it. We also need some decision framework for the survivality from node failure of a specific CD. 
By the way, do we also sometimes allow in-memory checkpoint in buddy too?

6. Need to come up with error injection framework.
Generate OS Error Signals from separate process? (Kyushick: I think what normally people do is just using kill command)

7. (Kyushick) Detaching profiler class to work with runtime.

8. (Kyushick) More interactive visualization of CD runtime system with sight. Need to work on the front-end part of Sight to generate viz-related log file that is appropriate to layout engine which is being developed by Hoa in Utah university.





