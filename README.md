Copyright 2014, The University of Texas at Austin 
All rights reserved.

THIS FILE IS PART OF THE CONTAINMENT DOMAINS RUNTIME LIBRARY

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met: 

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer. 

2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution. 

3. Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.


CD runtime system framework										Modified date: 09.26.2014


[Introduction]

This repository is Containment Domain (CD) runtime system libraries. This consists of four subdirectories.

1. src : it contains the source code of runtime system.

2. lib : it contains shared objects.

3. doc : it contains the document of runtime system.

4. tools : it contains the tools running with CD runtime system.

5. test : it contains the example codes that is annotated with CD libraris.


[Job allocation of runtime system development]

1. Song - Communication logging

2. Kyushick - Overall runtime system operations

3. Jinsuk - Overall runtime system operations

4. Seong-lyong - Libc logging 


[TODO]

1. (Kyushick) Reporting CDEntry of local space to the centralized data structure of CDEntry (HeadCD)

2. (Seong-lyong) Runtime logging: Isn’t runtime logging (what Rahul started) kind of critical for recovery, even on a single node?

3. (Song) Communication logging 

4. Some decision framework to map CD to storage for preservation. 
This is kind of important to be figured out soon, and currently it is selected by changing Macro (MEMORY=0) to preserving to file. 
This would be involved with error detection mechanism to be applied to current CD or decided statically by auto-tuner before run.

5. BLCR intergration and test for it. We also need some decision framework for the survivality from node failure of a specific CD. 
By the way, do we also sometimes allow in-memory checkpoint in buddy too?

6. Need to come up with error injection framework.
Generate OS Error Signals from separate process? 
(Kyushick: I think what normally people do is just using kill command)

7. (Kyushick) Detaching profiler class to work with runtime.

8. (Kyushick) More interactive visualization of CD runtime system with sight. 
Need to work on the front-end part of Sight to generate viz-related log file that is appropriate to layout engine which is being developed by Hoa in Utah university.



