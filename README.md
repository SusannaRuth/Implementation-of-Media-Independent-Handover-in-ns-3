## Implementation of Media Independent Handover (IEEE 802.21) in ns-3 
### Overview:
IEEE 802.21 provides a media-independent framework for seamless handovers between heterogenous networks [1]. The existing implementation of MIH in ns-3 [2] has been ported from ns-3.21 to its current version. Integration with L2 is done for Wi-Fi to generate some of the link events. Implementation of rest of the events, command service, information service and integration with WiMax and LTE are the future scope of this work. Remote MIH exchanges are also yet to be implemented in ns-3. This repository contains the implementation of MIH in ns-3 [3].

Use the following steps to run the example that demonstrates the generation of link events:
1) Configure and build the repository using the following commands:
- ./waf configure
- ./waf

2) Run the example with the following command:
- ./waf --run wifi-mih-example

### References
[1] IEEE P802.21-2008, "Standard for Local and metropolitan area networks - Media Independent Handover Services", January 2009, 323 pp., http://ieeexplore.ieee.org/document/4769367/

[2] http://code.nsnam.org/salumu/ns-3-mih/

[3] https://www.nsnam.org
