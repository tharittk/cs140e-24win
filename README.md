## CS140E: embedded operating systems (Engler, Winter, 2024)

<p align="center">
  <img src="labs/lab-memes/pi-intro.jpg" width="600" />
</p>

Tl;dr:
  - It's a lab class, so [jump right to the labs](./labs/README.md).

CS140E is an introductory operating systems course. It roughly covers
the same high-level material as [CS 212][cs212] (formerly CS 140), but
with a focus on embedded systems, interacting directly with hardware,
and verification. Both courses cover concepts such as virtual memory,
filesystems, networking, and scheduling, but take different approaches
to doing so. By the end of 140E, you will have (hopefully) built your
own simple, clean operating system for the widely-used, ARM-based
[Raspberry Pi][raspberrypi].

------------------------------------------------------------------------
### Document to myself

- [x] How compiler may bite you in bare-metal programming & assembly
- [x] Self-Replicating Code (implementing Ken Thompson's paper)
- [x] GPIO: First time with with BCM2835 Datasheet
- [x] Cross-check: Use checksum with tracing (extensively)
- [x] Interrupt: BCM2835 Datasheet again, code trapping mechanism (ARM Assembly heavy)
- [x] Thread: hand-roll a simple thread + context switch mechanism. Learn how to "brain surgey" through implementign `fork`
- [x] Bootleader: Makes Unix + Pi talk. Sending code through UART.
Last updated @ November 13, 2025
