#### segger rtt

This small library is off per default, but may be used on embedded device to support segger consoles. It is a SEGGER real-time transfer (RTT) which allows real-time communication on targets which support debugger memory accesses while the CPU is running.

In order to turn it on, use `-DUSE_SEGGER_RTT=true`.


- src: https://os.mbed.com/teams/anyThing-Connected/code/SEGGER_RTT/
- version: 7dcd871d726b
- changes: only added the CMakeLists.txt in order to integrate.
- License: 

```
SEGGER RTT Real Time Transfer for embedded targets         
                                                                   
All rights reserved.                                               
                                                                   
SEGGER strongly recommends to not make any changes                 
to or modify the source code of this software in order to stay     
compatible with the RTT protocol and J-Link.                       
                                                                   
Redistribution and use in source and binary forms, with or         
without modification, are permitted provided that the following    
conditions are met:                                                
                                                                   
o Redistributions of source code must retain the above copyright   
  notice, this list of conditions and the following disclaimer.    
                                                                   
o Redistributions in binary form must reproduce the above          
  copyright notice, this list of conditions and the following      
  disclaimer in the documentation and/or other materials provided  
  with the distribution.                                           
                                                                   
o Neither the name of SEGGER Microcontroller GmbH                  
  nor the names of its contributors may be used to endorse or      
  promote products derived from this software without specific     
  prior written permission.                                        
                                                                   
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND             
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,        
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF           
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           
DISCLAIMED. IN NO EVENT SHALL SEGGER Microcontroller BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR           
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;    
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF      
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT          
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE  
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH   
DAMAGE.
```
