A fully-fledge project that unveils the power of unauthorized Ring0 memory manipulation.

This project is based on the RWDrv.sys exploit seen in [Driver-Based Attacks: Past and Present](https://www.rapid7.com/blog/post/2021/12/13/driver-based-attacks-past-and-present/)

By extending a handle to the IOCTL driver, a crafted struct can be used to abuse externally-facing functions allowing--
1. Read access
2. Write access

credits to 
@IChooseYou (leak_k_process() method)
