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
### My Progression

- [x] How compiler may bite you in bare-metal programming & assembly
- [x] Self-Replicating Code (implementing Ken Thompson's paper)
- [x] GPIO: First time with with BCM2835 Datasheet
- [x] Cross-check: Use checksum with tracing (extensively)
- [x] Interrupt: BCM2835 Datasheet again, code trapping mechanism with timer interrupt (ARM Assembly heavy)

- [x] Thread: hand-roll a simple thread + context switch mechanism. Learn how to "brain surgey" through implementign `fork`
- [x] Bootleader: Makes Unix + Pi talk. Sending code through UART.
- [x] UART Driver: life was good when you have an access to `printk`. Now I cannot take it as given.
- [x] Interrupt Part 2: program the BCM2835 to handle interrupt from custom sources like GPIO pins
- [x] Debug Hardware: set and handle breakpoint/watchpoint exceptions and work with co-processor registers.

- [x] IMU/I2C: Interface with an accelerometer + gyroscope (MPU-6050) over the I2C protocol; write your own I2C driver from the Broadcom datasheet.
- [x] I2C + Mailboxes: Implement the I2C hardware driver from scratch; communicate with the GPU via the Pi's mailbox interface to query hardware info.
- [x] Single-Step Equivalence (Part 1): Read/write banked registers across ARM modes; build the foundations for pre-emptive single-step checking.
- [x] Single-Step Equivalence (Part 2): Wire it all together — hash every register on every instruction executed to detect any subtle OS state mismatch.

- [x] Virtual Memory (Pinned TLB): Enable a working VM system using pinned TLB entries; handle permission faults, domain faults, and unmapped memory exceptions.
- [x] Virtual Memory (Page Tables): Implement a section-based 1-level page table replacing the pinned approach from the previous lab. Simulate page-table walk of the MMU unit for double-check.

Last updated @ March 12, 2026
