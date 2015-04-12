# **Containment Domains Runtime System** #
**Modified date: 04.05.2015**

## **Copyright** ##
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



## **Introduction** ##

Containment Domains (CDs) is a library-level approach dealing with low-overhead resilient and scalable execution (see [http://lph.ece.utexas.edu/public/CDs](http://lph.ece.utexas.edu/public/CDs)). CDs abandon the prevailing one-size-fits-all approach to resilience and instead embrace the diversity of application needs, resilience mechanisms, and the deep hierarchies expected in exascale hardware and software.  CDs give software a means to express resilience concerns intuitively and concisely.  With CDs, software can preserve and restore state in an optimal way within the storage hierarchy  and can efficiently support uncoordinated recovery.  In addition, CDs allow software to tailor error detection, elision (ignoring some errors), and recovery mechanisms to algorithmic and system needs.

[The documentation about CD API](http://lph.ece.utexas.edu/users/CDAPI/) is available online. 


## **Organization** ##

This repository is Containment Domain (CD) runtime system libraries. This consists of four subdirectories.

1. src : it contains the source code of runtime system.

2. lib : it contains shared objects.

3. doc : it contains the document of runtime system.

4. tools : it contains the tools running with CD runtime system.

5. test : it contains the example codes that is annotated with CD libraries.

## **Supported features of CD Runtime** ##
                                                                                Updated at 04. 11. 2015
+---------------------------+-------------------------+-------------+-----+------------+------+------------+
|                           | Functionality           | Single Task | MPI | MPI+OpenMP | PGAS | Task-based |
+---------------------------+-------------------------+-------------+-----+------------+------+------------+
|           |               | via Copy                |      O      |  O  |    N/S     | N/S  |    N/S     |
|           | Preservation  | via Reference           |      O      |  O  |    N/S     | N/S  |    N/S     |
|           |               | via Reference (Remote)  |     N/A     |  O  |    N/S     | N/S  |    N/S     |
|           |               | via Regeneration        |     N/S     | N/S |    N/S     | N/S  |    N/S     |
|           |---------------+-------------------------+-------------+-----+------------+------+------------+
|           | Detection     | CDAssert                |      O      |  O  |    N/S     | N/S  |    N/S     |
|           |               | Hardware-assited        |     N/S     | N/S |    N/S     | N/S  |    N/S     |
|           |               | Comm-related            |     N/A     | N/S |    N/S     | N/S  |    N/S     |
| Strict CD |---------------+-------------------------+-------------+-----+------------+------+------------+
|           | Recovery      | Reexecution             |      O      |  O  |    N/S     | N/S  |    N/S     |
|           |               | Restart from Checkpoint |     N/S     | N/S |    N/S     | N/S  |    N/S     |
|           |---------------+-------------------------+-------------+-----+------------+------+------------+
|           | IO            | Local File System       |      O      |  O  |    N/S     | N/S  |    N/S     |
|           |               | Parallel File System    |     N/A     |  O  |    N/S     | N/S  |    N/S     |
|           |---------------+-------------------------+-------------+-----+------------+------+------------+
|           |               | Runtime Logging         |      O      |  O  |    N/S     | N/S  |    N/S     |
+-----------+               +-------------------------+-------------+-----+------------+------+------------+
|           | Logging       |                         |             |     |            |      |            |
| Relaxed CD|               | Communication Logging   |     N/A     |  O  |    N/S     | N/S  |    N/S     |
|           |               |                         |             |     |            |      |            |
+-----------+---------------+-------------------------+-------------+-----+------------+------+------------+
| Error Injection Interface                           |      O      |  O  |    N/S     | N/S  |    N/S     |
+-----------------------------------------------------+-------------+-----+------------+------+------------+
| Auto Tuner for CD runtime system                    |     N/S     | N/S |    N/S     | N/S  |    N/S     |
+-----------------------------------------------------+-------------+-----+------------+------+------------+
                                                                               N/A : Not Available
                                                                               N/S : Not Supported as of now