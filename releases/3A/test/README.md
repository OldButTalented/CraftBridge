# Option 3A tests

This directory contains the validated test material associated with the Option 3A firmware baseline.

- `test_core/`: deterministic native host tests for session, decoding, runtime metrics and Web rendering.
- `tools/ecu_simulator/`: deterministic SmartCraft ECU simulator and its regression tests.
- `tools/gate2_real_ecu/`: isolated physical Gate 2 harness used to verify the session protocol against the tested ECU.
- `tools/verify_motor_node_authority.py`: comparison of released firmware behavior with the separately held authority dataset.

The private authority dataset and raw Mercury/WP7 evidence are intentionally not included. Simulator and host-test results must not be described as physical validation.
