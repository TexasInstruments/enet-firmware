# Validation {#val_notes_mainpage}

[TOC]

# Introduction {#val_intro}

Sections below lists various validation reports that are included in the release.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# MCAL HIS Metric Report {#val_his_rep}
# MCAL KW Static Analysis Report {#val_kw_rep}

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# EthFw Unit Test Reports {#val_unit_rep}

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Performance Measurements {#val_perform_measurements}

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## EthFw NDK Stack
The CAN driver performance is characterized by the number of messages that is transmitted per second and the associated CPU load (CPU cycles required)

CAN Performance is measured under two main categories
-# Transmit (TX Only) : The CAN peripheral transmits pre-determined messages

-# Loopback mode (TX & RX) : The CAN peripheral provides a mechanism to receive back the message that was transmitted, without transmitting the same on the CAN bus.

Operation | Number of messages per second | % CPU Load | Platform & CPU Clock
----------|-------------------------------|------------|---------------------
TX Only   | ~5500                         | 6%         | DRA80X, R5F @ 400 MHz
TX & RX (Loopback mode)  | ~9950                         | 15%        | DRA80X, R5F @ 400 MHz

- CAN Configurations
    - Arbitration phase bit-rate <b>1 Mbps</b> (Mega Bits Per Second)
    - Data phase bit-rate <b>5 Mbps</b> (Mega Bits Per Second)
    - Payload <b>64 Bytes</b> with bit-rate switch <b>ON</b>
    - Message Identifier <b>Extended (29 bits) </b>

- Measured Hardware utilization
    - Utilization of <b>84.5% </b> was observed (TX only configuration)
        - For 1 & 5 Mbps bit rate, theoretical number of messages that could be transmitted per second is 6510)
        - utilization = (5500 / 6510) * 100 = 84.5%
