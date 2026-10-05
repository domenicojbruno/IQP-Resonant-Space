# IQP Report Revision Plan

> Review of the report draft dated May 4, 2026 (*Interference Patterns: A Resonant Space*), made 2026-10-05.
> Full row-by-row tracker (grammar, citations, structure): `IQP_Report_Revision_Tracker.xlsx`, delivered in chat.
> Draft text below was written from the project files as of 21-23 Sept 2026. Text in [brackets] is a placeholder.

## Content and structure changes to the existing draft

| Section | Page | Type | Issue | Change |
|---|---|---|---|---|
| Title page | cover | Content update | Lists one author (Corwin M. Loomis-Price) and the date May 4, 2026, but the text says "we" and "the team." | Update the author list, advisor line, and submission date for the current term. Confirm with your advisor whether this is one continuing report or a new report that cites the earlier one. |
| Abstract | i | Content update | Describes only research and a proposed display. No mention of the control system. | Add one or two sentences on what was built: the actuated wall, the ESP32 motor driver boards, time-of-flight sensing, and the wireless master control software. |
| Executive Summary | iii | Content update | 3. Methodology says the work was research, brainstorming and fabrication only. | Add the controls work: custom motor driver board, firmware, wireless link, master software, sensor testing. |
| Executive Summary | iii | Content update | 4. Findings: "the complete immersive installation was not finalized" and treats the wall as a static prototype. | Restate what now works and what does not. Use the same status wording as the new Section 4.3. |
| Executive Summary | iii | Content update | 5. Recommendations lists "Sensor testing and responsive system integration" as future work. | Remove or narrow items that are now done; replace with the remaining work (camera tracking, sensor port to ESP32, board revision, lighting, sound, participant study). |
| 3.2.3 | 8 | Content update | Methods paragraph mentions "motion sensors" in general terms. | Name the actual sensing approach (VL53L0X time-of-flight threshold sensors, planned overhead cameras) and point to the new Section 3.4.9. |
| 3.4 | 13 | Content update | Intro says this iteration "primarily focused on intellectual development and construction of the foundational arches wall structures. Additional features remain areas for continued research." | Rewrite to introduce both halves of the build: mechanical (3.4.1-3.4.3) and electrical/controls (new 3.4.4-3.4.9). |
| 3.4.2 | 14-16 | Content update | Describes the linear actuators but not how they are powered or driven. | Add one sentence that each actuator is driven by a BTS7960 H-bridge and refer to new Section 3.4.5. Add actuator model, stroke and voltage if known. |
| 3.4.3 | 16-18 | Content update | Describes "arches" and scissor wall units. Your control spec describes 13 hexagonal actuator positions in three rows (5 / 3 / 5). | Reconcile the two descriptions. State the actual number of actuators and their layout, and add a layout figure. |
| 4 Findings | 19 | Content update | Opening paragraph says the iteration "focused on conceptual development and construction of the foundational archway wall prototype." | Update to include control system development and bench testing. Keep the "preliminary" caveat for wellness outcomes. |
| 4.1 | 19-20 | Content update | "Even without the full integration of responsive systems..." | Update to reflect the level of integration actually reached at submission. |
| 4.2 | 20-21 | Content update | Six-item list of future focus areas includes "Implementing motion-responsive interaction." | Move completed items into findings; keep only work that is still open. |
| 5.1.1 | 21-22 | Content update | "Code Base Sensor Testing" is written as future work for the next team. | Replace with the new Section 5.1.1 draft (see Sections to Add). Most of this is now current work. |
| 5.1.2 | 22 | Content update | "Sensor Integration and Build Finalization" is written as future work. | Rewrite as remaining integration steps: install new board, verify pinout, calibrate each actuator, mount sensors. |
| 5.3 Deliverables | 25-26 | Content update | Lists "Preliminary coding frameworks for responsive interaction systems and their connected electrical systems" with no detail anywhere in the report. | Replace with specific deliverables: motor board firmware, gateway firmware, master control application, motor driver board schematic and PCB layout, sensor test sketch, code repository. |
| 5.4 Conclusions | 26 | Content update | Two sentences only. | Expand to a full paragraph covering both the wellness argument and the working technical platform. |
| Appendices | 29-31 | Content update | Only image appendices A and B. | Add technical appendices (pinout, command protocol, code repository). See Sections to Add. |
| 5.3 Deliverables | 25-26 | Structure | Section number 5.3 is used twice (Long-Term Recommendations and Deliverables). | Renumber Deliverables to 5.4 and Conclusions to 5.5. |
| Executive Summary | iii | Structure | All three Executive Summary pages are numbered III. | Number them iii, iv, v. |
| Table of Contents | 1-2 | Structure | Table of Contents pages are numbered 1-2 and the Introduction starts on 3. | Keep roman numerals through the front matter and start the Introduction on page 1. |
| Table of Contents | 1-2 | Structure | Entries will be out of date after renumbering and new sections. | Regenerate the table of contents last. |
| Executive Summary | iii | Structure | "Conclusion" is unnumbered while the other parts are numbered 1-5. | Number it 6 or remove the numbering from all parts. |
| 3.1 | 7 | Structure | Headings 3.1-3.4 end in colons ("Purpose:", "Objectives list:", "Design Process:", "Construction Process:"); no other headings do. | Remove the colons. Change "Objectives list" to "Objectives." |
| 3.2.1 | 7 | Structure | Objective 1 has an intro paragraph; Objectives 2 and 3 do not. | Add a one-sentence intro to Objectives 2 and 3, or remove it from Objective 1. |
| Whole document | all | Structure | The wall is called "arches wall," "arch wall," "archway wall," and "wall prototype." | Pick one term and use it everywhere. |
| Whole document | all | Structure | "Well being," "wellbeing," and "well-being" are all used. | Use "well-being" throughout. |
| Whole document | all | Structure | "IQP," "Qualifying Projects," and "continuation projects" are used interchangeably. | Define IQP once, then use it consistently. |
| Whole document | all | Structure | Mixed first person ("our research," "our community") and third person ("the team," "the project"). | Choose one voice. |
| 1 Introduction | 3 | Citation | "(WPI Well Being Task Force, 2022)" is cited but missing from the Bibliography. | Add the full reference. |
| 3.2.1 | 7 | Citation | "(Watson, 1988)" is cited for PANAS, but the listed reference is a different Watson paper ("Intraindividual and interindividual analyses..."). | Verify. PANAS is normally cited as Watson, Clark & Tellegen (1988), Journal of Personality and Social Psychology, 54(6), 1063-1070. |
| 1 Introduction | 3 | Citation | "(Chen et al., 2021)" - the reference has two authors. | "(Chen & Zeng, 2021)" |
| 2.1 | 4 | Citation | "(Liu et al, 2024)" - the reference has two authors. | "(Liu & Lin, 2024)" |
| 2.5.2 | 6 | Citation | "(Pinter, 2025)" - the reference has three authors. | "(Pinter et al., 2025)" |
| 2.5.1 | 5-6 | Citation | "Positive and Negative Affect Surveys (PANAS)"; also "Survey" in 5.1.3. | "Positive and Negative Affect Schedule (PANAS)" in both places. |
| 5.3.1 | 25 | Citation | "Worcester Museum of Art" | "Worcester Art Museum" |
| Bibliography | 27-28 | Citation | Not in alphabetical order (Hadavi is listed before Godley). | Move Godley above Hadavi. |
| Bibliography | 27-28 | Citation | Pinter entry uses a different format from the others (no commas after surnames, "doi:" prefix, year repeated). | Reformat to match the other APA entries. |
| Bibliography | 27-28 | Citation | No references for the hardware. | Add datasheets and documentation cited in the new technical sections (see Sections to Add, Bibliography additions). |
| 2.3 | 4-5 | Citation | Second and third paragraphs make research claims with no citation. | Add a source for the participatory art claims. |

(137 grammar and wording fixes are in the spreadsheet only.)

## Sections to add

### 2.6 Sensing, Actuation, and Control Technology

- **Where:** Background, after 2.5 Precedent Art Research
- **Why:** The report never explains the technology the wall depends on. A reader needs this before the new construction sections.
- **Check before using:** Add citations for the three datasheets and the ESP-NOW documentation. Confirm the sensor range figure against your own test results.
- **Source:** DS_vl53l0x.pdf; BTS7960 Motor Driver.pdf; esp32-wroom-32_datasheet_en.pdf

Responsive installations depend on three technical capabilities: sensing the presence of a participant, producing physical motion, and coordinating the two. This project used low-cost components that are common in student engineering work so that future teams can maintain and extend the system.

Presence was sensed with the VL53L0X time-of-flight (ToF) ranging sensor. A ToF sensor emits pulses of infrared laser light and measures the time the reflected light takes to return, which gives an absolute distance that does not depend on the color or reflectance of the target in the way that simple infrared proximity sensors do. The VL53L0X reports distances up to roughly two meters over an I2C connection, which is sufficient to detect a person crossing a hallway threshold.

Motion was produced by electric linear actuators. A DC actuator reverses direction when the polarity of its supply is reversed, so each actuator requires an H-bridge, a circuit of four switches that can connect the motor to the supply in either direction. The BTS7960 is a high-current half-bridge device; two of them form a full H-bridge that a microcontroller can command with two pulse-width-modulated (PWM) signals, one for each direction, where the duty cycle sets the speed.

Control was handled by the ESP32-WROOM-32, a microcontroller module with integrated Wi-Fi and enough PWM-capable pins to drive several actuators at once. The ESP32 also supports ESP-NOW, a connectionless wireless protocol from Espressif that lets boards exchange short packets addressed by hardware (MAC) address without a router. This allowed the wall's controller boards to be commanded without signal wiring between them.

### 3.2.4 Objective 4: Develop the Responsive Control System

- **Where:** Methodology, after Objective 3
- **Why:** The three existing objectives cover research and concept only. The controls work needs its own objective so the Findings can report against it.
- **Check before using:** Adjust the research questions to match what your advisor expects. Update Section 3.2 intro to say there are four objectives.
- **Source:** Project description; claude/resonant-wall-master-spec.md

This objective focused on giving the fabricated wall the ability to respond to people. The team set out to design the electronics, firmware, and software needed to detect a participant and move individual wall sections in response.

Objective 4: Research Questions:
- How can a participant's presence and position be detected reliably in a public hallway?
- How can many un-encoded linear actuators be driven safely and repeatably?
- How should motion be distributed across the wall so that the response reads as intentional rather than mechanical?

Methods:
The team selected components, designed a custom motor driver board, wrote firmware for the controller boards, and wrote a master control application. Each part was tested on the bench before integration. The construction of these systems is described in Sections 3.4.4 through 3.4.9.

### 3.4.4 Control System Architecture

- **Where:** Construction Process, after 3.4.3 Wall Creation
- **Why:** Gives the reader the whole system before the detail sections. Needs one block-diagram figure.
- **Check before using:** Confirm the 5 / 3 / 5 layout and thirteen-actuator count against the physical wall. Add a block diagram and a layout figure.
- **Source:** claude/resonant-wall-master-spec.md; claude/master-ui-and-gateway.md

The control system was organized in three layers. At the top, a master control application running on a PC decides how far each section of the wall should extend. In the middle, a single ESP32 connected to the PC by USB acts as a wireless gateway. At the bottom, three ESP32 motor driver boards mounted on the wall each control one horizontal row of actuators.

The wall was laid out as three rows of actuated sections in a staggered honeycomb pattern: five in the top row, three in the middle row, and five in the bottom row, for thirteen actuators in total. Each actuator is identified by its row and column (for example, M2,3 is the third actuator in the middle row), and each row is owned by one board. Figure [X] shows the layout and the flow of commands from the master to the actuators.

This arrangement keeps the boards on the wall simple. A board knows only how to move its own actuators safely and report their state. All decisions about which sections move, and when, are made by the master, so the behavior of the installation can be changed in software without reflashing the boards.

### 3.4.5 Motor Driver Board Design

- **Where:** After 3.4.4
- **Why:** The custom PCB is a major deliverable and is not mentioned anywhere in the draft.
- **Check before using:** State whether the new board has been fabricated and tested by submission. Add power-supply details (voltage, current, connector, fusing). Add schematic and PCB figures.
- **Source:** MotorDriver Board.pdf; MotorDriver Board.kicad_pcb; firmware/motor-board/config.h

A custom printed circuit board was designed in KiCad to hold one ESP32-WROOM-32 development module and connections for up to five BTS7960 motor drivers. Each driver receives two PWM signals from the ESP32, one that extends the actuator and one that retracts it.

Safety at power-up was a design requirement. The enable inputs of all five drivers are tied to a single ESP32 pin that is pulled to ground through a 10 kOhm resistor. The drivers therefore remain disabled whenever the microcontroller is unpowered, resetting, or has not yet started its program, and the actuators cannot move until the firmware deliberately enables them.

Each actuator has one limit-switch input. The extend and retract limit switches for an actuator share that single input, which halves the number of pins required; the firmware determines which switch was reached from the direction the actuator was travelling. The board also reserves pins for five infrared sensor inputs and one addressable LED data line for future lighting. The schematic is shown in Figure [X] and the full pin assignment is given in Appendix C.

An earlier prototype board, which carried only the PWM connections and no limit switches, was used for initial bench testing while the revised board was being prepared.

### 3.4.6 Motor Board Firmware

- **Where:** After 3.4.5
- **Why:** Documents how the actuators are controlled and how position is known without encoders.
- **Check before using:** Add measured travel times once calibration has been run on the wall. Confirm the PWM frequency and timeout values are the ones used at submission.
- **Source:** firmware/motor-board/README.md; MotorController.cpp; config.h

A single firmware program, written in C++ with the Arduino framework for ESP32, runs on all three motor driver boards. Two settings identify each board: its row and the number of actuators it drives.

The linear actuators have no position sensors, so the firmware estimates position from time. During a calibration routine, each actuator is retracted to its limit switch, then driven to the opposite limit while the firmware measures how long the full stroke takes. That travel time is stored in the board's non-volatile memory, separately for each actuator, because travel time varies from one actuator to another. Afterward, a request to move to a given percentage of extension is converted into a drive time.

At power-up, every actuator performs a homing move to its retract limit so that it starts from a known position. Because the two limit switches share one input, an actuator that starts with a switch already pressed first moves outward briefly and then approaches the retract limit, which guarantees it ends at the correct end.

Each actuator is managed as a small state machine with states for homing, idle, moving, calibrating, and fault. Every move is supervised by a timeout set to 1.5 times the measured travel time. If no limit switch is reached within that time, the firmware stops the actuator and reports a fault, which protects the mechanism if a switch fails. PWM is generated at 20 kHz, above the range of human hearing, so the drivers do not produce an audible tone in a space intended to be calming.

### 3.4.7 Wireless Communication

- **Where:** After 3.4.6
- **Why:** Explains the ESP-NOW link and why it replaced the serial plan.
- **Check before using:** Report measured range and reliability in the installation location if you test it. Confirm the status rate (250 ms) is unchanged.
- **Source:** claude/master-ui-and-gateway.md; firmware/motor-board/CommsEspNow.cpp; firmware/gateway/ResonantWallGateway.ino

The original plan was to command the boards over a wired serial connection. This was replaced with ESP-NOW, which requires no signal wiring between the master and the wall and addresses each board directly by its hardware address.

Commands are sent as four-byte packets containing an identifying byte, the actuator number, the command, and a value. Six commands are defined: drive forward, drive in reverse, stop, home, calibrate, and go to a position given as a percentage of full extension. A reserved actuator number applies a command to every actuator on a board.

Each board reports back to the master four times per second with a six-byte status packet for every actuator, giving its state, estimated position, and any fault. An emergency stop is implemented by broadcasting a stop command to all boards at once and then repeating it to each board individually. The complete packet formats are listed in Appendix D.

Each board also provides a text console over its USB port that accepts the same commands, which allowed individual boards to be tested without the rest of the system.

### 3.4.8 Master Control Software and Interaction Logic

- **Where:** After 3.4.7
- **Why:** This is where the engineering connects to the artistic intent. Currently absent from the report.
- **Check before using:** Add a screenshot of the interface. State which mode (threshold or position streaming) was used on the final wall. Adjust the final paragraph to match the team's own artistic intent.
- **Source:** master/resonant_wall_master.py; claude/resonant-wall-master-spec.md

The master control application was written in Python. It provides a graphical interface for configuring each board's connection, sending manual commands, monitoring the state of every actuator, and stopping the wall.

The interaction model treats a participant as a point on the face of the wall. Each wall section calculates its distance from that point, and its target extension is reduced according to a Gaussian (bell-shaped) function of that distance. The section directly in front of the participant retracts fully, neighboring sections retract partway, and distant sections remain extended. The result is a smooth recess that follows the participant along the wall, rather than individual sections switching on and off. Two parameters shape the behavior: a radius that sets how far the response spreads to neighboring sections, and a smoothing rate that sets how quickly sections follow their targets.

To prevent sections from chattering when a participant stands still near a boundary, commands are issued with hysteresis: a section is commanded to retract only when its target falls below 12 percent extension and to extend again only when its target rises above 85 percent. When the hallway is unoccupied, all sections return to full extension.

This behavior was designed to express the installation's theme. The wall yields to a person's presence and closes behind them, and the shared response of neighboring sections reflects the project's emphasis on connection rather than isolation.

### 3.4.9 Occupancy and Position Sensing

- **Where:** After 3.4.8
- **Why:** Documents the ToF sensor work, which is the core of the project description.
- **Check before using:** Add your measured detection distances. State how many sensors were installed, where, and whether they are read by an ESP32 at submission. State the status of camera tracking honestly (planned, prototyped, or working).
- **Source:** claude/vl53l0x-arduino-uno-test.md; TOFpins.png; DS_vl53l0x.pdf

Time-of-flight sensors were chosen to detect when a person enters the space in front of the wall. The design calls for VL53L0X sensors positioned at the hallway thresholds; a distance reading shorter than the width of the hallway indicates that someone has crossed.

The sensor was first evaluated on an Arduino Uno. It was configured in its long-range mode, which trades some speed and accuracy for greater detection distance, and polled ten times per second. The test confirmed that the sensor was suitable for threshold detection at hallway distances. Because the sensor measures reflected infrared light, its range is expected to be reduced in direct sunlight, which should be considered when the installation site is chosen.

When several of these sensors share one I2C bus, each must be assigned a unique address at start-up. This is done by holding all sensors in shutdown and enabling them one at a time, assigning each an address before the next is enabled.

The threshold sensors establish whether the space is occupied. Tracking where a participant is along the wall is planned to use overhead cameras, with the tracked position supplied to the interaction logic described in Section 3.4.8.

### 3.5 Safety Considerations

- **Where:** Methodology, after 3.4
- **Why:** A moving wall in a public hallway raises obvious safety questions that reviewers will ask. The report says nothing about them.
- **Check before using:** Fill in the bracketed sentence. Check with your advisor whether WPI requires a safety review or IRB approval for participant studies.
- **Source:** firmware/motor-board/README.md; claude/motor-board-peripheral-firmware.md

Because the installation moves in a space shared with the public, safety was treated as a design constraint. Several measures were built into the electronics and firmware. The motor drivers are disabled by default in hardware and are enabled only after the firmware has started. Each actuator stops at its limit switches, and a timeout stops any actuator that fails to reach one. An emergency stop in the master application halts every actuator with a single action, and closing the application sends the same stop command.

Some risks remain in the current design. The scissor mechanisms create pinch points that are not yet guarded. The boards cannot measure motor current, so an actuator that is blocked by an obstacle will continue to push until its timeout expires. [Describe the force the actuators can apply and any physical guarding, speed limits, or signage used.] These items should be addressed before the installation is left unattended in a public location.

### 4.3 Control System Development and Bench Testing

- **Where:** Findings, after 4.2
- **Why:** Findings currently report only on the mechanical wall and the brainstorming sessions.
- **Check before using:** IMPORTANT: replace both bracketed passages with what was actually tested. As of the project notes, the revised board had not been built and the gateway had not been run on hardware. Do not keep the last paragraph unless the hardware results support it.
- **Source:** claude/master-ui-and-gateway.md (Status); claude/vl53l0x-arduino-uno-test.md

The control system was developed and tested in stages. The time-of-flight sensor was confirmed to detect targets at hallway distances when tested on an Arduino Uno. The motor board firmware was exercised on the prototype driver board, where [N] actuators were driven in both directions from the USB console. The master control application was verified against simulated boards that reproduced the firmware's command and status packets, which confirmed address detection, delivery acknowledgement, status decoding, both motion modes, and the emergency stop.

[Report results on real hardware here: whether the gateway and boards communicated over ESP-NOW, whether homing and calibration ran on the revised board, the measured travel time of each actuator, and whether the wall responded to a person.]

These results show that a wall of independently actuated sections can be coordinated wirelessly from a single controller using inexpensive components, and that the interaction behavior can be developed and adjusted in software separately from the hardware.

### 4.4 Technical Limitations

- **Where:** Findings, after 4.3
- **Why:** Stating limits plainly strengthens the report and gives the next team a starting list.
- **Check before using:** Remove any limitation that has been fixed by submission.
- **Source:** claude/motor-board-peripheral-firmware.md (Open items)

Several limitations of the current system should be noted. Position is estimated from drive time rather than measured, so accuracy depends on calibration and will drift if the load on an actuator or the supply voltage changes; each actuator re-establishes its position only when it reaches a limit switch. The boards have no current sensing, so a stalled or obstructed actuator is detected only by timeout. On the revised board, two limit-switch inputs share pins with the USB serial port, which restricts use of the USB console while those actuators are running. The infrared sensor inputs and LED output on the board are not yet used.

Most importantly, no participant data were collected. The system's effect on mood, stress, or social connection has not been measured, and the wellness claims in this report remain a hypothesis supported by the literature rather than by this installation.

### 5.1.1 (replacement) Complete Hardware Integration and Calibration

- **Where:** Replaces existing 5.1.1 Code Base Sensor Testing
- **Why:** The existing text describes work that is now largely done.
- **Check before using:** Delete steps that are complete by submission.
- **Source:** firmware/motor-board/README.md (Suggested next steps)

The next team should begin by completing integration of the control system on the wall. The pin assignments of the revised motor driver board should be verified against the fabricated board before motors are connected. Each board should then be given its identity and the gateway's address, and every actuator should be homed and calibrated so that its travel time is stored. The team should confirm that limit switches stop each actuator at both ends and that the emergency stop halts the whole wall.

Two hardware changes are recommended for the next board revision: moving the two limit-switch inputs that share the USB serial pins to other pins, and connecting the motor drivers' current-sense outputs to the ESP32 so that stalls and obstructions can be detected directly.

### 5.1.2 (replacement) Sensor Integration and Participant Tracking

- **Where:** Replaces existing 5.1.2
- **Why:** Narrows the old section to what is still open.
- **Check before using:** Confirm the status of camera tracking before using this text.
- **Source:** claude/vl53l0x-arduino-uno-test.md (Open items); claude/resonant-wall-master-spec.md

With the wall moving reliably, the next step is to connect it to people. The time-of-flight sensor code should be moved from the Arduino Uno to an ESP32, extended to several sensors on one bus, and used to signal the master when the hallway is occupied. The camera-based tracking that supplies a participant's position along the wall should then be developed and mapped onto the wall's coordinate system. The interaction parameters, including the response radius, smoothing rate, and thresholds, should be tuned with real participants rather than chosen in advance.

The lighting and sound elements proposed during the participatory sessions can then be layered onto the same master application, using the LED output already present on the boards.

### Appendix C Motor Driver Board Pin Assignments

- **Where:** After Appendix B
- **Why:** Future teams need this to wire and debug the boards.
- **Check before using:** This table was transcribed from the schematic PDF and is marked unverified in the project notes. Check every row against the built board before publishing. Format as a real table in the report.
- **Source:** firmware/motor-board/config.h; claude/motor-board-peripheral-firmware.md

Table C1. ESP32 pin assignments on the motor driver board (revised board).

Driver enable (all five drivers): GPIO 2, pulled down through 10 kOhm
Actuator 0 extend / retract PWM: GPIO 12 / 13; limit input: GPIO 3
Actuator 1 extend / retract PWM: GPIO 26 / 25; limit input: GPIO 1
Actuator 2 extend / retract PWM: GPIO 32 / 27; limit input: GPIO 21
Actuator 3 extend / retract PWM: GPIO 18 / 19; limit input: GPIO 14
Actuator 4 extend / retract PWM: GPIO 22 / 23; limit input: GPIO 15
Infrared sensor inputs (unused): GPIO 36, 39, 34, 35, 5
LED strip data (unused): GPIO 4
Status LED: GPIO 16

### Appendix D Command and Status Protocol

- **Where:** After Appendix C
- **Why:** Documents the interface so the master and boards can be developed independently.
- **Check before using:** Format as tables. Confirm the values match the firmware at submission.
- **Source:** claude/master-ui-and-gateway.md (Protocol)

Command packet (master to board), 4 bytes: [0xA5, actuator index, command, value]. An actuator index of 0xFF addresses every actuator on the board.

Commands: 0 forward, 1 reverse, 2 stop, 3 home, 4 calibrate, 5 go to position. The value is a PWM duty of 0-255 for forward and reverse, a percentage of 0-100 for go to position, and is ignored otherwise.

Status packet (board to master), 6 bytes: [0xA6, board number, actuator index, state, position percent, fault], sent every 250 ms for each actuator. A position of 255 means the position is not yet known.

States: 0 uninitialized, 1 homing (clearing limit), 2 homing (seeking limit), 3 idle, 4 moving forward, 5 moving reverse, 6 calibrating, 7 fault.
Faults: 0 none, 1 homing timeout, 2 runaway timeout.

### Appendix E Source Code and Design Files

- **Where:** After Appendix D
- **Why:** Lets future teams find the code. Standard for a continuation project.
- **Check before using:** Confirm the repository is public (or accessible to WPI) and that the listed contents are actually committed.
- **Source:** Project instructions; claude/github-repo-index.md

All firmware, software, and board design files produced for this project are available at https://github.com/domenicojbruno/IQP-Resonant-Space. The repository contains the motor board firmware, the gateway firmware, the master control application, and the KiCad files for the motor driver board. Instructions for building and flashing the firmware are included in the repository.

### Front matter List of Figures

- **Where:** After Table of Contents
- **Why:** The report has 16 figures and will gain more. A figure list is standard in WPI project reports.
- **Check before using:** Generate automatically; do not type by hand.
- **Source:** Existing figure captions

List of Figures

Figure 1: Participatory brainstorming session ... [page]
Figure 2: Early collaborative concept ideation sessions ... [page]
[Continue for every figure. Generate automatically from the captions in your word processor.]

### Front matter Authorship

- **Where:** After Acknowledgements
- **Why:** The text says "we" but one author is listed. An authorship statement makes clear who wrote and built what, especially for a continuation.
- **Check before using:** Fill in. Check whether your advisor requires this section.
- **Source:** None - needs your input

[Name] wrote Sections [..] and led [mechanical design / fabrication]. [Name] wrote Sections [..] and led [electronics / firmware / software]. All authors reviewed and edited the full report.

### Bibliography Bibliography additions

- **Where:** Merge alphabetically into the existing list
- **Why:** The new technical sections cite hardware that has no references, and one existing citation has no entry.
- **Check before using:** Take the year and revision from each datasheet's cover page. Verify the Watson reference and the exact title of the WPI task force report before using them.
- **Source:** Datasheet PDFs in the project

Espressif Systems. ([year]). ESP32-WROOM-32 datasheet (Version [x]). [URL]

Espressif Systems. ([year]). ESP-NOW. ESP-IDF Programming Guide. [URL]

Infineon Technologies. ([year]). BTS 7960 high current PN half bridge NovalithIC (Rev. [x]). [URL]

STMicroelectronics. ([year]). VL53L0X: Time-of-Flight ranging sensor datasheet (Rev. [x]). [URL]

Watson, D., Clark, L. A., & Tellegen, A. (1988). Development and validation of brief measures of positive and negative affect: The PANAS scales. Journal of Personality and Social Psychology, 54(6), 1063-1070.

WPI Mental Health and Well-Being Task Force. (2022). [Report title]. Worcester Polytechnic Institute. [URL]

